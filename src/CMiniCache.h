#ifndef CMINICACHE_H_
#define CMINICACHE_H_

#include <pthread.h>
#include "tmap.h"
#define		PROJECTBASEERRORCODE_MINICACHE	30

typedef struct CMiniCacheNode CMiniCacheNode;
typedef struct CMiniCache     CMiniCache;

CMiniCache *CMiniCache_alloc(size_t lHashSize, size_t lMaxDataSize, size_t lMaxDataNum, size_t lOneAllocNum, int (*pDataFreeFun)(void*));
void CMiniCache_free(CMiniCache *cache);

int GetData(CMiniCache *cache, map_key_t tKey, void** vpData,size_t* lDataSize);
int AddData(CMiniCache *cache, map_key_t tKey, void*  vpData,size_t  lDataSize);
int DelData(CMiniCache *cache, map_key_t tKey);

int GetLastError(CMiniCache *cache, long* lErrorCode);
int SelfDelUnvalidNode(CMiniCache *cache);
int StopSelfDel(CMiniCache *cache);

void exportStats(CMiniCache *cache, char *dest, size_t len);

#endif
