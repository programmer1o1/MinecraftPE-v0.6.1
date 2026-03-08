#include "RegionFile.h"
#include "../../../platform/log.h"

const int SECTOR_BYTES = 4096;
const int SECTOR_INTS = SECTOR_BYTES / 4;
const int SECTOR_COLS = 32;

static const char* const REGION_DAT_NAME = "chunks.dat";

static void logAssert(int actual, int expected) {
	if (actual != expected) {
		LOGI("ERROR: I/O operation failed (%d vs %d)\n", actual, expected);
	}
}

RegionFile::RegionFile(const std::string& basePath)
:	file(NULL)
{
	filename = basePath;
	filename += "/";
	filename += REGION_DAT_NAME;

	offsets = new int[SECTOR_INTS];

	emptyChunk = new int[SECTOR_INTS];
	memset(emptyChunk, 0, SECTOR_INTS * sizeof(int));
}

RegionFile::RegionFile(const std::string& fullFilePath, bool /*isFullPath*/)
:	file(NULL)
{
	filename = fullFilePath;

	offsets = new int[SECTOR_INTS];

	emptyChunk = new int[SECTOR_INTS];
	memset(emptyChunk, 0, SECTOR_INTS * sizeof(int));
}

RegionFile::~RegionFile()
{
	close();
	delete [] offsets;
	delete [] emptyChunk;
}

bool RegionFile::open()
{
	close();

	memset(offsets, 0, SECTOR_INTS * sizeof(int));

	// attempt to open file
	file = fopen(filename.c_str(), "r+b");
	if (file)
	{
		// read offset table
		logAssert(fread(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);

		// mark initial sector as blocked
		sectorFree[0] = false;

		// setup blocked sectors
		for (int sector = 0; sector < SECTOR_INTS; sector++)
		{
			int offset = offsets[sector];
			if (offset)
			{
				int base = offset >> 8;
				int count = offset & 0xff;

				for (int i = 0; i < count; i++)
				{
					sectorFree[base + i] = false;
				}
			}
		}
	}
	else
	{
		// new world
		file = fopen(filename.c_str(), "w+b");
		if (!file)
		{
			LOGI("Failed to create chunk file %s\n", filename.c_str());
			return false;
		}

		// write sector header (all zeroes)
		logAssert(fwrite(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);

		// mark initial sector as blocked
		sectorFree[0] = false;
	}

	return file != NULL;
}

void RegionFile::close()
{
	if (file)
	{
		fclose(file);
		file = NULL;
	}
}

bool RegionFile::readChunk(int x, int z, RakNet::BitStream** destChunkData)
{
	int offset = offsets[x + z * SECTOR_COLS];

	if (offset == 0)
	{
		// not written to file yet
		return false;
	}

	int sectorNum = offset >> 8;
	int length = 0;

	if (!_fileCache.empty())
	{
		// Fast path: read directly from the in-memory cache.
		size_t byteOffset = (size_t)sectorNum * SECTOR_BYTES;
		if (byteOffset + sizeof(int) > _fileCache.size())
		{
			LOGI("ERROR: RegionFile cache underrun at sector %d\n", sectorNum);
			return false;
		}
		memcpy(&length, &_fileCache[byteOffset], sizeof(int));
		assert(length < ((offset & 0xff) * SECTOR_BYTES));
		length -= sizeof(int);

		unsigned char* data = new unsigned char[length > 0 ? length : 1];
		if (length > 0)
		{
			size_t dataOffset = byteOffset + sizeof(int);
			if (dataOffset + (size_t)length > _fileCache.size())
			{
				LOGI("ERROR: RegionFile cache data underrun for chunk (%d,%d)\n", x, z);
				delete[] data;
				return false;
			}
			memcpy(data, &_fileCache[dataOffset], length);
		}
		*destChunkData = new RakNet::BitStream(data, length > 0 ? length : 0, false);
	}
	else
	{
		// Slow path: seek + read from file.
		fseek(file, sectorNum * SECTOR_BYTES, SEEK_SET);
		fread(&length, sizeof(int), 1, file);

		assert(length < ((offset & 0xff) * SECTOR_BYTES));
		length -= sizeof(int);
		if (length <= 0) {
			//*destChunkData = NULL;
			//return false;
		}

		unsigned char* data = new unsigned char[length];
		logAssert(fread(data, 1, length, file), length);
		*destChunkData = new RakNet::BitStream(data, length, false);
		//delete [] data;
	}

	return true;
}

bool RegionFile::preloadToMemory()
{
	if (!file)
		return false;

	// Determine file size
	if (fseek(file, 0, SEEK_END) != 0)
		return false;
	long fileSize = ftell(file);
	if (fileSize <= 0)
		return false;
	rewind(file);

	_fileCache.resize((size_t)fileSize);
	size_t bytesRead = fread(_fileCache.data(), 1, (size_t)fileSize, file);
	if ((long)bytesRead != fileSize)
	{
		LOGI("ERROR: preloadToMemory only read %zu of %ld bytes\n", bytesRead, fileSize);
		_fileCache.clear();
		return false;
	}

	// Re-read the offset table from the cache (it was already loaded in open(),
	// but now the file pointer is at EOF — restore it so write() still works).
	rewind(file);

	LOGI("RegionFile: preloaded %ld bytes into memory cache\n", fileSize);
	return true;
}

void RegionFile::freeMemoryCache()
{
	std::vector<unsigned char>().swap(_fileCache); // release memory
}

bool RegionFile::writeChunk(int x, int z, RakNet::BitStream& chunkData)
{
	int size = chunkData.GetNumberOfBytesUsed() + sizeof(int);

	int offset = offsets[x + z * SECTOR_COLS];
	int sectorNum = offset >> 8;
	int sectorCount = offset & 0xff;
	int sectorsNeeded = (size / SECTOR_BYTES) + 1;

	if (sectorsNeeded > 256)
	{
		LOGI("ERROR: Chunk is too big to be saved to file\n");
		return false;
	}

	if (sectorNum != 0 && sectorCount == sectorsNeeded) {
		// the sector can be written on top of its old data
		write(sectorNum, chunkData);
	}
	else
	{
		// we need a new location

		// mark old sectors as free
		for (int i = 0; i < sectorCount; i++)
		{
			sectorFree[sectorNum + i] = true;
		}

		// find an available slot with enough run length
		int slot = 0;
		int runLength = 0;
		bool extendFile = false;
		while (runLength < sectorsNeeded)
		{
			if (sectorFree.find(slot + runLength) == sectorFree.end())
			{
				extendFile = true;
				break;
			}
			if (sectorFree[slot + runLength] == true)
			{
				runLength++;
			}
			else
			{
				slot = slot + runLength + 1;
				runLength = 0;
			}
		}

		if (extendFile)
		{
			fseek(file, 0, SEEK_END);
			for (int i = 0; i < (sectorsNeeded - runLength); i++)
			{
				fwrite(emptyChunk, sizeof(int), SECTOR_INTS, file);
				sectorFree[slot + i] = true;
			}
		}
		offsets[x + z * SECTOR_COLS] = (slot << 8) | sectorsNeeded;
		// mark slots as taken
		for (int i = 0; i < sectorsNeeded; i++)
		{
			sectorFree[slot + i] = false;
		}

		// write!
		write(slot, chunkData);

		// write sector data
		fseek(file, (x + z * SECTOR_COLS) * sizeof(int), SEEK_SET);
		fwrite(&offsets[x + z * SECTOR_COLS], sizeof(int), 1, file);
	}


	return true;
}

bool RegionFile::write(int sector, RakNet::BitStream& chunkData)
{
	fseek(file, sector * SECTOR_BYTES, SEEK_SET);
	//LOGI("writing %d B to sector %d\n", chunkData.GetNumberOfBytesUsed(), sector);
	int size = chunkData.GetNumberOfBytesUsed() + sizeof(int);
	logAssert(fwrite(&size, sizeof(int), 1, file), 1);
	logAssert(fwrite(chunkData.GetData(), 1, chunkData.GetNumberOfBytesUsed(), file), chunkData.GetNumberOfBytesUsed());

	return true;
}
