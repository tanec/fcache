#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "CMiniCache.h"
#include "util.h"

static int GetCacheNode(CMiniCache *cache, CMiniCacheNode** pNode);
static int  AddNode(CMiniCache *cache, CMiniCacheNode* pNode);
static int  DelNode(CMiniCache *cache, CMiniCacheNode* pNode,long lFlag);
//static int DelNode1(CMiniCache *cache);
static int FindNode(CMiniCache *cache, map_key_t tKey,CMiniCacheNode** pOldNode);

struct CMiniCacheNode {
  map_key_t tKey;
  md5_digest_t key; //key实际存放区，复制到这里避免多线程问题
  void  *vpData;    //数据域
  size_t lDataSize; //数据域长度

  CMiniCacheNode *pUp,   *pDown;
  CMiniCacheNode *pLeft, *pRight;
};
struct CMiniCache {
  size_t m_lMaxDataSize;       //最大可用内存字节数
  size_t m_lMaxDataNum;        //最大节点数
  size_t m_lOneAllocNum;       //一次分配的节点数
  size_t m_lNowAllocPos;       //当前已分配的节点块数
  size_t m_lAlloc;             //最大可分配块数
  size_t m_lNowDataPos;        //当前块已分配节点数（顺序分配）
  CMiniCacheNode **m_ppNode;   //预分配节点
  CMiniCacheNode *m_pIdleNode; //空闲链表头指针
  CMiniCacheNode *m_pEnd;      //数据使用时间链表尾指针
  CMiniCacheNode *m_pHead;     //数据使用时间链表头指针

  size_t m_lHashSize;          //哈希空间大小
  CMiniCacheNode **m_ppHash;   //哈希表
  size_t lDataSize;            //实际数据域总字节数
  size_t lDataNum;             //实际数据总数
  int m_lLastErrorCode;        //最后一次错误代码

  int m_lRunFlag;              //自淘汰循环标志
  void (*m_pDataRefFun)(void*);
  int (*m_pDataFreeFun)(void*);//数据域释放函数
  pthread_mutex_t lock;     //多线程下使用临界
};


/**************************************************************************
  //Function:			FreeSource
  //Description:		释放所用资源
  //Calls:
  //Called by:
  //Input:
  //Output:
  //Return:
            返回值				说明
            0					成功
            负值				失败
  //Others:
  //Author:	fanyh	Email: fanyh@zhongsou.com
  //Date:		2008-06-16
  **************************************************************************/
static inline int
FreeSource(CMiniCache *cache)
{
  if (cache == NULL) return 0;
  size_t i;
  for(i=0; i<cache->m_lNowAllocPos; i++) {
    if(cache->m_ppNode[i]) {
      free(cache->m_ppNode[i]);
      cache->m_ppNode[i] = NULL;
    }
  }
  if(cache->m_ppNode) {
    free(cache->m_ppNode);
    cache->m_ppNode = NULL;
  }
  if(cache->m_ppHash) {
    free(cache->m_ppHash);
    cache->m_ppHash = NULL;
  }
  pthread_mutex_destroy(&cache->lock);
  free(cache);
  return 0;
}

/**************************************************************************
//Function:			Init
//Description:		设定哈希空间、节点最大数目、一次分配节点数目等
//Calls:
//Called by:
//Input:
          lHashSize				哈希空间大小
          lMaxDataNum				节点最大数目
          lOneAllocNum			一次分配的节点数目
          pDataFreeFun			节点释放函数
//Output:
//Return:
          返回值				说明
          0					成功
          负值				失败
//Others:
1)	节点最大数目应当为一次分配的节点数目的整数倍；
//Author:	fanyh	Email: fanyh@zhongsou.com
//Date:		2008-06-16
**************************************************************************/
CMiniCache *
CMiniCache_alloc(size_t lHashSize, size_t lMaxDataSize, size_t lMaxDataNum, size_t lOneAllocNum,
		 int  (*pDataFreeFun)(void*),
		 void (*pDataRefFun)(void*))
{
  CMiniCache *cache = NULL;
  if(lHashSize <= 0 || lMaxDataNum <= 0 || lOneAllocNum <= 0) return NULL;

  cache = calloc(1, sizeof(CMiniCache));
  pthread_mutex_init(&cache->lock, NULL);

  cache->m_lHashSize    = lHashSize;
  cache->m_lMaxDataSize = lMaxDataSize;
  cache->m_lMaxDataNum  = lMaxDataNum;
  cache->m_lOneAllocNum = lOneAllocNum;
  cache->m_pDataFreeFun = pDataFreeFun;
  cache->m_pDataRefFun  = pDataRefFun;
  cache->m_lAlloc = cache->m_lMaxDataNum/cache->m_lOneAllocNum;

  cache->m_ppNode = calloc(cache->m_lAlloc, sizeof(CMiniCacheNode *));
  if(!cache->m_ppNode) { FreeSource(cache); return NULL; }

  cache->m_lNowAllocPos = 0;
  cache->m_ppNode[cache->m_lNowAllocPos] = calloc(cache->m_lOneAllocNum, sizeof(CMiniCacheNode));
  if(!cache->m_ppNode[cache->m_lNowAllocPos]) { FreeSource(cache); return NULL; }

  cache->m_lNowDataPos = 0;

  cache->m_ppHash = calloc(cache->m_lHashSize, sizeof(CMiniCacheNode *));
  if(!cache->m_ppHash) { FreeSource(cache); return NULL; }
  return cache;
}

void
CMiniCache_free(CMiniCache *cache)
{
  FreeSource(cache);
}


/**************************************************************************
	//Function:			GetCacheNode
	//Description:		获取一个Cache节点
	//Calls:
	//Called by:
	//Input:
	//Output:
						pNode				指向可用节点的指针
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
static inline CMiniCacheNode *
GetCacheNodeFromIdleList(CMiniCache *cache)
{
  CMiniCacheNode *n = NULL;
  if (cache && cache->m_pIdleNode) {
    n = cache->m_pIdleNode;
    cache->m_pIdleNode = cache->m_pIdleNode->pDown;
    memset(n, 0, sizeof(CMiniCacheNode));
  }
  return n;
}
int
GetCacheNode(CMiniCache *cache, CMiniCacheNode **pNode)
{
  pthread_mutex_lock(&cache->lock);

  //检查当前内存使用情况
  while(cache->m_pHead && cache->lDataSize > cache->m_lMaxDataSize)
    DelNode(cache, cache->m_pHead, 1);

  if(cache->m_pIdleNode) {
    //在空闲节点链表中获取
    *pNode = GetCacheNodeFromIdleList(cache);
    pthread_mutex_unlock(&cache->lock);
    return 0;
  } else {
    if(cache->m_lNowDataPos>=cache->m_lOneAllocNum) {
      //当前块已分配完
      cache->m_lNowAllocPos++;
      if(cache->m_lNowAllocPos >= cache->m_lAlloc) {
	//已超过设定的最大可用节点数
	DelNode(cache, cache->m_pHead, 1);
	*pNode = GetCacheNodeFromIdleList(cache);
	pthread_mutex_unlock(&cache->lock);
	return 0;
      }
      cache->m_ppNode[cache->m_lNowAllocPos] = calloc(cache->m_lOneAllocNum, sizeof(CMiniCacheNode));
      if(!cache->m_ppNode[cache->m_lNowAllocPos]) {
	//操作系统暂无内存可用
	cache->m_lLastErrorCode = PROJECTBASEERRORCODE_MINICACHE+42;
	DelNode(cache, cache->m_pHead, 1);
	*pNode = GetCacheNodeFromIdleList(cache);
	cache->m_lNowAllocPos--;
	pthread_mutex_unlock(&cache->lock);
	return 0;
      }
      cache->m_lNowDataPos = 0;
    }
    *pNode = &cache->m_ppNode[cache->m_lNowAllocPos][cache->m_lNowDataPos++];
  }
  memset(*pNode,0,sizeof(CMiniCacheNode));
  pthread_mutex_unlock(&cache->lock);
  return 0;
}

static inline size_t
hashVal(map_key_t key, size_t size)
{
  size_t ret = 0;
  ret = *(size_t*)(key->digest);
  return ret % size;
}

/**************************************************************************
  //Function:			__AddNode
  //Description:		加入关键字节点
  //Calls:
  //Called by:
  //Input:
            pNode				指向待加入节点的指针
  //Output:

  //Return:
            返回值				说明
            0					成功
            负值				失败
  //Others:
  //Author:	fanyh	Email: fanyh@zhongsou.com
  //Date:		2008-06-16
  **************************************************************************/
static inline int
__AddNode(CMiniCache *cache, CMiniCacheNode* pNode, bool readd)
{
  size_t lPos;
  lPos = hashVal(pNode->tKey, cache->m_lHashSize);

  //放到hash位置
  if(cache->m_ppHash[lPos]) {
    pNode->pDown = cache->m_ppHash[lPos];
    pNode->pUp = NULL;
    cache->m_ppHash[lPos]->pUp = pNode;
    cache->m_ppHash[lPos] = pNode;
  } else {
    pNode->pDown = NULL;
    pNode->pUp = NULL;
    cache->m_ppHash[lPos] = pNode;
  }

  //使用链表，left是更加新的
  if(cache->m_pEnd) {
    pNode->pRight = cache->m_pEnd;
    pNode->pLeft = NULL;
    cache->m_pEnd->pLeft = pNode;
    cache->m_pEnd = pNode;
  } else {
    pNode->pRight = NULL;
    pNode->pLeft = NULL;
    cache->m_pEnd = pNode;
  }

  if(!cache->m_pHead)
    cache->m_pHead = pNode;
  if (!readd) {
    cache->lDataSize += pNode->lDataSize;
    cache->lDataNum  += 1;
  }
  return 0;
}

/**************************************************************************
	//Function:			AddNode
	//Description:		新加入一个节点
	//Calls:
	//Called by:
	//Input:
						pNode				指向加入节点的指针
	//Output:
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
AddNode(CMiniCache *cache, CMiniCacheNode *pNode)
{
  pthread_mutex_lock(&cache->lock);

  CMiniCacheNode *pOldNode;
  if(0==FindNode(cache, pNode->tKey, &pOldNode)) {
    DelNode(cache, pOldNode, 1);
  }
  __AddNode(cache, pNode, false);
  if (cache->m_pDataFreeFun)
    cache->m_pDataRefFun(pNode->vpData);

  pthread_mutex_unlock(&cache->lock);

  return 0;
}

/**************************************************************************
	//Function:			GetData
	//Description:		获取节点数据
	//Calls:
	//Called by:
	//Input:
						tKey				记录关键字
	//Output:
						vpData				关键字的数据域
						lDataSize			数据域长度
						lTime				数据域失效时间
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
GetData(CMiniCache *cache, map_key_t tKey,
        void** vpData, size_t* lDataSize)
{
  int ret = -1;
  pthread_mutex_lock(&cache->lock);

  CMiniCacheNode *pOldNode;
  if(0==FindNode(cache, tKey, &pOldNode)) {
    *vpData = pOldNode->vpData;
    *lDataSize = pOldNode->lDataSize;
    DelNode(cache, pOldNode, 0);
    __AddNode(cache, pOldNode, true);
    if (cache->m_pDataRefFun)
      cache->m_pDataRefFun(*vpData);
    ret = 0;
  }

  pthread_mutex_unlock(&cache->lock);
  return ret;
}

/**************************************************************************
	//Function:			AddData
	//Description:		加入一条记录
	//Calls:
	//Called by:
	//Input:
						tKey				记录关键字
						vpData				关键字的数据域
						lDataSize			数据域长度
						lTime				数据域失效时间
	//Output:
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
AddData(CMiniCache *cache, map_key_t tKey,
        void* vpData,size_t lDataSize)
{
  CMiniCacheNode *pNode = NULL;
  GetCacheNode(cache, &pNode);
  if (pNode==NULL) return -1;
  memcpy(&pNode->key, tKey, sizeof(md5_digest_t));
  pNode->tKey = &pNode->key;
  pNode->vpData = vpData;
  pNode->lDataSize = lDataSize;
  AddNode(cache, pNode);
  return 0;
}

/**************************************************************************
	//Function:			DelData
	//Description:		根据关键字删除一条记录
	//Calls:
	//Called by:
	//Input:
						tKey				记录关键字
	//Output:
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
DelData(CMiniCache *cache, map_key_t tKey)
{
  pthread_mutex_lock(&cache->lock);

  CMiniCacheNode *pOldNode = NULL;
  if(0==FindNode(cache, tKey, &pOldNode)) {
    DelNode(cache, pOldNode, 1);
  }

  pthread_mutex_unlock(&cache->lock);
  return 0;
}

/**************************************************************************
	//Function:			GetLastError
	//Description:		释放指定内存区
	//Calls:
	//Called by:
	//Input:
	//Output:
						lErrorCode			最后发生错误的代码
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
GetLastError(CMiniCache *cache, long* lErrorCode)
{
  pthread_mutex_lock(&cache->lock);

  *lErrorCode = cache->m_lLastErrorCode;

  pthread_mutex_unlock(&cache->lock);
  return 0;
}

/**************************************************************************
	//Function:			SelfDelUnvalidNode
	//Description:		自淘汰失效节点
	//Calls:
	//Called by:
	//Input:
	//Output:
	//Return:
						返回值				说明
						0					成功
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-19
	**************************************************************************/
int
SelfDelUnvalidNode(CMiniCache *cache)
{
  cache->m_lRunFlag = 1;
  for(;cache->m_lRunFlag;) {
   pthread_mutex_lock(&cache->lock);

    while(cache->m_pHead && cache->lDataSize > cache->m_lMaxDataSize) {
      DelNode(cache, cache->m_pHead, 1);
    }

    pthread_mutex_unlock(&cache->lock);
    usleep(10000);
  }
  return 0;
}

/**************************************************************************
	//Function:			StopSelfDel
	//Description:		停止自淘汰
	//Calls:
	//Called by:
	//Input:
	//Output:
	//Return:
						返回值				说明
						0					成功
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-19
	**************************************************************************/
int
StopSelfDel(CMiniCache *cache)
{
  cache->m_lRunFlag = 0;
  return 0;
}

/**************************************************************************
	//Function:			DelNode
	//Description:		删除最早加入的节点
	//Calls:
	//Called by:
	//Input:
	//Output:
	//Return:
						返回值				说明
						0					成功
	//Others:
			删除的节点并非总是最早加入的
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-07-01
	*************************************************************************
int
DelNode1(CMiniCache *cache)
{
  pthread_mutex_lock(&cache->lock);

  DelNode(cache, cache->m_pHead, 1);

  pthread_mutex_unlock(&cache->lock);
  return 0;
}*/


/**************************************************************************
	//Function:			DelNode
	//Description:		删除节点
	//Calls:
	//Called by:
	//Input:
						pNode				指向待删除节点的指针
						lFlag				1表示删除且需要释放内存 0表示删除但不需要释放内存
	//Output:
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
DelNode(CMiniCache *cache, CMiniCacheNode *pNode, long lFlag)
{
  if(!pNode) return -1;

  if(pNode->pLeft && pNode->pRight) {
    pNode->pLeft->pRight = pNode->pRight;
    pNode->pRight->pLeft = pNode->pLeft;
  } else {
    if(NULL == pNode->pRight) {
      //m_pHead
      cache->m_pHead = pNode->pLeft;
      if(cache->m_pHead)
        cache->m_pHead->pRight = NULL;
    }
    if(NULL == pNode->pLeft) {
      //m_pEnd
      cache->m_pEnd = pNode->pRight;
      if(cache->m_pEnd)
        cache->m_pEnd->pLeft = NULL;
    }
  }

  size_t pos = hashVal(pNode->tKey, cache->m_lHashSize);
  if(pNode == cache->m_ppHash[pos]) {
    cache->m_ppHash[pos] = pNode->pDown;
    if(pNode->pDown)
      pNode->pDown->pUp = NULL;
  } else {
    if(pNode->pUp)
      pNode->pUp->pDown = pNode->pDown;
    if(pNode->pDown)
      pNode->pDown->pUp = pNode->pUp;
  }

  if(lFlag) {
    if(cache->m_pDataFreeFun && pNode->vpData) {
      while(cache->m_pDataFreeFun(pNode->vpData)) usleep(100);
      pNode->vpData = NULL;
      cache->lDataSize -= pNode->lDataSize;
      cache->lDataNum  -= 1;
    }
    pNode->pDown = cache->m_pIdleNode;
    cache->m_pIdleNode = pNode;
  }
  return 0;
}

/**************************************************************************
	//Function:			FindNode
	//Description:		根据关键字查找节点
	//Calls:
	//Called by:
	//Input:
						tKey				记录主关键字
	//Output:
						pNode				指向查找到节点的指针
	//Return:
						返回值				说明
						0					成功
						负值				失败
	//Others:
	//Author:	fanyh	Email: fanyh@zhongsou.com
	//Date:		2008-06-16
	**************************************************************************/
int
FindNode(CMiniCache *cache, map_key_t tKey, CMiniCacheNode **pOldNode)
{
  size_t lPos;
  if (!pOldNode) return -1;

  lPos = hashVal(tKey, cache->m_lHashSize);

  if(cache->m_ppHash[lPos]) {
    *pOldNode = cache->m_ppHash[lPos];
    while(*pOldNode) {
      if(md5_compare((*pOldNode)->tKey->digest, tKey->digest) == 0)
        return 0;
      else
	*pOldNode = (*pOldNode)->pDown;
    }
  }
  return -1;
}

static int exportIdx=0;

void
exportHashCount(CMiniCache *cache)
{
  int *count=NULL;
  size_t pos, total=0;
  
  count= calloc(cache->m_lHashSize, sizeof(int));
  if (!count) return;
  pthread_mutex_lock(&cache->lock);
  for (pos=0;pos<cache->m_lHashSize;pos++) {
    CMiniCacheNode * n = cache->m_ppHash[pos];
    while(n) {
      count[pos]++;
      total++;
      n=n->pDown;
    }
  }
  pthread_mutex_unlock(&cache->lock);

  char fn[256];
  memset(fn, 0, 256);
  snprintf(fn, 255, "/home/fcache/export/%d", exportIdx++);
  FILE *out = fopen(fn, "w");
  for(pos=0;pos<cache->m_lHashSize;pos++){
    if(count[pos]>0)
      fprintf(out, "%lu: %d\n", pos, count[pos]);
  }
  free(count);
  fprintf(out, "recored total num: %lu, count total: %lu\n", cache->lDataNum, total);
  fclose(out);
}

void
exportStats(CMiniCache *cache, char *dest, size_t len)
{
  if (cache && dest)
    snprintf(dest, len, "CMiniCache{max=%lu, num=%lu, curr=%lu}", cache->m_lMaxDataSize, cache->lDataNum, cache->lDataSize);
  //  if (cache) exportHashCount(cache);
}
