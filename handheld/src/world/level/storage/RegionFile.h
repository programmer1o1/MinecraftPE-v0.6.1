#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__

#include <map>
#include <string>
#include <vector>
#include "../../../raknet/BitStream.h"

typedef std::map<int, bool> FreeSectorMap;

class RegionFile
{
public:
	RegionFile(const std::string& basePath);
	// Construct with a full file path (no "/chunks.dat" appended)
	RegionFile(const std::string& fullFilePath, bool isFullPath);
	virtual ~RegionFile();

	bool open();
	bool readChunk(int x, int z, RakNet::BitStream** destChunkData);
	bool writeChunk(int x, int z, RakNet::BitStream& chunkData);

	// Pre-read the entire region file into memory so subsequent readChunk()
	// calls use memory lookups instead of fseek/fread. Safe to call from any
	// thread; call freeMemoryCache() once all chunks are loaded to reclaim RAM.
	bool preloadToMemory();
	void freeMemoryCache();

private:
	bool write(int sector, RakNet::BitStream& chunkData);
	void close();

	FILE* file;
	std::string	filename;
	int* offsets;
	int* emptyChunk;
	FreeSectorMap sectorFree;

	// In-memory cache of the whole file (populated by preloadToMemory())
	std::vector<unsigned char> _fileCache;
};


#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__*/
