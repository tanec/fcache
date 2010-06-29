// ProBlockChange.cpp: implementation of the CProBlockChange class.
//
//////////////////////////////////////////////////////////////////////

#include "ProBlockChange.h"
//#include <assert.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProBlockChange::CProBlockChange(const char *szFileName)
{
  int iRet=ReadChangeFile(szFileName,szNoPageHaveCache,lNoPageHaveCache,
          szNoPageNoCache,lNoPageNoCache,
           szHavePageHaveCache,lHavePageHaveCache,
           szHavePageNoCache,lHavePageNoCache,
           szNoDownPageHaveCache,lNoDownPageHaveCache,
           szNoDownPageNoCache,lNoDownPageNoCache);

  //assert(iRet==0);
  ReadChangeFileResult = iRet;

}

CProBlockChange::~CProBlockChange()
{

}

long CProBlockChange::GetChangeTime(long lInTime,char *pOutTime)
{
  if(lInTime<=0 || pOutTime==NULL)
    return -1;

  //long lTmpLen=time(NULL)-lInTime;
  long lTmpLen = lInTime;

  if(lTmpLen>=86400)
  {
    int iDay=lTmpLen/86400;
    int iHour=lTmpLen%86400;

    lTmpLen=sprintf(pOutTime,"%d天",iDay);
  }
  else if(lTmpLen>=3600)
  {
    int iHour=lTmpLen/3600;
    int iMin=lTmpLen%3600;

    lTmpLen=sprintf(pOutTime,"%d小时",iHour);
  }
  else if(lTmpLen>=60 && lTmpLen<3600)
  {
    int iMin=lTmpLen/60;
    int iSec=lTmpLen%60;

    lTmpLen=sprintf(pOutTime,"%d分钟",iMin);
  }
  else
  {
    lTmpLen=sprintf(pOutTime,"%d秒",lTmpLen);
  }

  return lTmpLen;

}
int CProBlockChange::ReadChangeFile(const char *szFileName,
          char *pNoPageHaveCache/*原页面不存在,有cache*/,long &lNoPageHaveCache,
          char *pNoPageNoCache/*原页面不存在,没有cache*/,long &lNoPageNoCache,
            char *pHavePageHaveCache/*原页面存在,有cache*/,long &lHavePageHaveCache,
            char *pHavePageNoCache/*原页面存在,没有cache*/,long &lHavePageNoCache,
            char *pNoDownPageHaveCache/*原页面暂不可下载,有cache*/,long &lNoDownPageHaveCache,
            char *pNoDownPageNoCache/*原页面暂不可下载,没有cache*/,long &lNoDownPageNoCache)
{
  if(szFileName==NULL || pNoPageHaveCache==NULL || pNoPageNoCache==NULL || pHavePageHaveCache==NULL || pHavePageNoCache==NULL
    || pNoDownPageHaveCache==NULL || pNoDownPageNoCache==NULL)
    return -1;


  FILE *fp=fopen(szFileName,"r");
  if(fp==NULL)
    return -2;


  //原页面不存在,有cache
  lNoPageHaveCache=GetFileNR(fp,"#NoPageHaveCache",16,pNoPageHaveCache);
  if(lNoPageHaveCache<=0)
  {
    fclose(fp);
    return lNoPageHaveCache-10;
  }

  //原页面不存在,没有cache
  lNoPageNoCache=GetFileNR(fp,"#NoPageNoCache",14,pNoPageNoCache);
  if(lNoPageNoCache<=0)
  {
    fclose(fp);
    return lNoPageNoCache-10;
  }

  //原页面存在,有cache
  lHavePageHaveCache=GetFileNR(fp,"#HavePageHaveCache",18,pHavePageHaveCache);
  if(lHavePageHaveCache<=0)
  {
    fclose(fp);
    return lHavePageHaveCache-10;
  }

  //原页面存在,没有cache
  lHavePageNoCache=GetFileNR(fp,"#HavePageNoCache",16,pHavePageNoCache);
  if(lHavePageNoCache<=0)
  {
    fclose(fp);
    return lHavePageNoCache-10;
  }

  //原页面暂不可下载,有cache
  lNoDownPageHaveCache=GetFileNR(fp,"#NoDownPageHaveCache",20,pNoDownPageHaveCache);
  if(lNoDownPageHaveCache<=0)
  {
    fclose(fp);
    return lNoDownPageHaveCache-10;
  }

  //原页面暂不可下载,没有cache
  lNoDownPageNoCache=GetFileNR(fp,"#NoDownPageNoCache",18,pNoDownPageNoCache);
  if(lNoDownPageNoCache<=0)
  {
    fclose(fp);
    return lNoDownPageNoCache-10;
  }

  fclose(fp);
  return 0;
}

long CProBlockChange::GetFileNR(FILE *fp,const char *pKeyFlag,int iKeyFlagLen,char *pOutBuf)
{
  if(fp==NULL || pOutBuf==NULL || pKeyFlag==NULL || iKeyFlagLen<=0)
    return -1;

  char szTmp[1024];
  long lLen=0;

  while(!feof(fp))
  {
    if(fgets(szTmp,1024,fp)==NULL)
      return -3;

    //if(strnicmp(szTmp,pKeyFlag,iKeyFlagLen)!=0)
    if(strncmp(szTmp,pKeyFlag,iKeyFlagLen)!=0)
      continue;

    break;
  }

  while(!feof(fp))
  {
    if(fgets(szTmp,1024,fp)==NULL)
      return -3;

    //if(strnicmp(szTmp,pKeyFlag,iKeyFlagLen)==0)
    if(strncmp(szTmp,pKeyFlag,iKeyFlagLen)==0)
      break;

    lLen+=sprintf(pOutBuf+lLen,"%s",szTmp);
  }

  return lLen;
}

long CProBlockChange::GetNoPageHaveCache(char *pInTime,char *pInUrl,char *pOutBuf,const long lInBufLen)
{
  if(pInTime==NULL || pInUrl==NULL || pOutBuf==NULL || lInBufLen<=lNoPageHaveCache)
    return -1;

  char *pStr=pInUrl+8;
  while(*pStr && *pStr!='/' && *pStr!='\\')pStr++;

  char cOld=*pStr;

  long lLen=sprintf(pOutBuf,szNoPageHaveCache,pInTime,pInUrl,pInUrl);

  *pStr=cOld;

  return lLen;

}

long CProBlockChange::GetNoPageNoCache(char *pInUrl,char *pOutBuf,const long lInBufLen)
{
  if(pInUrl==NULL || pOutBuf==NULL || lInBufLen<=lNoPageNoCache)
    return -1;

  char *pStr=pInUrl+8;
  while(*pStr && *pStr!='/' && *pStr!='\\')pStr++;

  char cOld=*pStr;

  long lLen=sprintf(pOutBuf,szNoPageNoCache,pInUrl,pInUrl);

  *pStr=cOld;

  return lLen;
}

long CProBlockChange::GetHavePageHaveCache(char *pInTime,char *pInUrl,char *pOutBuf,const long lInBufLen)
{
  if(pInUrl==NULL ||pInTime==NULL || pOutBuf==NULL || lInBufLen<=lHavePageHaveCache)
    return -1;


  return sprintf(pOutBuf,szHavePageHaveCache,pInTime,pInUrl,pInUrl);
}

long CProBlockChange::GetHavePageNoCache(char *pInUrl,char *pOutBuf,const long lInBufLen)
{
  if(pInUrl==NULL || pOutBuf==NULL || lInBufLen<=lHavePageNoCache)
    return -1;


  return sprintf(pOutBuf,szHavePageNoCache,pInUrl,pInUrl);
}

long CProBlockChange::GetNoDownPageHaveCache(char *pInTime,char *pInUrl,char *pOutBuf,const long lInBufLen)
{
  if(pInTime==NULL || pInUrl==NULL || pOutBuf==NULL || lInBufLen<=lNoDownPageHaveCache)
    return -1;


  return sprintf(pOutBuf,szNoDownPageHaveCache,pInTime);
  //lNoDownPageNoCache
}

long CProBlockChange::GetNoDownPageNoCache(char *pInUrl,char *pOutBuf,const long lInBufLen)
{
  if(pInUrl==NULL || pOutBuf==NULL || lInBufLen<=lNoDownPageNoCache)
    return -1;


  return sprintf(pOutBuf,szNoDownPageNoCache);

}
