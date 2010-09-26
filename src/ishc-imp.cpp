#include <pthread.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <event.h>
#include <ctype.h>
#include <unistd.h>
#include <iconv.h>

#include "ProBlockChange.h"
using namespace std;

#ifndef	  UINT64
#define   UINT64 unsigned long long
#endif

#define OK 1

/**ȫ�ֱ�������**/
static bool				DEBUGFLAG=false;		   //SOCKET��ʱʱ��(��λ����)
static unsigned int		SOCKETOUTTIME=10;		   //SOCKET��ʱʱ��(��λ����)
static unsigned int		SERVERIPMAX=0;			   //��ǰIP����
static bool				bInitFlag=false;		   //�Ƿ��ʼ��
static char				widgetVersion[20];		   //�Ƿ��ӡ��Ϣ
static char					I_MSP_CGIPATH[70];
static char					I_MSP_IMAGEPATH[70];
static char					I_MSP_CSSANDJSPATH[70];

struct CONNECTSERVERIPSTRUCT {
  char strIP[16];			//������IP
  int	iPort;				//������PORT
  bool bConnect;			//�Ƿ������ӷ�����

  pthread_mutex_t		pThreadMutex;	//�̶߳�д��

  CONNECTSERVERIPSTRUCT() {
    bConnect=true;
    pthread_mutex_init(&pThreadMutex,NULL);
  }

  ~CONNECTSERVERIPSTRUCT() {
    pthread_mutex_destroy(&pThreadMutex);
  }

  //����ֵ����
  void ChangeIsConnectValue(bool bC) {
    pthread_mutex_lock(&pThreadMutex);
    bConnect=bC;
    pthread_mutex_unlock(&pThreadMutex);
  }
};

//url ����
struct URLPARAMETER {
  int s;    //0 �� : isch, 1:imsp
  int t;    //��汾
  int ds;   //0 �ޣ�ȡimsp����ҳ��Դ�룬1��ȡimsp�Ŀ��
  int f;    //ˢ�µȼ�
  int a;	  //1���¼ӿ飬0 �ޣ����п�
  char b[50];	  //ishc ��id
  char u[256];   //url
};

CONNECTSERVERIPSTRUCT *pServerIP = NULL;
CProBlockChange *proBlockChange = NULL;
/**���ܺ���**/
size_t WriteFile( const void *buf, size_t itemSize, size_t items, FILE *fd ) {
  size_t saved = 0;
  size_t count;

  // Save requesting data
  while ( (count = fwrite( buf, items, itemSize, fd )) < items) {
    saved  += count;
    if ( ferror( fd ) ) return saved;
    buf     = ((char *) buf) + itemSize * count;
    items  -= count;
  }
  saved += count;
  return saved;
}
//ȥ���ո񡢻س������з�
int CancelEnter(char *p) {
  int lLen = strlen(p) -1;
  while(lLen >= 0 &&
        (p[lLen] == (char)0x0d ||
         p[lLen] == (char)0x0a ||
         p[lLen] == (char)0x20))
    lLen--;
  lLen++;
  p[lLen] = 0;
  return lLen;
}
//ɾ�����ҿո�
int TrimLeft_RightBlank(char *strWordBuf) {
  if(strWordBuf==NULL || strWordBuf[0]==0) return -1;

  int nStrLen=strlen(strWordBuf);
  if(nStrLen<=0) return -1;

  char *p1=strWordBuf,*p2=strWordBuf+nStrLen-1;

  if(*p2==' ') {
    while(*p2==' ') p2--;
    *(p2+1)=0;
    nStrLen=p2-strWordBuf;
  }

  if(*strWordBuf!=' ') return nStrLen;

  p2=strWordBuf;
  while(*p2 && *p2==' ')p2++;
  if(p2!=strWordBuf) {
    while(*p2)*p1++=*p2++;
    nStrLen=p1-strWordBuf;
    *p1++=0;
    *p1=0;
  }
  return nStrLen;
}
//URL����
int TwoHex2Int(char *pC) {
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
void UrlDecode(char *p) {
  char *pD;
  if(p==NULL) return;
  pD = p;
  while (*p) {
    if (*p=='%') {
      p++;
      if (isxdigit(p[0]) && isxdigit(p[1])) {
        *pD++ = (char) TwoHex2Int(p);
        p += 2;
      }
    }
    else  *pD++ = *p++;
  }
  *pD = '\0';
}
int GetUrlHost(char *url, char * &host) {
  if(url == NULL || host == NULL || strlen(url) < 7) return -1;
  int i = 0, urlLen = strlen(url);
  if(strncmp(url,"http://",7) == 0) i += 7;
  while(*(url + i) != '/' && i < urlLen) i++;
  memcpy(host,url,i);
  *(host + i) = 0;
  return 0;
}

//��ȡ�����ļ�
int  ReadConfigFile(const char *filename) {
  FILE *fp = fopen(filename,"r");
  if(fp == NULL) { return -1; }

  char tmp[200];
  if(fgets(tmp ,190,fp)) {
    if(strncmp(tmp,"ipnum=",6)!=0) { fclose(fp); return -2; }
    else {
      SERVERIPMAX = (unsigned int)atoi(tmp+6);
      if(SERVERIPMAX <= 0) { fclose(fp); return -3; }

      pServerIP = new CONNECTSERVERIPSTRUCT[SERVERIPMAX];
    }
  }

  for(int i=0; i<SERVERIPMAX; i++) {
    char tm[10];
    int sLen;
    sprintf(tm,"ip%d=",i);
    if(fgets(tmp ,190,fp)) {
      if(strncmp(tmp,tm,strlen(tm))!=0) { fclose(fp); return -4; }

      sLen = CancelEnter(tmp) - 4;
      strncpy(pServerIP[i].strIP,tmp+4,sLen);
      *(pServerIP[i].strIP + sLen) = 0;
    }
    sprintf(tm,"port%d=",i);
    if(fgets(tmp ,190,fp)) {
      if(strncmp(tmp,tm,strlen(tm))!=0) { fclose(fp); return -4; }
      pServerIP[i].iPort = atoi(tmp + strlen(tm));
    }
  }

  if(fgets(tmp ,190,fp)) {
    if(strncmp(tmp,"sockettime=",11)!=0) { fclose(fp); return -5; }
    else {
      SOCKETOUTTIME = (unsigned int)atoi(tmp + 11);
    }
  }
  if(fgets(tmp ,190,fp)) {
    if(strncmp(tmp,"widgetversion=",14)!=0) { fclose(fp); return -5; }
    else {
      int sLen = CancelEnter(tmp) - 14;
      strncpy(widgetVersion,tmp+14,sLen);
    }
  }
  fclose(fp);
  return 0;
}

//����socket����
int CreateSocket(const char *sServerIP,const int nPort,const int lTimeOut=10) {
  if(sServerIP==NULL || nPort<=0) return -2;

  int pSocket=-1;
  pSocket=socket(AF_INET,SOCK_STREAM,0);
  if(pSocket<0) return -1;

  sockaddr_in clientaddr;
  memset(&clientaddr,0,sizeof(clientaddr));

  clientaddr.sin_family =AF_INET;
  clientaddr.sin_port =htons(nPort);
  inet_aton(sServerIP,&(clientaddr.sin_addr));

  struct timeval nTimeOut;
  nTimeOut.tv_sec = lTimeOut;
  nTimeOut.tv_usec = 0;

  setsockopt(pSocket,SOL_SOCKET ,SO_RCVTIMEO,(char *)&nTimeOut,sizeof(timeval));
  setsockopt(pSocket,SOL_SOCKET ,SO_SNDTIMEO,(char *)&nTimeOut,sizeof(timeval));

  int option=1;
  setsockopt(pSocket,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

  int ret=connect(pSocket,(sockaddr*)&clientaddr,sizeof(sockaddr_in));

  if(ret!=0) { close(pSocket); return -5; }

  return pSocket;
}
//���ip���ӷ�����
int RandConnectServer(int &iIPIndex,const int iIPMAX,CONNECTSERVERIPSTRUCT *pServerIP,bool *ipS) {
  /**debug info**/
  if(DEBUGFLAG) {
    fprintf(stderr,"pServerIP[iIPIndex].strIP-[%s]-pServerIP[iIPIndex].iPort-[%d]==\n",pServerIP[iIPIndex].strIP,pServerIP[iIPIndex].iPort);
  }
  /**debug info**/

  int pSocket = -5;
  if(ipS[iIPIndex]) {
    pSocket = CreateSocket(pServerIP[iIPIndex].strIP,pServerIP[iIPIndex].iPort,SOCKETOUTTIME);
  }

  if(pSocket==-5 && iIPMAX>1) {
    int iTmpIndex=iIPIndex;
    do {
      if(iTmpIndex>=iIPMAX-1) iTmpIndex=0;
      else { iTmpIndex++; }

      if(ipS[iTmpIndex]) {
        pSocket=CreateSocket(pServerIP[iTmpIndex].strIP,pServerIP[iTmpIndex].iPort,SOCKETOUTTIME);
      }

    }while(iTmpIndex!=iIPIndex && pSocket<0);

    iIPIndex=iTmpIndex;
  }

  return pSocket;
}

//��ȡ����
// piScr sample:
// http://www.zhongsou.net/a/ishc?t=0&u=[http://www.steel114.com.cn/]&n=1&b=16598147728496656451_0&f=1
int fScanStr(char *poRes,int iResLen,char *piScr, const char *piMark) {
  if (piScr == NULL) return -1;

  int  i , j = 0;
  int	nMarkLen=strlen(piMark);
  char *p=piScr;

  poRes[0]=0;
 find_head:
  while(*p!=0 && !(strncmp(p,piMark,nMarkLen)==0 && p[nMarkLen]=='=')) {
    if (*p=='[') {
      while(*p && *p!=']') p++;
      if(*p!=']') return 0;
    } else p++;
  }
  if (*p==0) return 0;
  if (p>piScr) {
    if(*(p-1) != '?' && *(p-1) != '&') { p++; goto find_head; }
  }

  i=nMarkLen+1;
  if(p[i]=='[') {
    i++;
    while( p[i] != '\0' && j<iResLen) {
      if(p[i] == ']') { break; }
      poRes[j++] = p[i++];
    }
  } else {
    while( p[i] != '\0' && j<iResLen) {
      if(p[i] == '&') { break; }
      poRes[j++] = p[i++];
    }
  }

  if(j>=iResLen) return -5;
  poRes[j] = '\0';
  return 0;
}

// 0x8140 <= *p*(p+1) < 0xFEFE
static inline bool
isgbk(unsigned char *c)
{
  return (0x81 <= *c && *c < 0xFF && 0x40 <= *(c+1) && *(c+1) < 0xFF);
}

const char *encs[] = {"UTF-8", "BIG5"};
const char *enct   = "GBK";
static inline bool
convert(char *dest, size_t dest_len, const char *src)
{
  bool ret = false, needcvt = false;
  int i, n_encs;
  size_t inlen, outlen;
  char *s = NULL, *os=(char *)src, *d=dest;

  while(*os) {
    if(!isprint(*os)) {
      if (isgbk((unsigned char *)os)) {
	os += 2;
	continue;
      } else {
	needcvt=true;
	break;
      }
    }
    os++;
  }
  if (!needcvt) return false;

  n_encs = sizeof(encs) / sizeof(char *);
  for (i=0; i<n_encs; i++) {
    if(!ret) {
      iconv_t cd = iconv_open(enct, encs[i]);
      if (cd == (iconv_t) -1) continue;

      os=strdup(src);
      s =os;
      if (s!=NULL) {
	inlen  = strlen(src);
	outlen = dest_len;
	memset(dest, 0, dest_len);
        d = dest;

	while (inlen > 0) {
	  size_t convrst = iconv(cd, &s, &inlen, &d, &outlen);
	  if(convrst == (size_t) -1) break;
	}
	if (inlen == 0) ret = true;
	free(os);
      }
      iconv_close(cd);
    }
  }
  return ret;
}

//���url����
int SetUrlParameters(URLPARAMETER * &para,char *ppReg) {
  UrlDecode(ppReg);
  char *pReg, dest[4096];
  pReg = dest;
  if (!convert(pReg, 4095, ppReg)) {
    pReg = ppReg;
  }

  char parU[256]={0},strTmp[770]={0};

  if(fScanStr(strTmp,10,pReg,"s" )==-5) { return -1; }
  para->s=atol(strTmp);

  if(fScanStr(strTmp,10,pReg,"ds")==-5) { return -2; }
  para->ds=atol(strTmp);

  if(fScanStr(strTmp,10,pReg,"f" )==-5) { return -3; }
  para->f=atol(strTmp);

  if(fScanStr(strTmp,10,pReg,"a" )==-5) { return -4; }
  para->a=atol(strTmp);

  if(fScanStr(para->b,50,pReg,"b")==-5) { return -5; }
  TrimLeft_RightBlank(para->b);

  if(fScanStr(parU,  256,pReg,"u")==-5) { return -6; }
  TrimLeft_RightBlank(parU);

  if(strstr(parU,"http://") == NULL) {
    sprintf(para->u,"http://%s",parU);
  } else {
    sprintf(para->u,"%s",parU);
  }

  if(fScanStr(strTmp,10,pReg,"t" )==-5) { return -7; }
  para->t=atol(strTmp);

  return 0;
}

//��ӡimsp���
void I_MSP_PrintfHtmlPage(evbuffer *buf,char *strUrl) {
  if(strUrl==NULL) return ;

  evbuffer_add_printf(buf, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html>\n");
  evbuffer_add_printf(buf, "<head>\n");
  evbuffer_add_printf(buf, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gbk\">\n");
  evbuffer_add_printf(buf, "<title>��ҳ����</title>\n");

  evbuffer_add_printf(buf, "<link href=\"%sblue.css\" rel=\"stylesheet\" type=\"text/css\">\n\n",I_MSP_CSSANDJSPATH);
  evbuffer_add_printf(buf, "<script type=\"text/javascript\" src=\'%spreviewselect.js\'></script>\n",I_MSP_CGIPATH);

  evbuffer_add_printf(buf, "\n</head>\n\n");
  evbuffer_add_printf(buf, "<body>\n\n");
  evbuffer_add_printf(buf, "		<table cellspacing=\"0\" class=\"blo\" style=\"width:100%;\">\n");
  evbuffer_add_printf(buf, "			<tr class=\"blo_tr1\">\n");

  evbuffer_add_printf(buf, "				<td class=\"blo_im\"><img src=\"%szs.gif\" width=\"16\" height=\"16\"/></td>\n",I_MSP_IMAGEPATH);

  evbuffer_add_printf(buf, "				<td class=\"blo_ti\"><span>ѡ����������</span></td>\n");
  evbuffer_add_printf(buf, "				<td class=\"blo_li\">&nbsp;</td>\n");
  evbuffer_add_printf(buf, "				<td class=\"blo_sl\">&nbsp;</td>\n");

  evbuffer_add_printf(buf, "				<td class=\"blo_cl\"><a href=\"#\" onClick=\"OnCloseWindow();\"><img src=\"%sclose.gif\" alt=\"ȡ������\" width=\"12\" height=\"12\" border=\"0\"></a></td>\n",I_MSP_IMAGEPATH);

  evbuffer_add_printf(buf, "			</tr>\n");
  evbuffer_add_printf(buf, "			<tr style=\"\">\n");
  evbuffer_add_printf(buf, "				<td colspan=\"5\">\n");
  evbuffer_add_printf(buf, "					<div id=\"big_area_con\">\n");
  evbuffer_add_printf(buf, "						<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n");
  evbuffer_add_printf(buf, "                          <tr>\n");

  evbuffer_add_printf(buf, "                            <td background=\"%saddweb_topbg.jpg\"><table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n",I_MSP_IMAGEPATH);

  evbuffer_add_printf(buf, "                              <tr>\n");
  evbuffer_add_printf(buf, "                              <form name=\"form_sub\" onsubmit=\"return FormatHtmlPage();\">\n");
  evbuffer_add_printf(buf, "                                <td>&nbsp;</td>\n");
  evbuffer_add_printf(buf, "                                <td height=\"45\" align=\"right\" ><span style=\"color:333333;\">��ҳ��ַ��</span>\n");

  if(strUrl[0]==0) {
    evbuffer_add_printf(buf, "                                  <input name=\"biga_addw_inp\" type=\"text\" id=\"biga_addw_inp\" value=\"\" size=\"40\">\n");
    evbuffer_add_printf(buf, "                                  <input type=\"hidden\" name=\"hideurltext\" value=\"\">\n");
  } else {
    evbuffer_add_printf(buf, "                                  <input name=\"biga_addw_inp\" type=\"text\" id=\"biga_addw_inp\" value=\"%s\" size=\"40\">\n",strUrl);
    evbuffer_add_printf(buf, "                                  <input type=\"hidden\" name=\"hideurltext\" value=\"%s\">\n",strUrl);
  }

  evbuffer_add_printf(buf, "                                  <input name=\"dzbutton\"  type=\"submit\" value=\" �� \" >\n");
  evbuffer_add_printf(buf, "                                <label></label></td>\n");
  evbuffer_add_printf(buf, "                                </form>\n");
  evbuffer_add_printf(buf, "                                <td width=\"120\" align=\"right\" style=\"color:#666666;font-weight: bold;;\">������ʾ��</td>\n");
  evbuffer_add_printf(buf, "                              </tr>\n");
  evbuffer_add_printf(buf, "                            </table>\n");
  evbuffer_add_printf(buf, "                              <table width=\"100%\" height=\"50\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n");
  evbuffer_add_printf(buf, "                                <tr>\n");
  evbuffer_add_printf(buf, "                                  <td width=\"200\">&nbsp;</td>\n");
  evbuffer_add_printf(buf, "                                  <td align=\"center\"><input style=\"font-size:14px; font-weight:bold;\" name=\"dybutton\"  disabled type=\"button\" value=\" �� �� \" onClick=\"SubmitSelectBlocks();\">\n");
  evbuffer_add_printf(buf, "																	&nbsp;\n");
  evbuffer_add_printf(buf, "																	<input  name=\"unselectbutton\"  disabled type=\"button\" style=\"font-size:14px; \" value=\"ȡ��ѡ��\" onClick=\"UnSelectRange();\">\n");
  evbuffer_add_printf(buf, "																	&nbsp;\n");
  evbuffer_add_printf(buf, "																	<input name=\"ReturnButton\" type=\"button\" style=\"font-size:14px; \" value=\"�� ��\" onClick=\"OnCloseWindow();\"></td>\n");
  evbuffer_add_printf(buf, "                                </tr>\n");
  evbuffer_add_printf(buf, "                              </table></td>\n");

  evbuffer_add_printf(buf, "                            <td width=\"346\"><img src=\"%saddweb_tips.jpg\" width=\"346\" height=\"101\"></td>\n",I_MSP_IMAGEPATH);

  evbuffer_add_printf(buf, "                          </tr>\n");
  evbuffer_add_printf(buf, "                        </table>\n");
  evbuffer_add_printf(buf, "                <div id=\"big_area_con_clew\" class=\"big_area_con_clew\" style=\"display:none\"></div>");
  evbuffer_add_printf(buf, "							<div id=\"big_area_con_div2\">\n");

  if(strUrl[0]==0) {
    evbuffer_add_printf(buf, "								<iframe id=dividedcontentiframe src=\"about:blank\" width=\"100%\" marginwidth=\"2\" height=\"100%\" marginheight=\"2\" align=\"middle\" scrolling=\"auto\" frameborder=\"0\"></iframe>\n");
  } else {
    evbuffer_add_printf(buf, "								<iframe id=dividedcontentiframe src=\"%sishc?s=1&u=[%s]\" width=\"100%\" marginwidth=\"2\" height=\"100%\" marginheight=\"2\" align=\"middle\" scrolling=\"auto\" frameborder=\"0\"></iframe>\n",I_MSP_CGIPATH,strUrl);
  }

  evbuffer_add_printf(buf, "							</div>\n");
  evbuffer_add_printf(buf, "				  </div>\n");
  evbuffer_add_printf(buf, "				</td>\n");
  evbuffer_add_printf(buf, "			</tr>\n");
  evbuffer_add_printf(buf, "		</table>\n");


  evbuffer_add_printf(buf, "<SCRIPT language=\"JavaScript\">\n");
  evbuffer_add_printf(buf, "SetItemHeight();\n");
  evbuffer_add_printf(buf, "function SetItemHeight()\n");
  evbuffer_add_printf(buf, "{\n");
  evbuffer_add_printf(buf, "	var iHeight=document.documentElement.offsetHeight-135;\n");
  evbuffer_add_printf(buf, "	if(iHeight<425)\n");
  evbuffer_add_printf(buf, "			iHeight=425;\n");
  evbuffer_add_printf(buf, "  document.getElementById(\"big_area_con_div2\").style.height=iHeight+\"px\";\n");
  evbuffer_add_printf(buf, "}\n");
  evbuffer_add_printf(buf, "</script>	\n");
  evbuffer_add_printf(buf, "</body>\n");
  evbuffer_add_printf(buf, "</html>\n");
}
//��ӡimsp����ҳ
void I_MSP_PrintfErrorHtmlPage(evbuffer *buf, char *strMsg, char *strUrl) {
  if(strMsg==NULL) return;

  evbuffer_add_printf(buf, "<html><head><TITLE>��ҳ����</TITLE><meta http-equiv=\"Content-Type\" content=\"text/html; charset=gbk\"></head><body>");
  evbuffer_add_printf(buf, "<!-- ��ҳ��ʧ�ܣ�ʧ��ԭ��%s -->",strMsg);

  /*if(strUrl==NULL) {
    evbuffer_add_printf(buf, "\n<SCRIPT language=\"JavaScript\">\nparent.ShowTitle(\" %s\",false);\n</SCRIPT>\n",strMsg);
  } else {
    evbuffer_add_printf(buf, "\n<SCRIPT language=\"JavaScript\">\nparent.ShowTitle(\"�������ӵ� %s,���Ժ����ԡ�\",false);\n</SCRIPT>\n",strUrl);
  }*/
  evbuffer_add_printf(buf, "</body></html>");
}
//��ӡishc����ҳ
void I_SHC_36_PrintfErrorHtmlPage(evbuffer *buf, const char *strMsg, int iRet, char* errcode) {
  if(strMsg==NULL) return;

  evbuffer_add_printf(buf, "<html>\n<head>\n<TITLE>��ҳ����</TITLE>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gbk\">\n<style type=\"text/css\">\n<!--\nbody,td,th {\n	font-size: 12px;\n	text-align: center;\n}\na:link {\n	color: #0000CC;\n}\nbody {\n	margin-top: 4px;\n	margin-bottom: 4px;\n}\n-->\n</style>\n</head>\n<body>\n");

  evbuffer_add_printf(buf, "<div id=\"Block_Topbar\" style=\"background:#FFFFE1; text-align:center;padding:2px 0px 0px;display:\'\'; border:1px solid #ccc;position:absolute;top:0;left:0;width:100%;z-index:100 \">\n");
  evbuffer_add_printf(buf, "<div id=\"iewarning\" style=\"width:19px; padding:1px 2px 0px 4px;float:left;\">\n");
  evbuffer_add_printf(buf, "<img align=\"absmiddle\" src=\"http://i0.zhongso.com/img/warning_otip.gif\" border=\"0\" ></div>\n");
  evbuffer_add_printf(buf, "<div id=\"closeimg\" style=\"width:19px; float:right;padding:4px;\">\n");
  evbuffer_add_printf(buf, "<a href=\"javascript:closediv(\'Block_Topbar\');\" title=\"�ر���ʾ\">\n");
  evbuffer_add_printf(buf, "<img src=\"http://i0.zhongso.com/img/close_otip.gif\" align=\"absmiddle\" border=\"0\" ></a></div>\n");
  evbuffer_add_printf(buf, "<div style=\" marign-left:4px;font-size:12px;color:#F35B37;padding:2px\" id=\"Block_Topbar_info\">\n");
  evbuffer_add_printf(buf, "ҳ����ʱ�޷��򿪣�<a href=\"javascript:window.location.reload();\"  style=\"color:#0000FF;text-decoration:underline\" id=\"Block_Topbar_link\">\n");
  evbuffer_add_printf(buf, "<b>��ˢ��</b></a></div></div><div style=\"clear:both\"></div>\n");
  evbuffer_add_printf(buf, "<script>function closediv(i) {document.getElementById(i).style.display=\'none\'; }</script>\n");

  evbuffer_add_printf(buf, "\n<!--ȡ��ģ��ʧ��ԭ��%d,%s,%s-->\n",iRet,errcode,strMsg);
  evbuffer_add_printf(buf, "</body></html>");
}

//ȡ����
int GetMakeHtmlPage(const int pSocket,URLPARAMETER *para,char *&pBuf, char *err) {
  if(pSocket<0 || pBuf!=NULL) return -1;

  char idCode[11];
  int allRecvDataLength = 0;
  if(para->s == 1 || (para->s != 1 && para->a == 1)) {//imsp
    sprintf(idCode,"CMSV1.0001");
    char sendDataBuf[1024]={0};
    int SendDataLength = 0;
    char sendChildDataBuf[512] = {0};
    int SendChildDataLength = 0;
    int temLen = 0;
    short urlLen = (short)strlen(para->u);

    //ƴ������Э��
    if(para->s == 1) {//Ԥ��
      *(sendChildDataBuf+SendChildDataLength) = 2;
      SendChildDataLength += 1;
      *(short*)(sendChildDataBuf + SendChildDataLength) = urlLen;
      SendChildDataLength += sizeof(short);
      memcpy(sendChildDataBuf+SendChildDataLength,para->u,urlLen);
      SendChildDataLength += urlLen;
      *(sendChildDataBuf+SendChildDataLength) = 0;
      SendChildDataLength += 1;
    } else {//����鴴��
      /**debug info**/
      if(DEBUGFLAG) { fprintf(stderr,"para->b:%s\n",para->b); }
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
      if(DEBUGFLAG) { fprintf(stderr,"fId:%ld\n",fId); }
      /**debug info**/

      *pos = ch;
      bBuf = pos + 1;
      short bId;
      if(pos = strstr(bBuf,",")) {
        ch = *pos;
        *pos = 0;
        bId = (short)atoi(bBuf);
        *pos = ch;
      } else {
        bId = (short)atoi(bBuf);
      }

      /**debug info**/
      if(DEBUGFLAG) { fprintf(stderr,"bId:%d\n",bId); }
      /**debug info**/

      int tempUrlLen = 44 + strlen(para->b) + strlen(para->u);
      char *tempUrl = new char[tempUrlLen];
      sprintf(tempUrl,"u=[%s]&f=%s&b=%d&v=%d",para->u,strFid,bId,para->t);
      tempUrlLen = strlen(tempUrl);

      /**debug info**/
      if(DEBUGFLAG) { fprintf(stderr,"tempUrl:%s\n",tempUrl); }
      /**debug info**/

      *(sendChildDataBuf+SendChildDataLength) = 2;
      SendChildDataLength += 1;
      *(int*)(sendChildDataBuf + SendChildDataLength) = tempUrlLen;//��ϢԴurl����
      SendChildDataLength += sizeof(int);
      memcpy(sendChildDataBuf+SendChildDataLength,tempUrl,tempUrlLen);//��ϢԴurl
      SendChildDataLength += tempUrlLen;
      *(int*)(sendChildDataBuf + SendChildDataLength) = (int)0;//���Գ���
      SendChildDataLength += sizeof(int);
      delete[] tempUrl;

      *(UINT64*)(sendChildDataBuf + SendChildDataLength) = fId;//formatid
      SendChildDataLength += sizeof(UINT64);
      *(short*)(sendChildDataBuf + SendChildDataLength) = bId;//blockid
      SendChildDataLength += sizeof(short);
      *(sendChildDataBuf+SendChildDataLength) = para->t;//��汾
      SendChildDataLength += 1;
      *(short*)(sendChildDataBuf + SendChildDataLength) = urlLen;//url����
      SendChildDataLength += sizeof(short);
      memcpy(sendChildDataBuf+SendChildDataLength,para->u,urlLen);//url
      SendChildDataLength += urlLen;
    }

    //ƴ��������Э��
    temLen = strlen(idCode);
    memcpy(sendDataBuf+SendDataLength,idCode,temLen);//��֤��
    SendDataLength += temLen;

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"SendDataLength:%d\n",SendDataLength); }
    /**debug info**/

    *(int*)(sendDataBuf + SendDataLength) = (int)(sizeof(UINT64)+sizeof(char)+sizeof(int) + SendChildDataLength);
    SendDataLength += sizeof(int);
    *(UINT64*)(sendDataBuf + SendDataLength) = (UINT64)0;
    SendDataLength += sizeof(UINT64);
    if(para->s == 1) {
      *(sendDataBuf+SendDataLength) = 3;
    } else {
      *(sendDataBuf+SendDataLength) = 0;
    }
    SendDataLength += 1;
    *(int*)(sendDataBuf + SendDataLength) = (int)SendChildDataLength;
    SendDataLength += sizeof(int);
    memcpy(sendDataBuf+SendDataLength,sendChildDataBuf,SendChildDataLength);
    SendDataLength += SendChildDataLength;

    int nAllLen=0,nOneLen=0;

    /**debug info**/
    if(DEBUGFLAG) {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/debug.txt","wb");
      WriteFile(sendDataBuf,SendDataLength,1,pr);
      fclose(pr);
      fprintf(stderr,"SOCKETOUTTIME:%d\n",SOCKETOUTTIME);
    }
    /**debug info**/

    do {
      nOneLen=send(pSocket,sendDataBuf+nAllLen,SendDataLength-nAllLen,0);
      if(nOneLen>0)nAllLen+=nOneLen;
    }while(nOneLen>0 && nAllLen<SendDataLength);

    if(nAllLen!=SendDataLength) {
      if(strlen(err) == 0) { sprintf(err,"��������ʧ�ܣ�"); }
      return -5001;
    }
    //��֤�Է��Ƿ��յ�
    int verifyCodeLen = 8;
    char verifyCode[9];
    nAllLen=0;
    do {
      nOneLen=recv(pSocket,verifyCode+nAllLen,verifyCodeLen-nAllLen,0);
      if(nOneLen>0) { nAllLen+=nOneLen; }
    }while(nOneLen>0 && nAllLen<verifyCodeLen);

    if(nAllLen!=verifyCodeLen) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������COMMLENGTH,vn:%d,rn:%d,rd:%s��",verifyCodeLen,nAllLen,verifyCode);
      }
      return -5002;
    }
    if(strncmp(verifyCode,"COMM",4) != 0) {
      if(strlen(err) == 0) {
        sprintf(err,"У�����COMMSTR,code:%ld��",(int)*(int*)(verifyCode+4));
      }
      return -5003;
    }

    //��֤���ݵĺϷ���
    int verifyDataLen=strlen(idCode) + sizeof(int);
    nAllLen=0;
    nOneLen = 0;
    do {
      nOneLen=recv(pSocket,sendDataBuf+nAllLen,verifyDataLen-nAllLen,0);
      /**debug info**/
      if(DEBUGFLAG) { fprintf(stderr,"nOneLen:%d\n",nOneLen); }
      /**debug info**/

      if(nOneLen>0) { nAllLen+=nOneLen; }
    }while(nOneLen>0 && nAllLen<verifyDataLen);

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"%d-%d\n",nAllLen,verifyDataLen); }
    /**debug info**/

    if(nAllLen!=verifyDataLen) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������VERIFYCODELENGTH,vn:%d,rn:%d,rd:%s��",verifyDataLen,nAllLen,sendDataBuf);
      }
      return -5004;
    }

    sendDataBuf[verifyDataLen]=0;

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"sendDataBuf:%s\n",sendDataBuf); }
    /**debug info**/

    char ch = *(sendDataBuf + temLen);
    *(sendDataBuf + temLen) = 0;
    if(strncmp(sendDataBuf,idCode,temLen) != 0) {
      if(strlen(err) == 0) {
        sprintf(err,"У�����VERIFYCODESTR,code:%ld��",(int)*(int*)(sendDataBuf+temLen));
      }
      return -5005;
    }
    *(sendDataBuf + temLen) = ch;
    char *temBuf;
    temBuf = (sendDataBuf + temLen);
    allRecvDataLength = (int)*(int*)(temBuf);

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"allRecvDataLength:%d\n",allRecvDataLength); }
    /**debug info**/

    if(allRecvDataLength < 4) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������ALLDATALENGTH,code:%ld��",allRecvDataLength);
      }
      return -5006;
    }

    pBuf = new char[allRecvDataLength + verifyDataLen];

    if(pBuf ==	NULL) {
      if(strlen(err) == 0) { sprintf(err,"ϵͳ�ڲ�����1��"); }
      return -5007;
    }

    memcpy(pBuf,sendDataBuf,verifyDataLen);
    nAllLen=0;
    nOneLen=0;
    do {
      nOneLen=recv(pSocket,pBuf+verifyDataLen+nAllLen,allRecvDataLength-nAllLen,0);
      if(nOneLen>0) { nAllLen+=nOneLen; }
    }while(nOneLen>0 && nAllLen<allRecvDataLength);
    if(nAllLen!=allRecvDataLength) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������ALLRECVDATALENGTH,code:%ld��",nAllLen);
      }
      return -5008;
    }

    pBuf[nAllLen + verifyDataLen - 1]=0;

    /**debug info**/
    if(DEBUGFLAG) {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/recv.txt","wb");
      WriteFile(pBuf,allRecvDataLength + verifyDataLen,1,pr);
      fclose(pr);
    }
    /**debug info**/

    return (allRecvDataLength + verifyDataLen);
  } else {
    sprintf(idCode,"CMSV1.0002");
    char sendDataBuf[1024]={0};
    int SendDataLength = 0;
    char sendChildDataBuf[512] = {0};
    int SendChildDataLength = 0;
    int temLen = 0;
    short urlLen = (short)strlen(para->u);

    //ƴ������Э��
    *(sendChildDataBuf+SendChildDataLength) = 2;//������
    SendChildDataLength += 1;
    *(UINT64*)(sendChildDataBuf + SendChildDataLength) = (UINT64)0;//����id
    SendChildDataLength += sizeof(UINT64);

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"para->b:%s\n",para->b); }
    /**debug info**/

    char *bBuf = para->b;
    char *pos;
    char ch;
    pos = strstr(bBuf,"_");
    if (pos == NULL) return -6001;
    ch = *pos;
    *pos = 0;
    UINT64 fId = strtoull(bBuf,NULL,10);
    *pos = ch;
    bBuf = pos + 1;
    short bId;
    if(pos = strstr(bBuf,",")) {
      ch = *pos;
      *pos = 0;
      bId = (short)atoi(bBuf);
      *pos = ch;
    } else {
      bId = (short)atoi(bBuf);
    }

    *(UINT64*)(sendChildDataBuf + SendChildDataLength) = fId;//formatid
    SendChildDataLength += sizeof(UINT64);

    *(short*)(sendChildDataBuf + SendChildDataLength) = bId;//blockid
    SendChildDataLength += sizeof(short);
    *(sendChildDataBuf+SendChildDataLength) = para->t;//��汾
    SendChildDataLength += 1;
    *(short*)(sendChildDataBuf + SendChildDataLength) = urlLen;//url����
    SendChildDataLength += sizeof(short);
    memcpy(sendChildDataBuf+SendChildDataLength,para->u,urlLen);//url
    SendChildDataLength += urlLen;

    //ƴ��������Э��
    temLen = strlen(idCode);
    memcpy(sendDataBuf+SendDataLength,idCode,temLen);//��֤��
    SendDataLength += temLen;
    *(int*)(sendDataBuf + SendDataLength) = (int)(sizeof(UINT64)+sizeof(short)*2 + sizeof(int) + SendChildDataLength);
    SendDataLength += sizeof(int);
    *(UINT64*)(sendDataBuf + SendDataLength) = (UINT64)0;//����λ
    SendDataLength += sizeof(UINT64);
    *(short*)(sendDataBuf + SendDataLength) = (short)para->f;//ˢ�µȼ�
    SendDataLength += sizeof(short);
    *(short*)(sendDataBuf + SendDataLength) = (short)1;//������Ŀ
    SendDataLength += sizeof(short);
    *(int*)(sendDataBuf + SendDataLength) = (int)SendChildDataLength;
    SendDataLength += sizeof(int);
    memcpy(sendDataBuf+SendDataLength,sendChildDataBuf,SendChildDataLength);
    SendDataLength += SendChildDataLength;

    /**debug info**/
    if(DEBUGFLAG) {
      FILE *pr = NULL;
      pr = fopen("/zhongsou/i/a/debug.txt","wb");
      WriteFile(sendDataBuf,SendDataLength,1,pr);
      fclose(pr);
      fprintf(stderr,"pSocket:%d\n",pSocket);
    }
    /**debug info**/

    int nAllLen=0,nOneLen=0;
    do {
      nOneLen=send(pSocket,sendDataBuf+nAllLen,SendDataLength-nAllLen,0);
      if(nOneLen>0)nAllLen+=nOneLen;
    }while(nOneLen>0 && nAllLen<SendDataLength);

    if(nAllLen!=SendDataLength) {
      if(strlen(err) == 0) { sprintf(err,"��������ʧ�ܣ�"); }
      return -5001;
    }

    //��֤�Է��Ƿ��յ�
    int verifyCodeLen = 8;
    char verifyCode[9];
    nAllLen=0;
    do {
      nOneLen=recv(pSocket,verifyCode+nAllLen,verifyCodeLen-nAllLen,0);
      if(nOneLen>0) { nAllLen+=nOneLen; }
    }while(nOneLen>0 && nAllLen<verifyCodeLen);

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"ishc:verifyCode:%s;recelen:%d;\n",verifyCode,nAllLen); }
    /**debug info**/

    if(nAllLen!=verifyCodeLen) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������COMMLength,vn:%d,rn:%d,rd:%s��",verifyCodeLen,nAllLen,verifyCode);
      }
      return -5002;
    }
    if(strncmp(verifyCode,"COMM",4) != 0) {
      if(strlen(err) == 0) {
        sprintf(err,"У�����COMMSTR,code:%ld��",(int)*(int*)(verifyCode+4));
      }
      return -5003;
    }

    //��֤���ݵĺϷ���
    int verifyDataLen=strlen(idCode) + sizeof(int);
    nAllLen=0;
    do {
      nOneLen=recv(pSocket,sendDataBuf+nAllLen,verifyDataLen-nAllLen,0);
      if(nOneLen>0) { nAllLen+=nOneLen; }
    }while(nOneLen>0 && nAllLen<verifyDataLen);

    if(nAllLen!=verifyDataLen) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������VERIFYCODELENGTH,vn:%d,rn:%d,rd:%s��",verifyDataLen,nAllLen,sendDataBuf);
      }
      return -5004;
    }

    sendDataBuf[verifyDataLen]=0;
    ch = *(sendDataBuf + temLen);
    *(sendDataBuf + temLen) = 0;
    if(strncmp(sendDataBuf,idCode,temLen) != 0) {
      if(strlen(err) == 0) {
        sprintf(err,"У�����VERIFYCODESTR,code:%ld��",(int)*(int*)(sendDataBuf+temLen));
      }
      return -5005;
    }

    *(sendDataBuf + temLen) = ch;
    char *temBuf;
    temBuf = (sendDataBuf + temLen);
    allRecvDataLength = (int)*(int*)(temBuf);
    if(allRecvDataLength < 4) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������ALLDATALENGTH,code:%ld��",allRecvDataLength);
      }
      return -5006;
    }

    pBuf = new char[allRecvDataLength + verifyDataLen+2];
    if(pBuf ==	NULL) {
      if(strlen(err) == 0) { sprintf(err,"ϵͳ�ڲ�����1��"); }
      return -5007;
    }

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"ishc:allRecvDataLength:%d\n",allRecvDataLength); }
    /**debug info**/

    memcpy(pBuf,sendDataBuf,verifyDataLen);
    nAllLen=0;
    nOneLen=0;
    do {
      nOneLen=recv(pSocket,pBuf+verifyDataLen+nAllLen,allRecvDataLength-nAllLen,0);
      if(nOneLen>0) { nAllLen+=nOneLen; }
    }while(nOneLen>0 && nAllLen<allRecvDataLength);
    if(nAllLen != allRecvDataLength) {
      if(strlen(err) == 0) {
        sprintf(err,"���ݲ���������ALLRECVDATALENGTH,code:%ld��",nAllLen);
      }
      return -5008;
    }

    /**debug info**/
    if(DEBUGFLAG) { fprintf(stderr,"ishc:sendDataBuf:%s\n",sendDataBuf); }
    /**debug info**/

    pBuf[nAllLen+verifyDataLen]=0;

    /**debug info**/
    if(DEBUGFLAG) {
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

/**������**/
int i_shc_handler(evbuffer *buf, char *fullurl) {
  int ret = -1;

  if(!bInitFlag) {
    srand(time(NULL));
    ret=ReadConfigFile("/zhongsou/i/a/myig_module.ini");

    if(ret==0) {
      sprintf(I_MSP_CGIPATH,"/a/");
      sprintf(I_MSP_IMAGEPATH,"http://i0.zhongso.com/a/img_css/");
      sprintf(I_MSP_CSSANDJSPATH,"/a/img_css/");
      bInitFlag=true;
    } else {
      evbuffer_add_printf(buf,"<!--���ܶ�ȡ�����ļ�-->");
      return ret;
    }

    proBlockChange = new CProBlockChange("/zhongsou/i/a/blockchange.dat");
    if(proBlockChange -> ReadChangeFileResult != 0) {
      evbuffer_add_printf(buf,"<!--��ʼ������������ļ�ʧ��-->");
      //return OK;
    }
  }

  URLPARAMETER *para = new URLPARAMETER;
  ret = SetUrlParameters(para, fullurl);
  if(ret < 0) {
    evbuffer_add_printf(buf,"<!--��ȡ����ʧ��-->");
    delete para;
    return OK;
  }
  //imsp���
  if(para->s == 1 && para->ds == 1) {
    I_MSP_PrintfHtmlPage(buf, para->u);
    delete para;
    return OK;
  }

  //����socket
  int connIpIndex = 0;
  if(SERVERIPMAX > 1) { connIpIndex = rand()%SERVERIPMAX; }
  bool *ipStatus = new bool[SERVERIPMAX];
  for(int i=0;i<SERVERIPMAX;i++) { ipStatus[i] = true; }

  int pSocket = RandConnectServer(connIpIndex,SERVERIPMAX,pServerIP,ipStatus);
  /**debug info**/
  if(DEBUGFLAG) { fprintf(stderr,"pSocket:%d\n",pSocket); }
  /**debug info**/

  char strTmp[500]={0};

  if(pSocket<0) {
    sprintf(strTmp,"��ʱ���緱æ,������!!!\n");
    I_MSP_PrintfErrorHtmlPage(buf,strTmp,para->u);
    delete para;
    return OK;
  }

  char *pBuf=NULL;
  char errorStr[256];
  bool reGetFlag = false;

  GETHTML:
  //ͨ��Э��ȡ����
  memset(errorStr,0,256);
  int iRet=GetMakeHtmlPage(pSocket,para,pBuf,errorStr);

  //��ӡ��Ϣ
  if(para->s == 1 || (para->s != 1 && para->a == 1)) { //imsp �� ������
    if(iRet > -10000 && iRet < -5000) {
      if(iRet == -5001 || iRet == -5002) {
        ipStatus[connIpIndex] = false;
        if(SERVERIPMAX > 1) {
          connIpIndex = rand()%SERVERIPMAX;
          close(pSocket);
          pSocket = RandConnectServer(connIpIndex,SERVERIPMAX,pServerIP,ipStatus);
          if(pSocket<0) {
            sprintf(strTmp,"re1-��ʱ���緱æ��������!!!\n");
            I_MSP_PrintfErrorHtmlPage(buf,strTmp,para->u);
	    delete para;
            return OK;
          }
          goto GETHTML;
        }
      }

      sprintf(strTmp,"%s��������(%d)��",errorStr,iRet);
      I_MSP_PrintfErrorHtmlPage(buf,strTmp,para->u);
    } else if(iRet>0) {
      int dataLenPos;
      /*if(para->a == 1)
      {
        dataLenPos = 10 + sizeof(int)*4 + sizeof(char)*2;
        unsigned char md5Len[10] = {0};
        memcpy(md5Len,pBuf + dataLenPos,8);
        char outMd5[20] = {0};

        for(int i=0;i<8;i++)
          sprintf(outMd5+i*2,"%02x",md5Len[i]);
        evbuffer_add_printf(stderr,"%s\n",outMd5);

      }
      else*/
      {
        if(para->s == 1) {
          dataLenPos = 10 + sizeof(int)*8 + sizeof(char)*2;
        } else {
          dataLenPos = 10 + sizeof(int)*10 + sizeof(char)*3 + sizeof(short)*2 + strlen(para->u);
        }
        /**debug info**/
        if(DEBUGFLAG) { fprintf(stderr,"dataLenPos:%d\n",dataLenPos); }
        /**debug info**/

        char strDataLen[10] = {0};
        memcpy(strDataLen,pBuf + dataLenPos,4);
        int dataLen = (int)*(int*)strDataLen;
        if(dataLen > 0) {
          dataLenPos += 4;
          evbuffer_add_printf(buf, "%s", pBuf+dataLenPos);
        } else {
          sprintf(strTmp,"ȡ����ʧ��<!--(%ld)-->������",dataLen);
          I_MSP_PrintfErrorHtmlPage(buf,strTmp,para->u);
        }
      }
    } else {
      if(para->a == 1) {
        evbuffer_add_printf(buf,"<!--%s-->\n",errorStr);
      } else {
        sprintf(strTmp,"û���ҵ���%s ��Ӧ����ҳ����ȷ�ϸ�URL�Ƿ���ȷ������,%s",para->u,errorStr);
        I_MSP_PrintfErrorHtmlPage(buf,strTmp,para->u);
      }
    }
  } else  { // ��ӡ���������
    if(iRet==-5001) {
      I_SHC_36_PrintfErrorHtmlPage(buf,"��Ϊ����ǰ���ʵ���ҳ��ʱ�������ԡ�\n",iRet,errorStr);
    } else if(iRet>0) {
      int dataLenPos = 10 + sizeof(int)*8 + sizeof(short)*4 + sizeof(char)*2 + strlen(para->u);
      char strDataLen[10] = {0};
      memcpy(strDataLen,pBuf + dataLenPos,4);
      int statusCode = (int)*(int*)strDataLen;
      dataLenPos += sizeof(int);
      memcpy(strDataLen,pBuf + dataLenPos,4);
      int errorCodeTime = (int)*(int*)strDataLen;
      dataLenPos += sizeof(int);
      memcpy(strDataLen,pBuf + dataLenPos,4);
      int errorCode = (int)*(int*)strDataLen;
      dataLenPos += sizeof(int);

      if(para->f > 1 && errorCode < 0) {
        if(pBuf!=NULL) {
          delete[] pBuf;
          pBuf=NULL;
        }
        reGetFlag = true;
        para->f = 0;
        /**debug info**/
        if(DEBUGFLAG) { fprintf(stderr,"para->f > 2 :errorCode:%ld\n",errorCode); }
        /**debug info**/
        close(pSocket);
        pSocket = RandConnectServer(connIpIndex,SERVERIPMAX,pServerIP,ipStatus);

        /**debug info**/
        if(DEBUGFLAG) { fprintf(stderr,"re2-pSocket:%d\n",pSocket); }
        /**debug info**/

        if(pSocket<0) {
          sprintf(strTmp,"re2-�����������з�������������!!!\n");
          I_MSP_PrintfErrorHtmlPage(buf,strTmp,para->u);
	  delete para;
          return OK;
        }
        goto GETHTML;
      }
      /**debug info**/
      if(DEBUGFLAG) { fprintf(stderr,"errorCode:%ld -- statusCode:%ld\n",errorCode,statusCode); }
      /**debug info**/
      if(statusCode == 0) { // ����ȡ������
        if(errorCode > 0) {
          evbuffer_add_printf(buf, "%s\n", pBuf+dataLenPos);
          evbuffer_add_printf(buf, "<!--errorCode:%ld,statusCode:%ld-->",
                              errorCode, statusCode);
        } else {
          I_SHC_36_PrintfErrorHtmlPage(buf,"ȡ������ģ��ʧ�ܣ���",errorCode,errorStr);
        }
      } else {
        //CProBlockChange *proBlockChange = new CProBlockChange("/zhongsou/i/a/blockchange.dat");
        char outBuf[12000] = {0};
        char inTime[50] = {0};
        char *urlHost = new char[50];
        int ghr = GetUrlHost(para->u,urlHost);

        proBlockChange -> GetChangeTime(errorCodeTime,inTime);

        if(errorCode > 0) {
          if(statusCode == 1) {
            proBlockChange->GetHavePageHaveCache(inTime,para->u,outBuf,12000);
          } else if(statusCode == 2) {
            if(ghr != 0) {
              proBlockChange->GetNoPageHaveCache(inTime,para->u,outBuf,12000);
            } else {
              proBlockChange->GetNoPageHaveCache(inTime,urlHost,outBuf,12000);
            }
            evbuffer_add_printf(buf, "<!--%d-->\n",ghr);
            evbuffer_add_printf(buf, "<!--111:%s-->\n",proBlockChange -> szNoPageHaveCache);
            evbuffer_add_printf(buf, "<!--222:%s-->\n",outBuf);
          } else if(statusCode == 3) {
            proBlockChange->GetNoDownPageHaveCache(inTime,para->u,outBuf,12000);
          }

          int indiLen = strlen(outBuf);
          //static int temLen = errorCode + indiLen;
          int temLen = errorCode + indiLen;

          if(para->f > 1) {
            evbuffer_add_printf(buf,"%s",pBuf+dataLenPos);
          } else {
            char *merOutBuf = new char[temLen];
            memcpy(merOutBuf,pBuf+dataLenPos,errorCode);
            memcpy(merOutBuf+errorCode,outBuf,indiLen);
            *(merOutBuf+temLen-1) = 0;

            /**debug info**/
            if(DEBUGFLAG) {
              fprintf(stderr,"statusCode:%d--errorCode:%d--time:%ld--otime:%s\n",
                      statusCode,errorCode,errorCodeTime,inTime);
            }
            /**debug info**/

            evbuffer_add_printf(buf, "%s",merOutBuf);
            evbuffer_add_printf(buf, "<!--errorCode:%ld,statusCode:%ld-->",
                                errorCode, statusCode);
            if(merOutBuf!=NULL) {
              delete[] merOutBuf;
              merOutBuf=NULL;
            }
          }
        } else {
          if(statusCode == 1) {
            proBlockChange -> GetHavePageNoCache(para->u,outBuf,12000);
          } else if(statusCode == 2) {
            if(ghr != 0) {
              proBlockChange -> GetNoPageNoCache(para->u,outBuf,12000);
            } else {
              proBlockChange -> GetNoPageNoCache(urlHost,outBuf,12000);
            }
          } else if(statusCode == 3) {
            proBlockChange -> GetNoDownPageNoCache(para->u,outBuf,12000);
          }
          evbuffer_add_printf(buf, "%s",outBuf);
          evbuffer_add_printf(buf, "<!--errorCode:%ld,statusCode:%ld-->",
                              errorCode, statusCode);
        }

        if(urlHost != NULL) {
          delete[] urlHost;
          urlHost=NULL;
        }
      }
    } else {
      I_SHC_36_PrintfErrorHtmlPage(buf,"ȡ������ģ��ʧ�ܣ���",iRet,errorStr);
    }
  }

  evbuffer_add_printf(buf, "<!--connserver:%d-->\n", connIpIndex);
  for(int i=0;i<SERVERIPMAX;i++) {
    if(!ipStatus[i]) {
      evbuffer_add_printf(buf, "<!--errorserver:%d-->\n",i);
    }
  }

  if(pBuf!=NULL) {
    delete[] pBuf;
    pBuf=NULL;
  }
  if(ipStatus != NULL) {
    delete[] ipStatus;
    ipStatus=NULL;
  }

  close(pSocket);
  delete para;
  return OK;
}
