#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "zhongsou_page_save.h"
#include "settings.h"
#include "thread.h"
#include "log.h"
#include "be_read.h"

static void handle_page_save_byid(uint64_t, uint64_t);
/********************************************************************************
 * part I  : listen event
 ********************************************************************************/
static int listen_socket;

/**
 * {
 *    's', 'a', 'v', 'e', 'p', 'g', '\0', page_number:uint8_t(0<=n<=150), //8byte
 *    save_time:uint64_t                                                  //8byte
 *    page_number*(page_id:uint64_t)                                      //8byte * page_number
 * }
 */
static void *
handle_save_message(void *args)
{
#define BUFLEN 1400
  char buf[BUFLEN];
  struct sockaddr_in si_other;
  int i;
  socklen_t slen=sizeof(si_other);
  ssize_t len;

  tlog(DEBUG, "save server @%d start", cfg.save_server.port);
  while(true)
    if ((len=recvfrom(listen_socket, buf, BUFLEN, 0,
		      (struct sockaddr *)&si_other, &slen))!=-1) {
      tlog(DEBUG, "Received save packet from %s:%d\nData(%d): %s\n\n",
           inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), len, buf);
      if (len>=16
	  && buf[0]=='s'
	  && buf[1]=='a'
	  && buf[2]=='v'
	  && buf[3]=='e'
	  && buf[4]=='p'
	  && buf[5]=='g'
	  && buf[6]=='\0') {
	uint8_t page_num;
	uint64_t save_time, page_id;
	stream_t st = {7, len, (uint8_t *)buf};

	page_num  = readu8(&st);
	save_time = readu64(&st);
	for (i=0; i<page_num && st.pos+8<=st.len; i++) {
	  page_id = readu64(&st);
	  tlog(DEBUG, "save page: time=%llu, page=%llu", save_time, page_id);
	  handle_page_save_byid(page_id, save_time);	  
	}
      }
    }
  tlog(ERROR, "udp server @%d shutdown", cfg.udp_server.port);
#undef BUFLEN
}

void
wait_page_save_message(void)
{
  struct sockaddr_in si_me;

  page_save_time(0);
  if ((listen_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    perror("save listen: socket");

  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(cfg.save_server.port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(listen_socket, (const struct sockaddr *)&si_me, sizeof(si_me))==-1)
    perror("save listen: bind");

  create_worker(handle_save_message, NULL);
}


/********************************************************************************
 * part II : store save event
 ********************************************************************************/


typedef struct page_save_node_s page_save_node_t;
typedef struct page_save_s      page_save_t;

struct page_save_node_s {
  uint64_t page_id;
  uint64_t save_time;

  page_save_node_t *pUp,   *pDown;
  page_save_node_t *pLeft, *pRight;
};
struct page_save_s {
  size_t m_lMaxDataNum;        //最大节点数
  size_t m_lOneAllocNum;       //一次分配的节点数
  size_t m_lNowAllocPos;       //当前已分配的节点块数
  size_t m_lAlloc;             //最大可分配块数
  size_t m_lNowDataPos;        //当前块已分配节点数（顺序分配）
  page_save_node_t **m_ppNode;   //预分配节点
  page_save_node_t *m_pIdleNode; //空闲链表头指针
  page_save_node_t *m_pEnd;      //数据使用时间链表尾指针
  page_save_node_t *m_pHead;     //数据使用时间链表头指针

  size_t m_lHashSize;          //哈希空间大小
  page_save_node_t **m_ppHash;   //哈希表
  size_t lDataNum;             //实际数据总数

  pthread_mutex_t lock;     //多线程下使用临界
};

static inline int
FreeSource(page_save_t *ps)
{
  if (ps == NULL) return 0;
  size_t i;
  for(i=0; i<ps->m_lNowAllocPos; i++) {
    if(ps->m_ppNode[i]) {
      free(ps->m_ppNode[i]);
      ps->m_ppNode[i] = NULL;
    }
  }
  if(ps->m_ppNode) {
    free(ps->m_ppNode);
    ps->m_ppNode = NULL;
  }
  if(ps->m_ppHash) {
    free(ps->m_ppHash);
    ps->m_ppHash = NULL;
  }
  pthread_mutex_destroy(&ps->lock);
  free(ps);
  return 0;
}

page_save_t *
page_save_alloc(size_t lHashSize, size_t lMaxDataNum, size_t lOneAllocNum)
{
  page_save_t *ps = NULL;
  if(lHashSize <= 0 || lMaxDataNum <= 0 || lOneAllocNum <= 0) return NULL;

  ps = calloc(1, sizeof(page_save_t));
  pthread_mutex_init(&ps->lock, NULL);
  ps->m_lHashSize    = lHashSize;
  ps->m_lMaxDataNum  = lMaxDataNum;
  ps->m_lOneAllocNum = lOneAllocNum;
  ps->m_lAlloc = ps->m_lMaxDataNum/ps->m_lOneAllocNum;

  ps->m_ppNode = calloc(ps->m_lAlloc, sizeof(page_save_node_t *));
  if(!ps->m_ppNode) { FreeSource(ps); return NULL; }

  ps->m_lNowAllocPos = 0;
  ps->m_ppNode[ps->m_lNowAllocPos] = calloc(ps->m_lOneAllocNum, sizeof(page_save_node_t));
  if(!ps->m_ppNode[ps->m_lNowAllocPos]) { FreeSource(ps); return NULL; }

  ps->m_lNowDataPos = 0;

  ps->m_ppHash = calloc(ps->m_lHashSize, sizeof(page_save_node_t *));
  if(!ps->m_ppHash) { FreeSource(ps); return NULL; }
  return ps;
}

void
page_save_free(page_save_t *ps)
{
  FreeSource(ps);
}


static inline page_save_node_t *
GetCacheNodeFromIdleList(page_save_t *ps)
{
  page_save_node_t *n = NULL;
  if (ps && ps->m_pIdleNode) {
    n = ps->m_pIdleNode;
    ps->m_pIdleNode = ps->m_pIdleNode->pDown;
    memset(n, 0, sizeof(page_save_node_t));
  }
  return n;
}
int
GetCacheNode(page_save_t *ps, page_save_node_t **pNode)
{
  pthread_mutex_lock(&ps->lock);

  if(ps->m_pIdleNode) {
    //在空闲节点链表中获取
    *pNode = GetCacheNodeFromIdleList(ps);
    pthread_mutex_unlock(&ps->lock);
    return 0;
  } else {
    if(ps->m_lNowDataPos>=ps->m_lOneAllocNum) {
      //当前块已分配完
      ps->m_lNowAllocPos++;
      if(ps->m_lNowAllocPos >= ps->m_lAlloc) {
	//已超过设定的最大可用节点数
	DelNode(ps, ps->m_pHead, 1);
	*pNode = GetCacheNodeFromIdleList(ps);
	pthread_mutex_unlock(&ps->lock);
	return 0;
      }
      ps->m_ppNode[ps->m_lNowAllocPos] = calloc(ps->m_lOneAllocNum, sizeof(page_save_node_t));
      if(!ps->m_ppNode[ps->m_lNowAllocPos]) {
	//操作系统暂无内存可用
	DelNode(ps, ps->m_pHead, 1);
	*pNode = GetCacheNodeFromIdleList(ps);
	ps->m_lNowAllocPos--;
	pthread_mutex_unlock(&ps->lock);
	return 0;
      }
      ps->m_lNowDataPos = 0;
    }
    *pNode = &ps->m_ppNode[ps->m_lNowAllocPos][ps->m_lNowDataPos++];
  }
  memset(*pNode,0,sizeof(page_save_node_t));
  pthread_mutex_unlock(&ps->lock);
  return 0;
}

static inline int
__AddNode(page_save_t *ps, page_save_node_t *pNode, bool readd)
{
  size_t lPos;
  lPos = pNode->page_id % ps->m_lHashSize;

  //放到hash位置
  if(ps->m_ppHash[lPos]) {
    pNode->pDown = ps->m_ppHash[lPos];
    pNode->pUp = NULL;
    ps->m_ppHash[lPos]->pUp = pNode;
    ps->m_ppHash[lPos] = pNode;
  } else {
    pNode->pDown = NULL;
    pNode->pUp = NULL;
    ps->m_ppHash[lPos] = pNode;
  }

  //使用链表，left是更加新的
  if(ps->m_pEnd) {
    pNode->pRight = ps->m_pEnd;
    pNode->pLeft = NULL;
    ps->m_pEnd->pLeft = pNode;
    ps->m_pEnd = pNode;
  } else {
    pNode->pRight = NULL;
    pNode->pLeft = NULL;
    ps->m_pEnd = pNode;
  }

  if(!ps->m_pHead)
    ps->m_pHead = pNode;
  if (!readd) {
    ps->lDataNum  += 1;
  }
  return 0;
}

int
AddNode(page_save_t *ps, page_save_node_t *pNode)
{
  pthread_mutex_lock(&ps->lock);

  page_save_node_t *pOldNode;
  if(0==FindNode(ps, pNode->page_id, &pOldNode)) {
    DelNode(ps, pOldNode, 1);
  }
  __AddNode(ps, pNode, false);

  pthread_mutex_unlock(&ps->lock);

  return 0;
}

int
DelNode(page_save_t *ps, page_save_node_t *pNode, long lFlag)
{
  if(!pNode) return -1;

  if(pNode->pLeft && pNode->pRight) {
    pNode->pLeft->pRight = pNode->pRight;
    pNode->pRight->pLeft = pNode->pLeft;
  } else {
    if(NULL == pNode->pRight) {
      //m_pHead
      ps->m_pHead = pNode->pLeft;
      if(ps->m_pHead)
        ps->m_pHead->pRight = NULL;
    }
    if(NULL == pNode->pLeft) {
      //m_pEnd
      ps->m_pEnd = pNode->pRight;
      if(ps->m_pEnd)
        ps->m_pEnd->pLeft = NULL;
    }
  }

  size_t pos = pNode->page_id % ps->m_lHashSize;
  if(pNode == ps->m_ppHash[pos]) {
    ps->m_ppHash[pos] = pNode->pDown;
    if(pNode->pDown)
      pNode->pDown->pUp = NULL;
  } else {
    if(pNode->pUp)
      pNode->pUp->pDown = pNode->pDown;
    if(pNode->pDown)
      pNode->pDown->pUp = pNode->pUp;
  }

  if(lFlag) {
    pNode->pDown = ps->m_pIdleNode;
    ps->m_pIdleNode = pNode;
  }
  return 0;
}

int
FindNode(page_save_t *ps, uint64_t page_id, page_save_node_t **pOldNode)
{
  size_t lPos;
  if (!pOldNode) return -1;

  lPos = page_id % ps->m_lHashSize;

  if(ps->m_ppHash[lPos]) {
    *pOldNode = ps->m_ppHash[lPos];
    while(*pOldNode) {
      if(page_id == (*pOldNode)->page_id)
        return 0;
      else
	*pOldNode = (*pOldNode)->pDown;
    }
  }
  return -1;
}

static page_save_t *ps_state = NULL;

// record page_id's save time
static void
handle_page_save_byid(uint64_t page_id, uint64_t save_time)
{
  page_save_t *ps = ps_state;
  if (ps == NULL) return;

  page_save_node_t *pNode = NULL;
  GetCacheNode(ps, &pNode);
  if (pNode==NULL) return;
  pNode->page_id = page_id;
  pNode->save_time = save_time;
  AddNode(ps, pNode);
}

// get page_id's save time
uint64_t
page_save_time(uint64_t page_id)
{
  uint64_t ret = UNKNOWN_SAVE_TIME;
  page_save_t *ps = ps_state;
  if (ps == NULL) {
    ps = page_save_alloc(5*cfg.maxpagesave, cfg.maxpagesave, 50000);
    ps_state = ps;
  }

  pthread_mutex_lock(&ps->lock);

  page_save_node_t *pOldNode;
  if(0==FindNode(ps, page_id, &pOldNode)) {
    ret = pOldNode->save_time;
    DelNode(ps, pOldNode, 0);
    __AddNode(ps, pOldNode, true);
  }

  pthread_mutex_unlock(&ps->lock);

  return ret;
}
