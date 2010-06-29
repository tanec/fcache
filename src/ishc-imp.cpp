#include <pthread.h>
#include <vector>
#include "ProBlockChange.h"
using namespace std;

#ifndef	  UINT64
#define   UINT64 unsigned long long
#endif

#define OK 1

/**全局变量定义**/
static bool				DEBUGFLAG=false;		   //SOCKET超时时间(单位：秒)
static unsigned int		SOCKETOUTTIME=10;		   //SOCKET超时时间(单位：秒)
static unsigned int		SERVERIPMAX=0;			   //当前IP总数
static bool				bInitFlag=false;		   //是否初始化
static char				widgetVersion[20];		   //是否打印信息
static char					I_MSP_CGIPATH[70];
static char					I_MSP_IMAGEPATH[70];
static char					I_MSP_CSSANDJSPATH[70];

struct CONNECTSERVERIPSTRUCT
{
  char strIP[16];			//服务器IP
  int	iPort;				//服务器PORT
  bool bConnect;			//是否能连接服务器

  pthread_mutex_t		pThreadMutex;	//线程读写锁

  CONNECTSERVERIPSTRUCT()
  {
    bConnect=true;
    pthread_mutex_init(&pThreadMutex,NULL);
  }

  ~CONNECTSERVERIPSTRUCT()
  {
    pthread_mutex_destroy(&pThreadMutex);
  }

  void ChangeIsConnectValue(bool bC)      //连接值增加
  {
    pthread_mutex_lock(&pThreadMutex);
    bConnect=bC;
    pthread_mutex_unlock(&pThreadMutex);
  }
};

//url 参数
struct URLPARAMETER
{
  int s;    //0 无 : isch, 1:imsp
  int t;    //块版本
  int ds;   //0 无：取imsp剪切页的源码，1：取imsp的框架
  int f;    //刷新等级
  int a;	  //1：新加块，0 无：已有块
  char b[50];	  //ishc 块id
  char u[256];   //url
};

CONNECTSERVERIPSTRUCT *pServerIP = NULL;
CProBlockChange *proBlockChange = NULL;
/**功能函数定义**/
//读取配置文件
int  ReadConfigFile(const char *filename);//读取配置文件
//去掉空格、回车、换行符
int	 CancelEnter(char *p);
//创建socket连接
int  CreateSocket(const char *sServerIP,const int nPort,const int lTimeOut=10);
//随机ip连接服务器
int RandConnectServer(int &iIpIndex,const int nMaxIP,CONNECTSERVERIPSTRUCT *pServerIP,bool *ipS);
//获取一个随机数
int GetRandNum();
//URL解码
void UrlDecode(char *p);
int TwoHex2Int(char *pC);
//删除左右空格
int TrimLeft_RightBlank(char *strWordBuf);
//取数据
int GetMakeHtmlPage(const int pSocket,URLPARAMETER *para,char *&pBuf,char *err);
//获取参数
int fScanStr(char *poRes,int iResLen,char *piScr,char *piMark);
//填充url参数
int SetUrlParameters(URLPARAMETER * &para,char *pReg);
//打印imsp框架
void I_MSP_PrintfHtmlPage(char *strUrl);
//打印imsp错误页
void I_MSP_PrintfErrorHtmlPage(char *strMsg,char *strUrl);
//打印ishc错误页
void I_SHC_36_PrintfErrorHtmlPage(char *strMsg,int iRet,char* errcode);
int GetUrlHost(char *url,char * &host);

size_t WriteFile( const void *buf, size_t itemSize, size_t items, FILE *fd );
/**主函数**/
static int i_shc_handler()
{
  int ret = -1;

  if(!bInitFlag)
  {
    srand(time(NULL));
    ret=ReadConfigFile("/zhongsou/i/a/myig_module.ini");

    if(ret==0)
    {
      sprintf(I_MSP_CGIPATH,"/a/");
      sprintf(I_MSP_IMAGEPATH,"http://i0.zhongso.com/a/img_css/");
      sprintf(I_MSP_CSSANDJSPATH,"/a/img_css/");

      bInitFlag=true;
    }
    else
    {
      fprintf(stderr,"<!--不能读取配置文件-->");
      return ret;
    }

    proBlockChange = new CProBlockChange("/zhongsou/i/a/blockchange.dat");
    if(proBlockChange -> ReadChangeFileResult != 0)
    {
      fprintf(stderr,"<!--初始化错误块配置文件失败-->");
      //return OK;
    }
  }

  URLPARAMETER *para = new URLPARAMETER;
  ret = SetUrlParameters(para,r->args);
  if(ret < 0)
  {
    fprintf(stderr,"<!--读取参数失败-->");
    return OK;
  }
  //imsp框架
  if(para->s == 1 && para->ds == 1)
  {
    I_MSP_PrintfHtmlPage(r,para->u);
    return OK;
  }

  //创建socket
  int connIpIndex = 0;
  bool *ipStatus = new bool[SERVERIPMAX];
  for(int i=0;i<SERVERIPMAX;i++)
  {
    ipStatus[i] = true;
  }

  char strTmp[500]={0};

  if(SERVERIPMAX > 1)
  {
    connIpIndex = GetRandNum()%SERVERIPMAX;
  }
  int pSocket = RandConnectServer(r,connIpIndex,SERVERIPMAX,pServerIP,ipStatus);

  /**debug info**/
  if(DEBUGFLAG)
  {
    fprintf(stderr,"pSocket:%d\n",pSocket);
  }
  /**debug info**/

  if(pSocket<0)
  {
    sprintf(strTmp,"此时网络繁忙,请重试!!!\n");
    I_MSP_PrintfErrorHtmlPage(r,strTmp,para->u);
    return OK;
  }

  char *pBuf=NULL;
  char errorStr[256];
  bool reGetFlag = false;

  GETHTML:
  //通过协议取内容
  memset(errorStr,0,256);
  int iRet=GetMakeHtmlPage(pSocket,para,pBuf,r,errorStr);

  //打印信息
  if(para->s == 1 || (para->s != 1 && para->a == 1))//imsp 或 创建块
  {
    if(iRet > -10000 && iRet < -5000)
    {
      if(iRet == -5001 || iRet == -5002)
      {
        ipStatus[connIpIndex] = false;
        if(SERVERIPMAX > 1)
        {
          connIpIndex = GetRandNum()%SERVERIPMAX;
          close(pSocket);
          pSocket = RandConnectServer(r,connIpIndex,SERVERIPMAX,pServerIP,ipStatus);
          if(pSocket<0)
          {
            sprintf(strTmp,"re1-此时网络繁忙，请重试!!!\n");
            I_MSP_PrintfErrorHtmlPage(r,strTmp,para->u);

            return OK;
          }
          goto GETHTML;
        }
      }

      sprintf(strTmp,"%s，请重试(%d)。",errorStr,iRet);
      I_MSP_PrintfErrorHtmlPage(r,strTmp,para->u);
    }
    else if(iRet>0)
    {
      int dataLenPos;
      /*if(para->a == 1)
      {
        dataLenPos = 10 + sizeof(long)*4 + sizeof(char)*2;
        unsigned char md5Len[10] = {0};
        memcpy(md5Len,pBuf + dataLenPos,8);
        char outMd5[20] = {0};

        for(int i=0;i<8;i++)
          sprintf(outMd5+i*2,"%02x",md5Len[i]);
        fprintf(stderr,"%s\n",outMd5);

      }
      else*/
      {

        if(para->s == 1)
        {
          dataLenPos = 10 + sizeof(long)*8 + sizeof(char)*2;
        }
        else
        {
          dataLenPos = 10 + sizeof(long)*10 + sizeof(char)*3 + sizeof(short)*2 + strlen(para->u);
        }
        /**debug info**/
        if(DEBUGFLAG)
        {
          fprintf(stderr,"dataLenPos:%d\n",dataLenPos);
        }
        /**debug info**/

        char strDataLen[10] = {0};
        memcpy(strDataLen,pBuf + dataLenPos,4);
        long dataLen = (long)*(long*)strDataLen;
        if(dataLen > 0)
        {   dataLenPos += 4;
          ap_rputs(pBuf+dataLenPos,r);
        }
        else
        {
          sprintf(strTmp,"取数据失败<!--(%ld)-->！！！",dataLen);
          I_MSP_PrintfErrorHtmlPage(r,strTmp,para->u);
        }
      }
    }
    else
    {	if(para->a == 1)
      {
        fprintf(stderr,"<!--%s-->\n",errorStr);
      }
      else
      {
        sprintf(strTmp,"没有找到：%s 对应的网页，请确认该URL是否正确！！！,%s",para->u,errorStr);
        I_MSP_PrintfErrorHtmlPage(r,strTmp,para->u);
      }
    }

  }
  else // 打印块检索内容
  {

    if(iRet==-5001)
    {
      sprintf(strTmp,"因为您当前访问的网页超时，请重试。\n");
      I_SHC_36_PrintfErrorHtmlPage(r,strTmp,iRet,errorStr);
    }
    else if(iRet>0)
    {
      int dataLenPos = 10 + sizeof(long)*8 + sizeof(short)*4 + sizeof(char)*2 + strlen(para->u);
      char strDataLen[10] = {0};
      memcpy(strDataLen,pBuf + dataLenPos,4);
      long statusCode = (long)*(long*)strDataLen;
      dataLenPos += sizeof(long);
      memcpy(strDataLen,pBuf + dataLenPos,4);
      long errorCodeTime = (long)*(long*)strDataLen;
      dataLenPos += sizeof(long);
      memcpy(strDataLen,pBuf + dataLenPos,4);
      long errorCode = (long)*(long*)strDataLen;
      dataLenPos += sizeof(long);

      if(para->f > 1 && errorCode < 0)
      {

        if(pBuf!=NULL)
        {
          delete[] pBuf;
          pBuf=NULL;
        }
        reGetFlag = true;
        para->f = 0;
        /**debug info**/
        if(DEBUGFLAG)
        {
          fprintf(stderr,"para->f > 2 :errorCode:%ld\n",errorCode);
        }
        /**debug info**/
        close(pSocket);
        pSocket = RandConnectServer(r,connIpIndex,SERVERIPMAX,pServerIP,ipStatus);

        /**debug info**/
        if(DEBUGFLAG)
        {
          fprintf(stderr,"re2-pSocket:%d\n",pSocket);
        }
        /**debug info**/

        if(pSocket<0)
        {
          sprintf(strTmp,"re2-不能连接所有服务器，请重试!!!\n");
          I_MSP_PrintfErrorHtmlPage(r,strTmp,para->u);

          return OK;
        }
        goto GETHTML;
      }
      /**debug info**/
      if(DEBUGFLAG)
      {
        fprintf(stderr,"errorCode:%ld -- statusCode:%ld\n",errorCode,statusCode);
      }
      /**debug info**/
      if(statusCode == 0) // 正常取回数据
      {
        if(errorCode > 0)
        {
          fprintf(stderr,"%s\n",pBuf+dataLenPos);
          char statusStr[50];
          sprintf(statusStr,"<!--errorCode:%ld,statusCode:%ld-->",errorCode,statusCode);
          fprintf(stderr,"%s",statusStr);
        }
        else
        {
          I_SHC_36_PrintfErrorHtmlPage(r,"取得数据模块失败！！",errorCode,errorStr);
        }

      }
      else
      {
        //CProBlockChange *proBlockChange = new CProBlockChange("/zhongsou/i/a/blockchange.dat");
        char outBuf[12000] = {0};
        char inTime[50] = {0};
        char *urlHost = new char[50];
        int ghr = GetUrlHost(para->u,urlHost);

        proBlockChange -> GetChangeTime(errorCodeTime,inTime);

        if(errorCode > 0)
        {
          if(statusCode == 1)
          {
            proBlockChange->GetHavePageHaveCache(inTime,para->u,outBuf,12000);
          }
          else if(statusCode == 2)
          {
            if(ghr != 0)
            {
              proBlockChange->GetNoPageHaveCache(inTime,para->u,outBuf,12000);
            }
            else
            {
              proBlockChange->GetNoPageHaveCache(inTime,urlHost,outBuf,12000);
            }
            fprintf(stderr,"<!--%d-->\n",ghr);
            fprintf(stderr,"<!--111:%s-->\n",proBlockChange -> szNoPageHaveCache);
            fprintf(stderr,"<!--222:%s-->\n",outBuf);
          }
          else if(statusCode == 3)
          {
            proBlockChange->GetNoDownPageHaveCache(inTime,para->u,outBuf,12000);
          }

          int indiLen = strlen(outBuf);
          //static long temLen = errorCode + indiLen;
          long temLen = errorCode + indiLen;

          if(para->f > 1)
          {
            fprintf(stderr,"%s",pBuf+dataLenPos);
          }
          else
          {
            char *merOutBuf = new char[temLen];
            memcpy(merOutBuf,pBuf+dataLenPos,errorCode);
            memcpy(merOutBuf+errorCode,outBuf,indiLen);
            *(merOutBuf+temLen) = 0;

            /**debug info**/
            if(DEBUGFLAG)
            {
              fprintf(stderr,"statusCode:%d--errorCode:%d--time:%ld--otime:%s\n",statusCode,errorCode,errorCodeTime,inTime);
            }
            /**debug info**/

            fprintf(stderr,"%s",merOutBuf);
            char statusStr[50];
            sprintf(statusStr,"<!--errorCode:%ld,statusCode:%ld-->",errorCode,statusCode);
            fprintf(stderr,"%s",statusStr);
            if(merOutBuf!=NULL)
            {
              delete[] merOutBuf;
              merOutBuf=NULL;
            }
          }
        }
        else
        {
          if(statusCode == 1)
          {
            proBlockChange -> GetHavePageNoCache(para->u,outBuf,12000);
          }
          else if(statusCode == 2)
          {
            if(ghr != 0)
            {
              proBlockChange -> GetNoPageNoCache(para->u,outBuf,12000);
            }
            else
            {
              proBlockChange -> GetNoPageNoCache(urlHost,outBuf,12000);
            }
          }
          else if(statusCode == 3)
          {
            proBlockChange -> GetNoDownPageNoCache(para->u,outBuf,12000);
          }
          fprintf(stderr,"%s",outBuf);
          char statusStr[50];
          sprintf(statusStr,"<!--errorCode:%ld,statusCode:%ld-->",errorCode,statusCode);
          fprintf(stderr,"%s",statusStr);
        }

        if(urlHost != NULL)
        {
          delete[] urlHost;
          urlHost=NULL;
        }

      }
    }
    else
    {
      I_SHC_36_PrintfErrorHtmlPage(r,"取得数据模块失败！！",iRet,errorStr);
    }
  }
  fprintf(stderr,"<!--connserver:%d-->\n",connIpIndex);
  for(int i=0;i<SERVERIPMAX;i++)
  {
    if(!ipStatus[i])
    {
      fprintf(stderr,"<!--errorserver:%d-->\n",i);
    }
  }

  if(pBuf!=NULL)
  {
    delete[] pBuf;
    pBuf=NULL;
  }
  if(ipStatus != NULL)
  {
    delete[] ipStatus;
    ipStatus=NULL;
  }

  close(pSocket);
  return OK;
}

//调用module接口
static handler_rec i_shc_handlers[]=
{
  {"i-shc-handler",i_shc_handler},
  {NULL}
};

//module 接口
module MODULE_VAR_EXPORT i_shc_module=
{
  STANDARD_MODULE_STUFF,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  i_shc_handlers,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};


/**函数实现**/
int GetMakeHtmlPage(const int pSocket,URLPARAMETER *para,char *&pBuf,request_rec *r,char *err)
{
  if(pSocket<0 || pBuf!=NULL)
    return -1;

  char idCode[11];
  int allRecvDataLength = 0;
  if(para->s == 1 || (para->s != 1 && para->a == 1))//imsp
  {
    sprintf(idCode,"CMSV1.0001");
    char sendDataBuf[1024]={0};
    int SendDataLength = 0;
    char sendChildDataBuf[512] = {0};
    int SendChildDataLength = 0;
    int temLen = 0;
    short urlLen = (short)strlen(para->u);

    //拼子数据协议
    if(para->s == 1)//预览
    {
      *(sendChildDataBuf+SendChildDataLength) = 2;
      SendChildDataLength += 1;
      *(short*)(sendChildDataBuf + SendChildDataLength) = urlLen;
      SendChildDataLength += sizeof(short);
      memcpy(sendChildDataBuf+SendChildDataLength,para->u,urlLen);
      SendChildDataLength += urlLen;
      *(sendChildDataBuf+SendChildDataLength) = 0;
      SendChildDataLength += 1;
    }
    else//物理块创建
    {

      /**debug info**/
      if(DEBUGFLAG)
      {
        fprintf(stderr,"para->b:%s\n",para->b);
      }
      /**debug info**/

      char *bBuf = para->b;
      char *pos;
      char ch;
      pos = strstr(bBuf,"_");
      ch = *pos;
      *pos = 0;
      char strFid[50];
      sprintf(strFid,"%s",bBuf);
      UINT64 fId = strtoull(bBuf,NULL,10);

      /**debug info**/
      if(DEBUGFLAG)
      {
        fprintf(stderr,"fId:%ld\n",fId);
      }
      /**debug info**/

      *pos = ch;
      bBuf = pos + 1;
      short bId;
      if(pos = strstr(bBuf,","))
      {
        ch = *pos;
        *pos = 0;
        bId = (short)atoi(bBuf);
        *pos = ch;
      }
      else
      {
        bId = (short)atoi(bBuf);
      }

      /**debug info**/
      if(DEBUGFLAG)
      {
        fprintf(stderr,"bId:%d\n",bId);
      }
      /**debug info**/

      long tempUrlLen = 44 + strlen(para->b) + strlen(para->u);
      char *tempUrl = new char[tempUrlLen];
      sprintf(tempUrl,"u=[%s]&f=%s&b=%d&v=%d",para->u,strFid,bId,para->t);
      tempUrlLen = strlen(tempUrl);

      /**debug info**/
      if(DEBUGFLAG)
      {
        fprintf(stderr,"tempUrl:%s\n",tempUrl);
      }
      /**debug info**/

      *(sendChildDataBuf+SendChildDataLength) = 2;
      SendChildDataLength += 1;
      *(long*)(sendChildDataBuf + SendChildDataLength) = tempUrlLen;//信息源url长度
      SendChildDataLength += sizeof(long);
      memcpy(sendChildDataBuf+SendChildDataLength,tempUrl,tempUrlLen);//信息源url
      SendChildDataLength += tempUrlLen;
      *(long*)(sendChildDataBuf + SendChildDataLength) = (long)0;//属性长度
      SendChildDataLength += sizeof(long);
      delete tempUrl;


      *(UINT64*)(sendChildDataBuf + SendChildDataLength) = fId;//formatid
      SendChildDataLength += sizeof(UINT64);
      *(short*)(sendChildDataBuf + SendChildDataLength) = bId;//blockid
      SendChildDataLength += sizeof(short);
      *(sendChildDataBuf+SendChildDataLength) = para->t;//块版本
      SendChildDataLength += 1;
      *(short*)(sendChildDataBuf + SendChildDataLength) = urlLen;//url长度
      SendChildDataLength += sizeof(short);
      memcpy(sendChildDataBuf+SendChildDataLength,para->u,urlLen);//url
      SendChildDataLength += urlLen;

    }

    //拼发送数据协议
    temLen = strlen(idCode);
    memcpy(sendDataBuf+SendDataLength,idCode,temLen);//验证码
    SendDataLength += temLen;

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"SendDataLength:%d\n",SendDataLength);
    }
    /**debug info**/

    *(long*)(sendDataBuf + SendDataLength) = (long)(sizeof(UINT64)+sizeof(char)+sizeof(long) + SendChildDataLength);
    SendDataLength += sizeof(long);
    *(UINT64*)(sendDataBuf + SendDataLength) = (UINT64)0;
    SendDataLength += sizeof(UINT64);
    if(para->s == 1)
    {
      *(sendDataBuf+SendDataLength) = 3;
    }
    else
    {
      *(sendDataBuf+SendDataLength) = 0;
    }
    SendDataLength += 1;
    *(long*)(sendDataBuf + SendDataLength) = (long)SendChildDataLength;
    SendDataLength += sizeof(long);
    memcpy(sendDataBuf+SendDataLength,sendChildDataBuf,SendChildDataLength);
    SendDataLength += SendChildDataLength;

    int nAllLen=0,nOneLen=0;

    /**debug info**/
    if(DEBUGFLAG)
    {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/debug.txt","wb");
      WriteFile(sendDataBuf,SendDataLength,1,pr);
      fclose(pr);
      fprintf(stderr,"SOCKETOUTTIME:%d\n",SOCKETOUTTIME);
    }
    /**debug info**/

    do
    {
      nOneLen=send(pSocket,sendDataBuf+nAllLen,SendDataLength-nAllLen,0);
      if(nOneLen>0)nAllLen+=nOneLen;
    }while(nOneLen>0 && nAllLen<SendDataLength);

    if(nAllLen!=SendDataLength)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"发送数据失败！");
      }
      return -5001;
    }
    //验证对方是否收到
    int verifyCodeLen = 8;
    char verifyCode[9];
    nAllLen=0;
    do
    {
      nOneLen=recv(pSocket,verifyCode+nAllLen,verifyCodeLen-nAllLen,0);
      if(nOneLen>0)
      {
        nAllLen+=nOneLen;
      }

    }while(nOneLen>0 && nAllLen<verifyCodeLen);

    if(nAllLen!=verifyCodeLen)
    {

      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误COMMLENGTH,vn:%d,rn:%d,rd:%s！",verifyCodeLen,nAllLen,verifyCode);
      }
      return -5002;
    }
    if(strncmp(verifyCode,"COMM",4) != 0)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"校验错误COMMSTR,code:%ld！",(long)*(long*)(verifyCode+4));
      }
      return -5003;
    }

    //验证数据的合法性
    int verifyDataLen=strlen(idCode) + sizeof(long);
    nAllLen=0;
    nOneLen = 0;
    do
    {
      nOneLen=recv(pSocket,sendDataBuf+nAllLen,verifyDataLen-nAllLen,0);

      /**debug info**/
      if(DEBUGFLAG)
      {
        fprintf(stderr,"nOneLen:%d\n",nOneLen);
      }
      /**debug info**/

      if(nOneLen>0)
      {
        nAllLen+=nOneLen;
      }

    }while(nOneLen>0 && nAllLen<verifyDataLen);

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"%d-%d\n",nAllLen,verifyDataLen);
    }
    /**debug info**/

    if(nAllLen!=verifyDataLen)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误VERIFYCODELENGTH,vn:%d,rn:%d,rd:%s！",verifyDataLen,nAllLen,sendDataBuf);
      }
      return -5004;
    }

    sendDataBuf[verifyDataLen]=0;

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"sendDataBuf:%s\n",sendDataBuf);
    }
    /**debug info**/

    char ch = *(sendDataBuf + temLen);
    *(sendDataBuf + temLen) = 0;
    if(strncmp(sendDataBuf,idCode,temLen) != 0)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"校验错误VERIFYCODESTR,code:%ld！",(long)*(long*)(sendDataBuf+temLen));
      }
      return -5005;
    }
    *(sendDataBuf + temLen) = ch;
    char *temBuf;
    temBuf = (sendDataBuf + temLen);
    allRecvDataLength = (long)*(long*)(temBuf);

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"allRecvDataLength:%d\n",allRecvDataLength);
    }
    /**debug info**/

    if(allRecvDataLength < 4)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误ALLDATALENGTH,code:%ld！",allRecvDataLength);
      }
      return -5006;
    }

    pBuf = new char[allRecvDataLength + verifyDataLen];

    if(pBuf ==	NULL)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"系统内部错误1！");
      }
      return -5007;
    }

    memcpy(pBuf,sendDataBuf,verifyDataLen);
    nAllLen=0;
    nOneLen=0;
    do
    {
      nOneLen=recv(pSocket,pBuf+verifyDataLen+nAllLen,allRecvDataLength-nAllLen,0);
      if(nOneLen>0)
      {
        nAllLen+=nOneLen;
      }

    }while(nOneLen>0 && nAllLen<allRecvDataLength);
    if(nAllLen!=allRecvDataLength)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误ALLRECVDATALENGTH,code:%ld！",nAllLen);
      }
      return -5008;
    }

    pBuf[nAllLen + verifyDataLen]=0;

    /**debug info**/
    if(DEBUGFLAG)
    {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/recv.txt","wb");
      WriteFile(pBuf,allRecvDataLength + verifyDataLen,1,pr);
      fclose(pr);
    }
    /**debug info**/

    return (allRecvDataLength + verifyDataLen);

  }
  else
  {

    sprintf(idCode,"CMSV1.0002");
    char sendDataBuf[1024]={0};
    int SendDataLength = 0;
    char sendChildDataBuf[512] = {0};
    int SendChildDataLength = 0;
    int temLen = 0;
    short urlLen = (short)strlen(para->u);

    //拼子数据协议
    *(sendChildDataBuf+SendChildDataLength) = 2;//块类型
    SendChildDataLength += 1;
    *(UINT64*)(sendChildDataBuf + SendChildDataLength) = (UINT64)0;//数据id
    SendChildDataLength += sizeof(UINT64);

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"para->b:%s\n",para->b);
    }
    /**debug info**/

    char *bBuf = para->b;
    char *pos;
    char ch;
    pos = strstr(bBuf,"_");
    ch = *pos;
    *pos = 0;
    UINT64 fId = strtoull(bBuf,NULL,10);
    *pos = ch;
    bBuf = pos + 1;
    short bId;
    if(pos = strstr(bBuf,","))
    {
      ch = *pos;
      *pos = 0;
      bId = (short)atoi(bBuf);
      *pos = ch;
    }
    else
    {
      bId = (short)atoi(bBuf);
    }

    *(UINT64*)(sendChildDataBuf + SendChildDataLength) = fId;//formatid
    SendChildDataLength += sizeof(UINT64);

    *(short*)(sendChildDataBuf + SendChildDataLength) = bId;//blockid
    SendChildDataLength += sizeof(short);
    *(sendChildDataBuf+SendChildDataLength) = para->t;//块版本
    SendChildDataLength += 1;
    *(short*)(sendChildDataBuf + SendChildDataLength) = urlLen;//url长度
    SendChildDataLength += sizeof(short);
    memcpy(sendChildDataBuf+SendChildDataLength,para->u,urlLen);//url
    SendChildDataLength += urlLen;

    //拼发送数据协议
    temLen = strlen(idCode);
    memcpy(sendDataBuf+SendDataLength,idCode,temLen);//验证码
    SendDataLength += temLen;
    *(long*)(sendDataBuf + SendDataLength) = (long)(sizeof(UINT64)+sizeof(short)*2 + sizeof(long) + SendChildDataLength);
    SendDataLength += sizeof(long);
    *(UINT64*)(sendDataBuf + SendDataLength) = (UINT64)0;//保留位
    SendDataLength += sizeof(UINT64);
    *(short*)(sendDataBuf + SendDataLength) = (short)para->f;//刷新等级
    SendDataLength += sizeof(short);
    *(short*)(sendDataBuf + SendDataLength) = (short)1;//请求数目
    SendDataLength += sizeof(short);
    *(long*)(sendDataBuf + SendDataLength) = (long)SendChildDataLength;
    SendDataLength += sizeof(long);
    memcpy(sendDataBuf+SendDataLength,sendChildDataBuf,SendChildDataLength);
    SendDataLength += SendChildDataLength;

    /**debug info**/
    if(DEBUGFLAG)
    {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/debug.txt","wb");
      WriteFile(sendDataBuf,SendDataLength,1,pr);
      fclose(pr);
      fprintf(stderr,"pSocket:%d\n",pSocket);
    }
    /**debug info**/

    int nAllLen=0,nOneLen=0;
    do
    {
      nOneLen=send(pSocket,sendDataBuf+nAllLen,SendDataLength-nAllLen,0);
      if(nOneLen>0)nAllLen+=nOneLen;
    }while(nOneLen>0 && nAllLen<SendDataLength);

    if(nAllLen!=SendDataLength)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"发送数据失败！");
      }
      return -5001;
    }

    //验证对方是否收到
    int verifyCodeLen = 8;
    char verifyCode[9];
    nAllLen=0;
    do
    {
      nOneLen=recv(pSocket,verifyCode+nAllLen,verifyCodeLen-nAllLen,0);
      if(nOneLen>0)
      {
        nAllLen+=nOneLen;
      }

    }while(nOneLen>0 && nAllLen<verifyCodeLen);

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"ishc:verifyCode:%s;recelen:%d;\n",verifyCode,nAllLen);
    }
    /**debug info**/

    if(nAllLen!=verifyCodeLen)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误COMMLength,vn:%d,rn:%d,rd:%s！",verifyCodeLen,nAllLen,verifyCode);
      }
      return -5002;
    }
    if(strncmp(verifyCode,"COMM",4) != 0)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"校验错误COMMSTR,code:%ld！",(long)*(long*)(verifyCode+4));
      }
      return -5003;
    }

    //验证数据的合法性
    int verifyDataLen=strlen(idCode) + sizeof(long);
    nAllLen=0;
    do
    {
      nOneLen=recv(pSocket,sendDataBuf+nAllLen,verifyDataLen-nAllLen,0);
      if(nOneLen>0)
      {
        nAllLen+=nOneLen;
      }

    }while(nOneLen>0 && nAllLen<verifyDataLen);

    if(nAllLen!=verifyDataLen)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误VERIFYCODELENGTH,vn:%d,rn:%d,rd:%s！",verifyDataLen,nAllLen,sendDataBuf);
      }
      return -5004;
    }

    sendDataBuf[verifyDataLen]=0;
    ch = *(sendDataBuf + temLen);
    *(sendDataBuf + temLen) = 0;
    if(strncmp(sendDataBuf,idCode,temLen) != 0)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"校验错误VERIFYCODESTR,code:%ld！",(long)*(long*)(sendDataBuf+temLen));
      }
      return -5005;
    }

    *(sendDataBuf + temLen) = ch;
    char *temBuf;
    temBuf = (sendDataBuf + temLen);
    allRecvDataLength = (long)*(long*)(temBuf);
    if(allRecvDataLength < 4)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误ALLDATALENGTH,code:%ld！",allRecvDataLength);
      }
      return -5006;
    }

    pBuf = new char[allRecvDataLength + verifyDataLen+2];
    if(pBuf ==	NULL)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"系统内部错误1！");
      }
      return -5007;
    }

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"ishc:allRecvDataLength:%d\n",allRecvDataLength);
    }
    /**debug info**/

    memcpy(pBuf,sendDataBuf,verifyDataLen);
    nAllLen=0;
    nOneLen=0;
    do
    {
      nOneLen=recv(pSocket,pBuf+verifyDataLen+nAllLen,allRecvDataLength-nAllLen,0);
      if(nOneLen>0)
      {
        nAllLen+=nOneLen;
      }

    }while(nOneLen>0 && nAllLen<allRecvDataLength);
    if(nAllLen != allRecvDataLength)
    {
      if(strlen(err) == 0)
      {
        sprintf(err,"数据不完整错误ALLRECVDATALENGTH,code:%ld！",nAllLen);
      }
      return -5008;
    }

    /**debug info**/
    if(DEBUGFLAG)
    {
      fprintf(stderr,"ishc:sendDataBuf:%s\n",sendDataBuf);
    }
    /**debug info**/

    pBuf[nAllLen+verifyDataLen]=0;

    /**debug info**/
    if(DEBUGFLAG)
    {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/revc2.txt","wb");
      WriteFile(pBuf,nAllLen+verifyDataLen,1,pr);
      fclose(pr);
    }
    /**debug info**/

    return (allRecvDataLength+verifyDataLen);

  }

  return allRecvDataLength;
}

int  ReadConfigFile(const char *filename)
{
  FILE *fp = fopen(filename,"r");
  if(fp == NULL)
  {
    return -1;
  }


  char tmp[200];
  if(fgets(tmp ,190,fp))
  {
    if(strncmp(tmp,"ipnum=",6)!=0)
    {
      fclose(fp);
      return -2;
    }
    else
    {
      SERVERIPMAX = (unsigned int)atoi(tmp+6);
      if(SERVERIPMAX <= 0)
      {
        fclose(fp);
        return -3;
      }

      pServerIP = new CONNECTSERVERIPSTRUCT[SERVERIPMAX];
    }
  }
  int tmpIpNum = 0;
  for(int i=0; i<SERVERIPMAX; i++)
  {
    char tm[10];
    int sLen;
    sprintf(tm,"ip%d=",i);
    if(fgets(tmp ,190,fp))
    {
      if(strncmp(tmp,tm,strlen(tm))!=0)
      {
        fclose(fp);
        return -4;
      }

      sLen = CancelEnter(tmp) - 4;
      strncpy(pServerIP[i].strIP,tmp+4,sLen);
      *(pServerIP[i].strIP + sLen) = 0;
    }
    sprintf(tm,"port%d=",i);
    if(fgets(tmp ,190,fp))
    {
      if(strncmp(tmp,tm,strlen(tm))!=0)
      {
        fclose(fp);
        return -4;
      }
      pServerIP[i].iPort = atoi(tmp + strlen(tm));
    }
  }

  if(fgets(tmp ,190,fp))
  {
    if(strncmp(tmp,"sockettime=",11)!=0)
    {
      fclose(fp);
      return -5;
    }
    else
    {
      SOCKETOUTTIME = (unsigned int)atoi(tmp + 11);
    }
  }
  if(fgets(tmp ,190,fp))
  {
    if(strncmp(tmp,"widgetversion=",14)!=0)
    {
      fclose(fp);
      return -5;
    }
    else
    {
      int sLen = CancelEnter(tmp) - 14;
      strncpy(widgetVersion,tmp+14,sLen);
    }
  }
  fclose(fp);
  return 0;
}

int CancelEnter(char *p)
{
  long lLen = strlen(p) -1;
  while(lLen >= 0 && (p[lLen] == (char)0x0d || p[lLen] == (char)0x0a || p[lLen] == (char)0x20))
    lLen--;
  lLen++;
  p[lLen] = 0;
  return lLen;
}

int CreateSocket(const char *ipAddr,const int port,const int lTimeOut)
{
  if(ipAddr==NULL || port<=0)
    return -2;


  int pSocket=-1;
  pSocket=socket(AF_INET,SOCK_STREAM,0);
  if(pSocket<0)
    return -1;

  sockaddr_in clientaddr;
  memset(&clientaddr,0,sizeof(clientaddr));

  clientaddr.sin_family =AF_INET;
  clientaddr.sin_port =htons(port);
  inet_aton(ipAddr,&(clientaddr.sin_addr));


  struct timeval nTimeOut;
  nTimeOut.tv_sec = lTimeOut;
  nTimeOut.tv_usec = 0;

  setsockopt(pSocket,SOL_SOCKET ,SO_RCVTIMEO,(char *)&nTimeOut,sizeof(timeval));
  setsockopt(pSocket,SOL_SOCKET ,SO_SNDTIMEO,(char *)&nTimeOut,sizeof(timeval));

  int option=1;
  setsockopt(pSocket,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

  int ret=connect(pSocket,(sockaddr*)&clientaddr,sizeof(sockaddr_in));

  if(ret!=0)
  {
    close(pSocket);
    return -5;
  }

  return pSocket;
}

int RandConnectServer(int &iIPIndex,const int iIPMAX,CONNECTSERVERIPSTRUCT *pServerIP,bool *ipS)
{
  int iNoConnect=0;

  /**debug info**/
  if(DEBUGFLAG)
  {
    fprintf(stderr,"pServerIP[iIPIndex].strIP-[%s]-pServerIP[iIPIndex].iPort-[%d]==\n",pServerIP[iIPIndex].strIP,pServerIP[iIPIndex].iPort);
  }
  /**debug info**/

  int pSocket = -5;

  if(ipS[iIPIndex])
  {
    pSocket = CreateSocket(pServerIP[iIPIndex].strIP,pServerIP[iIPIndex].iPort,SOCKETOUTTIME);
  }

  if((int)pSocket==-5 && iIPMAX>1)
  {
    int iTmpIndex=iIPIndex;
    do
    {
      if(iTmpIndex>=iIPMAX-1)
        iTmpIndex=0;
      else
      {
        iTmpIndex++;
      }

      if(ipS[iTmpIndex])
      {
        pSocket=CreateSocket(pServerIP[iTmpIndex].strIP,pServerIP[iTmpIndex].iPort,SOCKETOUTTIME);
      }

    }while(iTmpIndex!=iIPIndex && pSocket<0);

    iIPIndex=iTmpIndex;
  }

  return pSocket;
}

int GetRandNum()
{
  //struct timeb tp;
  //ftime(&tp);
  //return tp.millitm;
  return rand();
}

void UrlDecode(char *p)
{
  char *pD;
  if(p==NULL)
    return;
  pD = p;
  while (*p)
  {
    if (*p=='%')
    {
      p++;
      if (isxdigit(p[0]) && isxdigit(p[1]))
      {
        *pD++ = (char) TwoHex2Int(p);
        p += 2;
      }
    }
    else  *pD++ = *p++;
  }
  *pD = '\0';
}

int TwoHex2Int(char *pC)
{
  int Hi,Lo,Result;
  Hi = pC[0];
  if ('0'<=Hi && Hi<='9')
    Hi -= '0';
  else  if ('a'<=Hi && Hi<='f')
    Hi -= ('a'-10);
  else if ('A'<=Hi && Hi<='F')
    Hi -= ('A'-10);
  Lo = pC[1];
  if ('0'<=Lo && Lo<='9')
    Lo -= '0';
  else  if ('a'<=Lo && Lo<='f')
    Lo -= ('a'-10);
  else if ('A'<=Lo && Lo<='F')
    Lo -= ('A'-10);
  Result = Lo + 16*Hi;
  return Result;
}

int TrimLeft_RightBlank(char *strWordBuf)
{
  if(strWordBuf==NULL || strWordBuf[0]==0)
    return -1;

  int nStrLen=strlen(strWordBuf);
  if(nStrLen<=0)
    return -1;

  char *p1=strWordBuf,*p2=strWordBuf+nStrLen-1;

  if(*p2==' ')
  {
    while(*p2==' ')
      p2--;

    *(p2+1)=0;
    *(p2+2)=0;

    nStrLen=p2-strWordBuf;
  }

  if(*p1!=' ')
    return nStrLen;


  p2=strWordBuf;
  while(*p2 && *p2==' ')p2++;
  if(p2!=p1)
  {
    while(*p2)*p1++=*p2++;
    nStrLen=p1-strWordBuf;
    *p1++=0;
    *p1=0;
  }
  return nStrLen;
}

int fScanStr(char *poRes,int iResLen,char *piScr,char *piMark)
{
  int  i , j = 0;

  if (piScr == NULL)
    return -1;

  int	nMarkLen=strlen(piMark);

  poRes[0]=0;

  char *p=piScr;

  if(strncmp(p,piMark,nMarkLen)==0 && p[nMarkLen]=='=')
  {
    i=nMarkLen+1;
  }
  else if(*p=='?' && strncmp(p+1,piMark,nMarkLen)==0 && p[nMarkLen+1]=='=')
  {
    i=nMarkLen+2;
  }
  else
  {
    if(*p!='&')
    {
      if(*p=='?')
      {
        p++;
      }

      while(*p && *p!='=')p++;
      if(*p!='=')
        return 0;

      p++;
      if(*p=='[')
      {
        while(*p && *p!=']')
          p++;

        if(*p!=']')
          return 0;
      }
    }

    while(*p)
    {
      if(*p=='&')
      {
        if(strncmp(p+1,piMark,nMarkLen)==0 && p[nMarkLen+1]=='=')
        {
          i=p-piScr+2+nMarkLen;
          break;
        }

        if(*(p+nMarkLen+2)=='[')
        {
          p+=nMarkLen+2;
          while(*p && *p!=']')
            p++;

          if(*p!=']')
            return 0;
        }
      }

      p++;
    }

    if(*p==0)
      return -2;
  }
  char  pEndChar='\0';

  if(piScr[i]=='[')
  {
    i++;

    while( piScr[i] != '\0' && j<iResLen)
    {
      if(piScr[i] == ']')
      {
        break;
      }

      poRes[j++] = piScr[i++];
    }

  }
  else
  {


    while( piScr[i] != '\0' && j<iResLen)
    {
      if(piScr[i] == '&')
      {
        break;
      }

      poRes[j++] = piScr[i++];
    }
  }

  if(j>=iResLen)
    return -5;

  poRes[j] = '\0';

  return 0;

}
int SetUrlParameters(URLPARAMETER * &para,char *pReg)
{
  UrlDecode(pReg);

  int pBufLength = 0;
  char parU[256]={0},strTmp[770]={0};
  int parS=-1,parDs=-1,parF=-1;

  if(fScanStr(strTmp,10,pReg,"s")==-5)
  {
    return -1;
  }
  para->s=atol(strTmp);
  if(fScanStr(strTmp,10,pReg,"ds")==-5)
  {
    return -2;
  }
  para->ds=atol(strTmp);
  if(fScanStr(strTmp,10,pReg,"f")==-5)
  {
    return -3;
  }
  para->f=atol(strTmp);
  if(fScanStr(strTmp,10,pReg,"a")==-5)
  {
    return -4;
  }
  para->a=atol(strTmp);
  if(fScanStr(para->b,50,pReg,"b")==-5)
  {
    return -5;
  }
  TrimLeft_RightBlank(para->b);
  if(fScanStr(parU,256,pReg,"u")==-5)
  {
    return -6;
  }
  TrimLeft_RightBlank(parU);
  if(strstr(parU,"http://") == NULL)
  {
    sprintf(para->u,"http://%s",parU);
  }
  else
  {
    sprintf(para->u,"%s",parU);
  }
  if(fScanStr(strTmp,10,pReg,"t")==-5)
  {
    return -7;
  }
  para->t=atol(strTmp);

  return 0;
}

void I_MSP_PrintfHtmlPage(request_rec *r,char *strUrl)
{
  if(strUrl==NULL)
    return ;


  ap_rputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html>\n",r);
  ap_rputs("<head>\n",r);
  ap_rputs("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">\n",r);
  ap_rputs("<title>网页订制</title>\n",r);

  fprintf(stderr,"<link href=\"%sblue.css\" rel=\"stylesheet\" type=\"text/css\">\n",I_MSP_CSSANDJSPATH);

  ap_rputs("\n",r);

  fprintf(stderr,"<script type=\"text/javascript\" src=\'%spreviewselect.js\'></script>\n",I_MSP_CGIPATH);


  ap_rputs("\n",r);
  ap_rputs("</head>\n",r);
  ap_rputs("\n",r);
  ap_rputs("<body>\n",r);
  ap_rputs("\n",r);
  ap_rputs("		<table cellspacing=\"0\" class=\"blo\" style=\"width:100%;\">\n",r);
  ap_rputs("			<tr class=\"blo_tr1\">\n",r);

  fprintf(stderr,"				<td class=\"blo_im\"><img src=\"%szs.gif\" width=\"16\" height=\"16\"/></td>\n",I_MSP_IMAGEPATH);

  ap_rputs("				<td class=\"blo_ti\"><span>选择内容区域</span></td>\n",r);
  ap_rputs("				<td class=\"blo_li\">&nbsp;</td>\n",r);
  ap_rputs("				<td class=\"blo_sl\">&nbsp;</td>\n",r);

  fprintf(stderr,"				<td class=\"blo_cl\"><a href=\"#\" onClick=\"OnCloseWindow();\"><img src=\"%sclose.gif\" alt=\"取消订阅\" width=\"12\" height=\"12\" border=\"0\"></a></td>\n",I_MSP_IMAGEPATH);

  ap_rputs("			</tr>\n",r);
  ap_rputs("			<tr style=\"\">\n",r);
  ap_rputs("				<td colspan=\"5\">\n",r);
  ap_rputs("					<div id=\"big_area_con\">\n",r);
  ap_rputs("						<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n",r);
  ap_rputs("                          <tr>\n",r);

  fprintf(stderr,"                            <td background=\"%saddweb_topbg.jpg\"><table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n",I_MSP_IMAGEPATH);

  ap_rputs("                              <tr>\n",r);
  ap_rputs("                              <form name=\"form_sub\" onsubmit=\"return FormatHtmlPage();\">\n",r);
  ap_rputs("                                <td>&nbsp;</td>\n",r);
  ap_rputs("                                <td height=\"45\" align=\"right\" ><span style=\"color:333333;\">网页地址：</span>\n",r);

  if(strUrl[0]==0)
  {
    ap_rputs("                                  <input name=\"biga_addw_inp\" type=\"text\" id=\"biga_addw_inp\" value=\"\" size=\"40\">\n",r);
    ap_rputs("                                  <input type=\"hidden\" name=\"hideurltext\" value=\"\">\n",r);

  }
  else
  {
    fprintf(stderr,"                                  <input name=\"biga_addw_inp\" type=\"text\" id=\"biga_addw_inp\" value=\"%s\" size=\"40\">\n",strUrl);
    fprintf(stderr,"                                  <input type=\"hidden\" name=\"hideurltext\" value=\"%s\">\n",strUrl);

  }

  ap_rputs("                                  <input name=\"dzbutton\"  type=\"submit\" value=\" 打开 \" >\n",r);
  ap_rputs("                                <label></label></td>\n",r);
  ap_rputs("                                </form>\n",r);
  ap_rputs("                                <td width=\"120\" align=\"right\" style=\"color:#666666;font-weight: bold;;\">操作提示：</td>\n",r);
  ap_rputs("                              </tr>\n",r);
  ap_rputs("                            </table>\n",r);
  ap_rputs("                              <table width=\"100%\" height=\"50\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n",r);
  ap_rputs("                                <tr>\n",r);
  ap_rputs("                                  <td width=\"200\">&nbsp;</td>\n",r);
  ap_rputs("                                  <td align=\"center\"><input style=\"font-size:14px; font-weight:bold;\" name=\"dybutton\"  disabled type=\"button\" value=\" 完 成 \" onClick=\"SubmitSelectBlocks();\">\n",r);
  ap_rputs("																	&nbsp;\n",r);
  ap_rputs("																	<input  name=\"unselectbutton\"  disabled type=\"button\" style=\"font-size:14px; \" value=\"取消选择\" onClick=\"UnSelectRange();\">\n",r);
  ap_rputs("																	&nbsp;\n",r);
  ap_rputs("																	<input name=\"ReturnButton\" type=\"button\" style=\"font-size:14px; \" value=\"返 回\" onClick=\"OnCloseWindow();\"></td>\n",r);
  ap_rputs("                                </tr>\n",r);
  ap_rputs("                              </table></td>\n",r);

  fprintf(stderr,"                            <td width=\"346\"><img src=\"%saddweb_tips.jpg\" width=\"346\" height=\"101\"></td>\n",I_MSP_IMAGEPATH);

  ap_rputs("                          </tr>\n",r);
  ap_rputs("                        </table>\n",r);

  ap_rputs("                <div id=\"big_area_con_clew\" class=\"big_area_con_clew\" style=\"display:none\"></div>",r);

  ap_rputs("							<div id=\"big_area_con_div2\">\n",r);

  if(strUrl[0]==0)
  {
    ap_rputs("								<iframe id=dividedcontentiframe src=\"about:blank\" width=\"100%\" marginwidth=\"2\" height=\"100%\" marginheight=\"2\" align=\"middle\" scrolling=\"auto\" frameborder=\"0\"></iframe>\n",r);
  }
  else
  {
    fprintf(stderr,"								<iframe id=dividedcontentiframe src=\"%sishc?s=1&u=[%s]\" width=\"100%\" marginwidth=\"2\" height=\"100%\" marginheight=\"2\" align=\"middle\" scrolling=\"auto\" frameborder=\"0\"></iframe>\n",I_MSP_CGIPATH,strUrl);
  }

  ap_rputs("							</div>\n",r);
  ap_rputs("				  </div>\n",r);
  ap_rputs("				</td>\n",r);
  ap_rputs("			</tr>\n",r);
  ap_rputs("		</table>\n",r);


  ap_rputs("<SCRIPT language=\"JavaScript\">\n",r);
  ap_rputs("SetItemHeight();\n",r);
  ap_rputs("function SetItemHeight()\n",r);
  ap_rputs("{\n",r);
  ap_rputs("	var iHeight=document.documentElement.offsetHeight-135;\n",r);
  ap_rputs("	if(iHeight<425)\n",r);
  ap_rputs("			iHeight=425;\n",r);

  ap_rputs("  document.getElementById(\"big_area_con_div2\").style.height=iHeight+\"px\";\n",r);

  ap_rputs("}\n",r);
  ap_rputs("</script>	\n",r);

  ap_rputs("</body>\n",r);
  ap_rputs("</html>\n",r);

}

void I_MSP_PrintfErrorHtmlPage(request_rec *r,char *strMsg,char *strUrl)
{
  if(strMsg==NULL)
    return;

  ap_rputs("<html><head><TITLE>网页订制</TITLE><meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">\n<body>",r);
  fprintf(stderr,"<!-- 打开页面失败，失败原因：%s -->",strMsg);

  /*if(strUrl==NULL)
  {
    fprintf(stderr,"\n<SCRIPT language=\"JavaScript\">\nparent.ShowTitle(\" %s\",false);\n</SCRIPT>\n",strMsg);
  }
  else
  {
    fprintf(stderr,"\n<SCRIPT language=\"JavaScript\">\nparent.ShowTitle(\"不能连接到 %s,请稍后重试。\",false);\n</SCRIPT>\n",strUrl);
  }*/
  ap_rputs("</body></html>",r);

}

void I_SHC_36_PrintfErrorHtmlPage(request_rec *r,char *strMsg,int iRet,char* errcode)
{
  if(strMsg==NULL)
    return;

  ap_rputs("<html>\n<head>\n<TITLE>网页订制</TITLE>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">\n<style type=\"text/css\">\n<!--\nbody,td,th {\n	font-size: 12px;\n	text-align: center;\n}\na:link {\n	color: #0000CC;\n}\nbody {\n	margin-top: 4px;\n	margin-bottom: 4px;\n}\n-->\n</style>\n</head>\n<body>\n",r);

  ap_rputs("<div id=\"Block_Topbar\" style=\"background:#FFFFE1; text-align:center;padding:2px 0px 0px;display:\'\'; border:1px solid #ccc;position:absolute;top:0;left:0;width:100%;z-index:100 \">\n",r);
  ap_rputs("<div id=\"iewarning\" style=\"width:19px; padding:1px 2px 0px 4px;float:left;\">\n",r);
  ap_rputs("<img align=\"absmiddle\" src=\"http://i0.zhongso.com/img/warning_otip.gif\" border=\"0\" ></div>\n",r);
  ap_rputs("<div id=\"closeimg\" style=\"width:19px; float:right;padding:4px;\">\n",r);
  ap_rputs("<a href=\"javascript:closediv(\'Block_Topbar\');\" title=\"关闭提示\">\n",r);
  ap_rputs("<img src=\"http://i0.zhongso.com/img/close_otip.gif\" align=\"absmiddle\" border=\"0\" ></a></div>\n",r);
  ap_rputs("<div style=\" marign-left:4px;font-size:12px;color:#F35B37;padding:2px\" id=\"Block_Topbar_info\">\n",r);
  ap_rputs("页面暂时无法打开，<a href=\"javascript:window.location.reload();\"  style=\"color:#0000FF;text-decoration:underline\" id=\"Block_Topbar_link\">\n",r);
  ap_rputs("<b>请刷新</b></a></div></div><div style=\"clear:both\"></div>\n",r);
  ap_rputs("<script>function closediv(i) {document.getElementById(i).style.display=\'none\'; }</script>\n",r);

  fprintf(stderr,"\n<!--取得模块失败原因：%d,%s,%s-->\n",iRet,errcode,strMsg);
  ap_rputs("</body></html>",r);

}

int GetUrlHost(char *url,char * &host)
{
  if(url == NULL || host == NULL || strlen(url) < 7) return -1;
  int urlLen = strlen(url);
  int p = strncmp(url,"http://",7);
  int i = 0;
  if(p == 0) i += 7;
  while(*(url + i) != '/' && i < urlLen)
    i++;
  memcpy(host,url,i);
  *(host + i) = 0;
  return 0;
}

size_t WriteFile( const void *buf, size_t itemSize, size_t items, FILE *fd )
{
  size_t saved = 0;
  size_t count;

  // Save requesting data
  while ( (count = fwrite( buf, items, itemSize, fd )) < items)
  {
    saved  += count;
    if ( ferror( fd ) )
      return saved;
    buf     = ((char *) buf) + itemSize * count;
    items  -= count;
  }

  saved += count;

  return saved;
}

