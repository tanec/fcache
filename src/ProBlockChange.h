// ProBlockChange.h: interface for the CProBlockChange class.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


class CProBlockChange
{
public:
  CProBlockChange(const char *szFileName);
  virtual ~CProBlockChange();

  long GetChangeTime(long lInTime,char *pOutTime);

  long GetNoPageHaveCache(char *pInTime,char *pInUrl,char *pOutBuf,const long lInBufLen);
  long GetNoPageNoCache(char *pInUrl,char *pOutBuf,const long lInBufLen);
  long GetHavePageHaveCache(char *pInTime,char *pInUrl,char *pOutBuf,const long lInBufLen);
  long GetHavePageNoCache(char *pInUrl,char *pOutBuf,const long lInBufLen);
  long GetNoDownPageHaveCache(char *pInTime,char *pInUrl,char *pOutBuf,const long lInBufLen);
  long GetNoDownPageNoCache(char *pInUrl,char *pOutBuf,const long lInBufLen);
  int ReadChangeFileResult;

  char szNoPageHaveCache[10240];
private:
  long GetFileNR(FILE *fp,const char *pKeyFlag,int iKeyFlagLen,char *pOutBuf);

  int ReadChangeFile(const char *szFileName,
          char *pNoPageHaveCache,long &lNoPageHaveCache,							//原页面不存在,有cache
          char *pNoPageNoCache,long &lNoPageNoCache,								//原页面不存在,没有cache
            char *pHavePageHaveCache,long &lHavePageHaveCache,						//原页面存在,有cache
            char *pHavePageNoCache,long &lHavePageNoCache,							//原页面存在,没有cache
            char *pNoDownPageHaveCache,long &lNoDownPageHaveCache,					//原页面暂不可下载,有cache
            char *pNoDownPageNoCache,long &lNoDownPageNoCache);						//原页面暂不可下载,没有cache


private:

  char szNoPageNoCache[10240];
  char szHavePageHaveCache[10240];
  char szHavePageNoCache[10240];
  char szNoDownPageHaveCache[10240];
  char szNoDownPageNoCache[10240];

  long lNoPageHaveCache,lNoPageNoCache,lHavePageHaveCache,lHavePageNoCache,lNoDownPageHaveCache,lNoDownPageNoCache;


};
