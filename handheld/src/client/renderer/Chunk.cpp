#include "Chunk.h"
#include "Tesselator.h"
#include "TileRenderer.h"
#include "culling/Culler.h"
#include "../../world/entity/Entity.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/level/Region.h"
#include "../../world/level/chunk/LevelChunk.h"
#include "../../util/Mth.h"
//#include "../../platform/time.h"

/*static*/ int Chunk::updates = 0;
//static Stopwatch swRebuild;
//int* _layerChunks[3] = {0, 0, 0}; //Chunk::NumLayers];
//int _layerChunkCount[3] = {0, 0, 0};

Chunk::Chunk( Level* level_, int x, int y, int z, int size, int lists_, GLuint* ptrBuf/*= NULL*/)
:	level(level_),
	visible(false),
	compiled(false),
    _empty(true),
	xs(size), ys(size), zs(size),
	dirty(false),
	occlusion_visible(true),
	occlusion_querying(false),
	queued(false),
	lists(lists_),
	vboBuffers(ptrBuf),
	bb(0,0,0,1,1,1),
	t(Tesselator::instance)
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	, _cpuMeshReady(false)
	, _inFlight(false)
#endif
{
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	for (int i = 0; i < NumLayers; ++i) _cpuVertexCount[i] = 0;
#endif
	for (int l = 0; l < NumLayers; l++) {
		empty[l] = false;
	}

	radius = Mth::sqrt((float)(xs * xs + ys * ys + zs * zs)) * 0.5f;

	this->x = -999;
	setPos(x, y, z);
}

void Chunk::setPos( int x, int y, int z )
{
	if (x == this->x && y == this->y && z == this->z) return;

	reset();
	this->x = x;
	this->y = y;
	this->z = z;
	xm = x + xs / 2;
	ym = y + ys / 2;
	zm = z + zs / 2;

	const float xzg = 1.0f;
	const float yp = 2.0f;
	const float yn = 0.0f;
	bb.set(x-xzg, y-yn, z-xzg, x + xs+xzg, y + ys+yp, z + zs+xzg);

	//glNewList(lists + 2, GL_COMPILE);
	//ItemRenderer.renderFlat(AABB.newTemp(xRenderOffs - g, yRenderOffs - g, zRenderOffs - g, xRenderOffs + xs + g, yRenderOffs + ys + g, zRenderOffs + zs + g));
	//glEndList();
	setDirty();
}

void Chunk::translateToPos()
{
	glTranslatef2((float)x, (float)y, (float)z);
}

// Shared tessellation logic.
// cpuOnly=true  → uses tArg (background thread tesselator), stores into _cpuMesh[], NO GL calls.
// cpuOnly=false → uses t (global Tesselator::instance), uploads to GPU immediately (original path).
void Chunk::rebuildImpl(Tesselator& tArg, bool cpuOnly)
{
	updates++;

	int x0 = x;
	int y0 = y;
	int z0 = z;
	int x1 = x + xs;
	int y1 = y + ys;
	int z1 = z + zs;
	for (int l = 0; l < NumLayers; l++) {
		empty[l] = true;
	}
	_empty = true;

	LevelChunk::touchedSky = false;

	int r = 1;
	Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
	TileRenderer tileRenderer(&region);

	bool doRenderLayer[NumLayers] = {true, false, false};
	for (int l = 0; l < NumLayers; l++) {
		if (!doRenderLayer[l]) continue;
		bool renderNextLayer = false;
		bool rendered = false;

		bool started = false;
		int cindex = -1;

		for (int y = y0; y < y1; y++) {
			for (int z = z0; z < z1; z++) {
				for (int x = x0; x < x1; x++) {
					++cindex;
					int tileId = region.getTile(x, y, z);
					if (tileId > 0) {
						if (!started) {
							started = true;

#ifndef USE_VBO
							if (!cpuOnly) {
								glNewList(lists + l, GL_COMPILE);
								glPushMatrix2();
								translateToPos();
								float ss = 1.000001f;
								glTranslatef2(-zs / 2.0f, -ys / 2.0f, -zs / 2.0f);
								glScalef2(ss, ss, ss);
								glTranslatef2(zs / 2.0f, ys / 2.0f, zs / 2.0f);
							}
#endif
							tArg.begin();
							tArg.offset(0.0f, 0.0f, 0.0f);
						}

						Tile* tile = Tile::tiles[tileId];
						int renderLayer = tile->getRenderLayer();

						if (renderLayer > l) {
							renderNextLayer = true;
							doRenderLayer[renderLayer] = true;
						} else if (renderLayer == l) {
							rendered |= tileRenderer.tesselateInWorld(tile, x, y, z);
						}
					}
				}
			}
		}

		if (started) {
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
			if (cpuOnly) {
				tArg.endToCPU(_cpuMesh[l], _cpuVertexCount[l]);
			} else {
#ifdef USE_VBO
				renderChunk[l] = tArg.end(true, vboBuffers[l]);
				renderChunk[l].pos.x = (float)this->x;
				renderChunk[l].pos.y = (float)this->y;
				renderChunk[l].pos.z = (float)this->z;
#else
				tArg.end(false, -1);
				glPopMatrix2();
				glEndList();
#endif
			}
#else  // !desktop
#ifdef USE_VBO
			renderChunk[l] = tArg.end(true, vboBuffers[l]);
			renderChunk[l].pos.x = (float)this->x;
			renderChunk[l].pos.y = (float)this->y;
			renderChunk[l].pos.z = (float)this->z;
#else
			tArg.end(false, -1);
			glPopMatrix2();
			glEndList();
#endif
#endif  // desktop
			tArg.offset(0, 0, 0);
		} else {
			rendered = false;
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
			if (cpuOnly) {
				_cpuMesh[l].clear();
				_cpuVertexCount[l] = 0;
			}
#endif
		}
		if (rendered) {
			empty[l] = false;
			_empty = false;
		}
		if (!renderNextLayer) break;
	}

	skyLit = LevelChunk::touchedSky;
	compiled = true;
}

void Chunk::rebuild()
{
	if (!dirty) return;
	rebuildImpl(t, false);
}

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
void Chunk::rebuildCPU(Tesselator& tArg)
{
	// Note: dirty flag is NOT checked here — the caller (mesh thread) already
	// decided this chunk needs work. We reset empty[] then tessellate.
	rebuildImpl(tArg, true);
	_cpuMeshReady.store(true, std::memory_order_release);
}

void Chunk::uploadGPU()
{
#ifdef USE_VBO
	for (int l = 0; l < NumLayers; l++) {
		if (_cpuMesh[l].empty()) continue;

		int bytes = (int)_cpuMesh[l].size();
		int access = GL_STATIC_DRAW;
		glBindBuffer2(GL_ARRAY_BUFFER, vboBuffers[l]);
		glBufferData2(GL_ARRAY_BUFFER, bytes, _cpuMesh[l].data(), access);

		renderChunk[l].vboId     = vboBuffers[l];
		renderChunk[l].vertexCount = _cpuVertexCount[l];
		renderChunk[l].pos.x = (float)this->x;
		renderChunk[l].pos.y = (float)this->y;
		renderChunk[l].pos.z = (float)this->z;

		// Release CPU memory after upload
		_cpuMesh[l].clear();
		_cpuMesh[l].shrink_to_fit();
	}
#endif
	_cpuMeshReady.store(false, std::memory_order_release);
	_inFlight.store(false, std::memory_order_release);
}
#endif

float Chunk::distanceToSqr( const Entity* player ) const
{
	float xd = (float) (player->x - xm);
	float yd = (float) (player->y - ym);
	float zd = (float) (player->z - zm);
	return xd * xd + yd * yd + zd * zd;
}

float Chunk::squishedDistanceToSqr( const Entity* player ) const
{
	float xd = (float) (player->x - xm);
	float yd = (float) (player->y - ym) * 2;
	float zd = (float) (player->z - zm);
	return xd * xd + yd * yd + zd * zd;
}

void Chunk::reset()
{
	for (int i = 0; i < NumLayers; i++) {
		empty[i] = true;
	}
	visible = false;
	compiled = false;
    _empty = true;
}

int Chunk::getList( int layer )
{
	if (!visible) return -1;
	if (!empty[layer]) return lists + layer;
	return -1;
}

RenderChunk& Chunk::getRenderChunk( int layer )
{
	return renderChunk[layer];
}

int Chunk::getAllLists( int displayLists[], int p, int layer )
{
	if (!visible) return p;
	if (!empty[layer]) displayLists[p++] = (lists + layer);
	return p;
}

void Chunk::cull( Culler* culler )
{
	visible = culler->isVisible(bb);
}

void Chunk::renderBB()
{
	//glCallList(lists + 2);
}

bool Chunk::isEmpty()
{
	return compiled && _empty;//empty[0] && empty[1] && empty[2];
//	if (!compiled) return false;
//	return empty[0] && empty[1];
}

void Chunk::setDirty()
{
	dirty = true;
}

void Chunk::setClean()
{
	dirty = false;
	queued = false;
}

bool Chunk::isDirty()
{
	return dirty;
}

void Chunk::resetUpdates()
{
	updates = 0;
	//swRebuild.reset();
}
