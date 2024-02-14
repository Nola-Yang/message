#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

#include "4g_serialport.h"
#include "4g_at.h"
#include "logger.h"

int excute_center_num=1;		//excute get_center_number and handle_center_number once,then =0;

int send_at(serialport_ctx_t *sp,char *at,char *expect,char *buf,int bufsize,int timeout)		//把发AT，收AT封装到一起
{
	char		rc_msg[10000];

	if(at==NULL||expect==NULL)
	{
		log_error("the two arguments(at and expect) can't be NULL\n");
		return -1;
	}

	if(serialport_send(sp,at,strlen(at))<0)
	{
		log_error("send AT commond failure:%s\n",strerror(errno));
		return -2;
	}

	//sleep(4);

	if(serialport_receive(sp,rc_msg,sizeof(rc_msg),timeout)<0)
	{
		log_error("receive back message failure:%s\n",strerror(errno));
		return -3;
	}

	if(!strstr(rc_msg,expect))
	{
		log_error("can't find the message that you expect in AT back message\n");
		return -4;
	}

	if(buf)
	{
		strncpy(buf,rc_msg,bufsize);
	}
	return 0;
}	

/*	查模块的MCC和MNC	 */
int query_module_mcc_and_mnc(serialport_ctx_t *sp,char *mcc,char *mnc)
{
	int				rv=-1;
	char			buf[512];
	char			*ptr=NULL;
	char			*delim="\",\"";
	char			*p1=NULL;


	if((rv=send_at(sp,"AT+QNWINFO\r","OK",buf,sizeof(buf),2))<0)
	{
		log_error("query module mcc and mnc failure.\n");
		return -1;
	}
	log_info("query module mcc and mnc successfully!\n");

	ptr=strstr(buf,"CDMA1X");

	//log_info("ptr=%s\n",ptr);

	ptr+=9;

	//log_info("ptr=%s\n",ptr);

	snprintf(mcc,4,"%s",ptr);

	ptr+=3;

	snprintf(mnc,3,"%s",ptr);


	return 0;
}



/*	设置模块拨号上网的APN	      */
int set_module_apn(serialport_ctx_t *sp,char *apn)
{
	int             rv=-1;
	char			buf[64];

	snprintf(buf,sizeof(buf),"AT+CGDCONT=1,\"IP\",\"%s\"\r",apn);	//AT+CGDCONT=1,"IP","CTNET"

	if((rv=send_at(sp,buf,"OK",NULL,0,2))<0)
	{
		log_error("Set module apn failure.\n");
		return -1;
	}
	log_info("Set 4G module apn successfully!\n");

	return 0;
}


int check_gsm_ready(serialport_ctx_t *sp)		//发送AT后实际发送的指令是：AT<CR>也就是“AT\r”，而我收到的OK其实是：<CR><LF><OK><CR><LF>，也就是“\r\nOK\r\n”
{
	int	rv=-1;

	if((rv=send_at(sp,"AT\r","OK",NULL,0,2))<0)
	{
		log_error("GSM can't be used\n");
		return -1;
	}
	log_info("GSM is ready to be used\n");

	return 0;
}	

/*	模块识别	*/
int check_module_type(serialport_ctx_t *sp,char *module_type)
{
	int             rv=-1;

	if((rv=send_at(sp,"AT+CGMM\r",module_type,NULL,0,2))<0)
	{
		log_error("Check module type '%s' failure.\n",module_type);
		return -1;
	}
	log_info("Check  module type '%s' successfully!\n",module_type);

	return 0;
}


int check_sim_present(serialport_ctx_t *sp)		//AT+CPIN? 查看SIM卡是否正常，返回ready则表示正常
{
	int	rv=-1;

	if((rv=send_at(sp,"AT+CPIN?\r","READY",NULL,0,2))<0)
	{
		log_error("can't find SIM card\n");
		return -1;
	}
	log_info("Find SIM card successfully\n");
	return 0;
}

int check_sim_register(serialport_ctx_t *sp)		//AT+CREG? 查询模块是否注册上GSM网络，返回+GREG：0,0表示未注册，终端在搜寻新的运营商，+GREG：0,1表示注册本地网（正常），+GREG：0,2表示未注册，终端正在搜寻基站，+GREG：0,3表示SIM卡注册被拒绝，+GREG：0,4未知错误
{
	int	rv=-1;

	if((rv=send_at(sp,"AT+CREG?\r","0,1",NULL,0,2))==0)
	{
		log_info("SIM card register in GSM network OK\n");		//注册本地网络正常
		return 0;
	}

	if((rv=send_at(sp,"AT+CREG?\r","0,0",NULL,0,2))==0)
	{
		log_error("SIM card didn't register\n");
		return -1;
	}

	if((rv=send_at(sp,"AT+CREG?\r","0,2",NULL,0,2))==0)
	{
		log_error("SIM card didn't register and looking for base station\n");
		return -2;
	}

	if((rv=send_at(sp,"AT+CREG?\r","0,3",NULL,0,2))==0)
	{
		log_error("SIM card registration denied\n");
		return -3;
	}

	if((rv=send_at(sp,"AT+CREG?\r","0,4",NULL,0,2))==0)
	{
		log_error("unknow error\n");
		return -4;
	}

	return 0;
}

int check_gsm_signal(serialport_ctx_t *sp)		//AT+CSQ查询信号强度，第一个值正常范围为16-31，99为不正常
{
	int	rv=-1;
	char	buf[64];
	char	*ptr=NULL;
	char	*msg=NULL;
	int	num=0;

	if((rv=send_at(sp,"AT+CSQ\r","+CSQ",buf,sizeof(buf),2))<0)
	{
		log_error("can't check signal problem\n");
		return -1;
	}

	if((ptr=strstr(buf,","))==NULL)		//例:+CSQ 18,99. 目的是获取18
		return -2;

	ptr-=2;

	msg=strtok(ptr,",");

	num=atoi(msg);

	if(num>=16&&num<=31)
	{
		log_info("The signal is OK!\n");
		return 0;
	}
	else
	{
		log_error("the signal is too low or not right\n");
		return -3;
	}

	return 0;
}

int check_gsm_all_ready(serialport_ctx_t *sp)
{
	if(check_gsm_ready(sp)!=0)
		return GSM_MODULE_NOTREADY;

	if(check_sim_present(sp)!=0)
		return GSM_SIM_NOTREADY;

	if(check_sim_register(sp)!=0)
		return GSM_SIM_REGISTER_NOTREADY;

	if(check_gsm_signal(sp)!=0)
		return GSM_SIM_SIGNAL_NOTGOOD;


	return GSM_ALL_READY;
}

/*	获取中心号码	  */
int get_center_number(serialport_ctx_t *sp,char *center_num)	
{
	int	rv=-1;
	char	buf[64];
	char	*ptr=NULL;
	char	*delim="\"";
	char	*p1=NULL;

	if((rv=send_at(sp,"AT+CSCA?\r","CSCA",buf,sizeof(buf),2))!=0)		//AT+CSCA?
	{
		log_error("can't find center number\n");			//+CSCA: "+8617386070887",145
		return -1;							
	}									//OK	

	ptr=strstr(buf,"8");
	p1=strtok(ptr,delim);

	strcpy(center_num,p1);	

	//log_info("the center number is '%s'\n",gsm->center_num);

	return 0;
}

/*	传你想发短信的号码和内容	*/
int send_sms_text(serialport_ctx_t *sp,char *phone_num,char *sms)
{
	int     rv=-1;
	char    buf[64];
	char    buf2[64];

	if((rv=send_at(sp,"AT+CMGF=1\r","OK",NULL,0,2))!=0)
	{
		//printf("set SMS format to text failure:%s\n",strerror(errno));
		log_error("set SMS format to text failure:%s\n",strerror(errno));
		return -1;
	}

	snprintf(buf,sizeof(buf),"AT+CMGS=\"%s\"\r",phone_num);

	if((rv=send_at(sp,buf,">",NULL,0,2))<0)
	{
		//printf("send AT+CMGS='phone_num' failure or can't receive '>'\n");
		log_error("send AT+CMGS='phone_num' failure or can't receive '>'\n");
		return -2;
	}

	snprintf(buf2,sizeof(buf2),"%s\x1a",sms);  //ctrl+z 发出短信内容，放在要发的内容后

	if((rv=send_at(sp,buf2,"OK",NULL,0,4))<0)
	{
		//printf("send SMS(text) failure\n");
		log_error("send SMS(text) failure\n");
		return -3;
	}
	//printf("send SMS(text) successfully!\n");
	log_info("send SMS(text) successfully!\n");

	return 0;
}

/*	UTF-8 -> Unicode , LSB	     */
int utf8_to_unicode(char* utf8_buf,char* unic_buf)
{
	if(!utf8_buf)
	{
		printf("Invalid parameter\n");
		return -1;
	}
	char *temp = unic_buf;

	char b1,b2,b3,b4;  //b1: high data bit   b4: low data bits

	while(*utf8_buf)
	{
		if(*utf8_buf > 0x00 && *utf8_buf <= 0x7E)  //Single byte
		{
			*temp = 0;
			temp++;
			*temp = *utf8_buf;
			temp++;
			utf8_buf++;  //Next unprocessed character
		}
		else if(((*utf8_buf) & 0xE0) == 0xC0)  //Double bytes
		{
			b1 = *utf8_buf;
			b2 = *(utf8_buf+1);

			if((b2 & 0xC0) != 0x80)  //Check the legality of characters,Double bytes of UTF-8: 110xxxxx 10xxxxxx
				return -1;

			*temp = (b1 >> 2) & 0x07;
			temp++;
			*temp = (b1 << 6) + (b2 & 0x3F);
			temp++;
			utf8_buf+=2;
		}
		else if(((*utf8_buf) & 0xF0) == 0xE0)  //Three bytes
		{
			b1 = *utf8_buf;
			b2 = *(utf8_buf+1);
			b3 = *(utf8_buf+2);
			if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )  //Check the legality of characters,1110xxxx 10xxxxxx 10xxxxxx
				return -1;

			*temp = (b1 << 4) + ((b2 >> 2) & 0x0F);
			temp++;
			*temp = (b2 << 6) + (b3 & 0x3F);
			temp++;
			utf8_buf+=3;
		}
		else if(*utf8_buf >= 0xF0 && *utf8_buf < 0xF8) //Four bytes
		{
			b1 = *utf8_buf;
			b2 = *(utf8_buf+1);
			b3 = *(utf8_buf+2);
			b4 = *(utf8_buf+3);
			if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) )  //11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				return -1;
			*temp = ((b1 << 2) & 0x1C)  + ((b2 >> 4) & 0x03);
			temp++;
			*temp = (b2 << 4) + ((b3 >> 2) & 0x0F);
			temp++;
			*temp = (b3 << 6) + (b4 & 0x3F);

			temp++;
			utf8_buf+=4;
		}
		else
			return -1;

	}

	/* Add FFFE at the end */
	*temp = 0xFF;
	temp++;
	*temp = 0xFE;

	return 0;
}

/*	字节流转字符流	     */
int Hex2Str( const char *sSrc,  char *sDest, int nSrcLen )
{
	int              i;
	char             szTmp[3];

	if(!sSrc || !sDest || nSrcLen <= 0)
	{
		printf("Unable to transcode Hex to String,Invalid parameter.\n");
		return -1;
	}

	for( i = 0; i < nSrcLen; i++ )
	{
		if(sSrc[i] != 0xFF && sSrc[i+1] != 0xFE)  //0xFF 0xFE is the end of Unicode
		{
			sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );
			memcpy( &sDest[i * 2], szTmp, 2 );
		}
		else
			break;
	}
	return 0;
}

/*	处理中心号码	  */
int handle_center_number(char *center_num)		
{						//1.尾部+F
	int	i;				//2.奇偶位交换并在首部添加0891	
	int	temp;				
	char 	head[64]="0891";		//如果这里的head没有赋初值的话,下面的strcat会segmentation fault,因为字符串数组空间不够放下center_num!!!

	strcat(center_num,"F");			

	for(i=0;i<14;i+=2)			
	{
		temp=center_num[i];
		center_num[i]=center_num[i+1];
		center_num[i+1]=temp;
	}

	strcat(head,center_num);		//在head后面放center_num
	strcpy(center_num,head);		//把head copy 到center_num!	

	return 0;
}

/*	处理电话号码	  */
int handle_phone_number(char *phone_num)		
{								
	int 	i;
	int		temp;
	char	add[128]="86";
	char	head[64]="11000D91";
	char	tail[64]="000800";
	char	phone_num_buf1[1024];
	char	phone_num_buf2[1024];

	snprintf(phone_num_buf1,sizeof(phone_num_buf1),"%s%s%s",add,phone_num,"F");	//1.在phone_num首部+‘86’，尾部+‘F’
	strcpy(phone_num,phone_num_buf1);

	for(i=0;i<14;i+=2)									//2.奇偶位交换
	{
		temp=phone_num[i];
		phone_num[i]=phone_num[i+1];
		phone_num[i+1]=temp;
	}

	snprintf(phone_num_buf2,sizeof(phone_num_buf2),"%s%s%s",head,phone_num,tail);	//3.首部+11000D91，尾部+000800
	strcpy(phone_num,phone_num_buf2);

	return 0;
}

/*	处理unicode短信内容	  */
int handle_unicode(char *sms)
{
	char	len[1024];

	sprintf(len,"%.2x",strlen(sms)/2);	//1.把要发的unicode格式的中文的长度/2后转为16进制，如12为 0c

	strcat(len,sms);			//2.将16进制数放在其首部
	strcpy(sms,len);

	return 0;
}

/*	发pdu短信	*/
int send_sms_pdu(serialport_ctx_t *sp,char *center_num,char *phone_num,char *sms)               //1.unicode_sms 是中文转的unicode
{                                                                                               //2.center_num 是用get_number函数获取的中心号码
	int     rv=0;                                                                           //3.phone_num 是自定义变量,如"8617687499243"
	int     half=0; //(phone_num+unicode_sms)/2
	char    pdu_buf[1024];
	char    cmgs_buf[1024];
	char    PhoneNum_add_Sms_buf[1024];
	char    CenterNum_add_PhoneNum_add_Sms_buf[1024];

	if((rv=send_at(sp,"AT+CMGF=0\r","OK",NULL,0,2))<0)
	{
		//printf("pdu sms:send 'AT+CMGF=0' failure or didn't receive 'OK'\n");
		log_error("pdu sms:send 'AT+CMGF=0' failure or didn't receive 'OK'\n");
		return -1;
	}

	/*      获取中心号跟处理中心号都只执行一次      */
	if(excute_center_num)
	{
		get_center_number(sp,center_num);

		handle_center_number(center_num);

		excute_center_num=0;
	}

	handle_phone_number(phone_num);

	handle_unicode(sms);

	/*      把处理好的目标电话号码和短信内容放一起，求其长度的一半      */
	snprintf(PhoneNum_add_Sms_buf,sizeof(PhoneNum_add_Sms_buf),"%s%s",phone_num,sms);

	half=strlen(PhoneNum_add_Sms_buf)/2;

	/*      把处理好的中心号、目标电话号码和短信内容放一起      */
	snprintf(pdu_buf,sizeof(pdu_buf),"%s%s\x1a",center_num,PhoneNum_add_Sms_buf);

	//log_info("pdu_buf=%s\n",pdu_buf);

	snprintf(cmgs_buf,sizeof(cmgs_buf),"AT+CMGS=%d\r",half);
	if((rv=send_at(sp,cmgs_buf,">",NULL,0,2))<0)
	{
		//printf("pdu sms:send 'AT+CMGS=half' failure or didn't receive '>'\n");
		log_error("pdu sms:send 'AT+CMGS=half' failure or didn't receive '>'\n");
		return -2;
	}

	if((rv=send_at(sp,pdu_buf,"OK",NULL,0,4))<0)
	{
		//printf("send SMS(pdu) failure\n");
		log_error("send SMS(pdu) failure\n");
		return -3;
	}
	//printf("send SMS(pdu) successfully!\n");
	log_info("send SMS(pdu) successfully!\n");

	return 0;
}

/*
   int send_sms_pdu(serialport_ctx_t *sp,char *center_num,char *phone_num,char *sms)		//1.unicode_sms 是中文转的unicode
   {												//2.center_num 是用get_number函数获取的中心号码		
   int	rv=0;										//3.phone_num 是自定义变量,如"8617687499243"
   int	half=0;	//(phone_num+unicode_sms)/2	
   char	pdu_buf[1024];
   char	cmgs_buf[1024];
   char	PhoneNum_add_Sms_buf[1024];	
   char	CenterNum_add_PhoneNum_add_Sms_buf[1024];

   char    str_unicode[256] = {0};
   char    unicode_buf[256] = {0};


   if((rv=send_at(sp,"AT+CMGF=0\r","OK",NULL,0,2))<0)
   {
//printf("pdu sms:send 'AT+CMGF=0' failure or didn't receive 'OK'\n");
log_error("pdu sms:send 'AT+CMGF=0' failure or didn't receive 'OK'\n");
return -1;
}

if(utf8_to_unicode(sms,unicode_buf) != 0)
{
printf("UTF-8 to Unicode failed,Check your input.\n");
return -2;
}

if(Hex2Str(unicode_buf,str_unicode,256) != 0)
{
printf("Hex to String failed.\n");
return -3;
}


//	获取中心号跟处理中心号都只执行一次	
if(excute_center_num)
{
get_center_number(sp,center_num);	

handle_center_number(center_num);

excute_center_num=0;
}

handle_phone_number(phone_num);	

handle_unicode(str_unicode);

//	把处理好的目标电话号码和短信内容放一起，求其长度的一半	    	
snprintf(PhoneNum_add_Sms_buf,sizeof(PhoneNum_add_Sms_buf),"%s%s",phone_num,str_unicode);

half=strlen(PhoneNum_add_Sms_buf)/2;

//	把处理好的中心号、目标电话号码和短信内容放一起	    	
snprintf(pdu_buf,sizeof(pdu_buf),"%s%s\x1a",center_num,PhoneNum_add_Sms_buf);

//log_info("pdu_buf=%s\n",pdu_buf);

snprintf(cmgs_buf,sizeof(cmgs_buf),"AT+CMGS=%d\r",half);
if((rv=send_at(sp,cmgs_buf,">",NULL,0,2))<0)
{
//printf("pdu sms:send 'AT+CMGS=half' failure or didn't receive '>'\n");
log_error("pdu sms:send 'AT+CMGS=half' failure or didn't receive '>'\n");
return -2;
}

if((rv=send_at(sp,pdu_buf,"OK",NULL,0,2))<0)
{
//printf("send SMS(pdu) failure\n");
log_error("send SMS(pdu) failure\n");
return -3;
}
//printf("send SMS(pdu) successfully!\n");	
log_info("send SMS(pdu) successfully!\n");	

return 0;	
}
*/

/*
   int set_ring(serialport_ctx_t *sp)		//设置来电显示ring
   {
   int	rv=-1;

   if((rv=send_at(sp,"AT+CLCC\r","CLCC",NULL,0,2))<0)
   {
//printf("set show ring failure\n");
log_error("set show ring failure\n");
return -1;
}
//printf("set show ring successfully!\n");
log_info("set show ring successfully!\n");

return 0;
}

int call_phone(serialport_ctx_t *sp,gsm_ctx_t *gsm)
{
int	rv=-1;
char	phone_buf[64]="ATD";

strcat(phone_buf,gsm->phone_num);		//即ATD17687499243
strcat(phone_buf,";\r");			//即ATD17687499243;\r

if((rv=send_at(sp,phone_buf,"OK",NULL,0,2))<0)
{
//printf("make a phone failure\n");	
log_error("make a phone failure\n");	
return -1;
}
//printf("make a phone successfully,waiting for answer...\n");
log_info("make a phone successfully,waiting for answer...\n");

return 0;
}

int answer_phone(serialport_ctx_t *sp)
{
int	rv=-1;

if((rv=send_at(sp,"ATA\r","OK",NULL,0,2))<0)
{
//printf("answer phone failure\n");
log_error("answer phone failure\n");
return -1;
}
//printf("answer phone successfully!\n");
log_info("answer phone successfully!\n");

return 0;
}

int hangup_phone(serialport_ctx_t *sp)
{
int	rv=-1;
char 	*ptr=NULL;

if((rv=send_at(sp,"ATH\r","OK",NULL,0,2))<0)
{
//printf("hang up phone failure\n");
log_error("hang up phone failure\n");
return -1;
}
//printf("hang up phone successfully!\n");
log_info("hang up phone successfully!\n");

return 0;
}
*/

/*      根据 index 删除单条 sms      */
int delete_a_sms(serialport_ctx_t *sp,int index)
{
	int     rv=-1;
	char    cmd[32];

	sprintf(cmd,"AT+CMGD=%d\r",index);

	if((rv=send_at(sp,cmd,"OK",NULL,0,2))<0)
	{
		log_error("Delete SMS failure.\n");
		return -1;
	}
	log_info("Delete SMS successfully!\n");

	return 0;
}

/*	读 ”rec unread“ sms	 */
int read_sms(serialport_ctx_t *sp,char *phone_num,int index)		//read 'rec unread' sms,then delete.
{
	int		rv=-1;
	//int	index;
	char	buf[1024];
	char	*ptr=NULL;
	int		compare_flag=0;
	char	compare_number[12];
	char	*sms_start=NULL;
	char	*sms_end=NULL;
	char	sms_buf[256];

	if((rv=send_at(sp,"AT+CMGF=1\r","OK",NULL,0,2))!=0)		//only set AT+CMGF=1,then can AT+CMGL="ALL"
	{
		log_error("set SMS format to text failure:%s\n",strerror(errno));
		return -1;
	}

	memset(buf,0,sizeof(buf));

	if((rv=send_at(sp,"AT+CMGL=\"ALL\"\r","OK",buf,sizeof(buf),2))<0)
	{
		log_error("Read SMS failure.\n");
		return -1;
	}


	if(strstr(buf,"+CMGL")==NULL)
	{
		log_info("There haven't new sms\n");
		return 0;
	}
	ptr=buf;

	while((ptr=strstr(ptr,"\",\""))!=NULL)
	{
		ptr+=3;

		snprintf(compare_number,12,ptr);

		//log_info("compare_number=%s\n",compare_number);
		//log_info("phone_num=%s\n",phone_num);

		if(strcmp(compare_number,phone_num) == 0)
		{
			compare_flag = 1;
			break;
		}
		else
		{
			continue;
		}

	}

	index=atoi(ptr-17);		//index 值,最多只能存99条短信，所以index为个位数时，我预留了一个空格，保证了能有两位数字
	//log_info("index=%d\n",index);

	if(!compare_flag)
	{
		log_info("There is no message for %s\n",phone_num);
		return 0;
	}

	sms_start = strstr(ptr,"\r\n");		//碰到换行的下两位就是短信内容
	sms_start+=2;

	sms_end = strstr(sms_start,"\r\n");	//再次碰到换行就是短信内容的结束

	strncpy(sms_buf,sms_start,(int)(sms_end - sms_start));

	log_info("Read SMS from '%s':%s\n",phone_num,sms_buf);

	return 0;
}


/*	socket 与 PC connect上后，查询一下模块里有没有短信，若有，则发给PC       */
int query_sms(serialport_ctx_t *sp,char *pc_sms)
{
	int			rv=-1;
	char		*ptr=NULL;
	char    	*sms_start=NULL;
	char    	*sms_end=NULL;
	char		unsend_sms[1024];

	if( (rv=send_at(sp,"AT+CPMS=\"SM\",\"SM\",\"SM\"\r","OK",NULL,0,2))!=0 )
	{
		log_error("set SMS storage carrier failure:%s\n",strerror(errno));
		return -1;
	}
	log_info("set SMS storage carrier successfully!\n");

	if( (rv=send_at(sp,"AT+CMGF=1\r","OK",NULL,0,2))<0 )            	 	//only set AT+CMGF=1,then can AT+CMGL="ALL"
	{
		log_error("set SMS format to text failure:%s\n",strerror(errno));
		//return -1;
	}

	memset(unsend_sms,0,sizeof(unsend_sms));
	if( (rv=send_at(sp,"at+cmgl=\"all\"\r","OK",unsend_sms,sizeof(unsend_sms),2))<0 )
	{
		log_error("Read SMS failure.\n");
		return -1;
	}

	//log_info("pc_sms=%s\n",pc_sms);

	if(strstr(unsend_sms,"+CMGL")==NULL)
	{
		log_info("There haven't new sms in module.\n");
		return -1;
	}
	/*
	   ptr=strstr(pc_sms,"+CMGL");
	   strcpy(pc_sms,ptr);
	   */

	ptr=strstr(unsend_sms,"+CMGL");
	sms_start = strstr(ptr,"\r\n");         //碰到换行的下两位就是短信内容

	sms_start+=2;

	sms_end = strstr(sms_start,"\r\n");     //再次碰到换行就是短信内容的结束

	strncpy(pc_sms,sms_start,(int)(sms_end - sms_start));
	//log_info("pc_sms=%s\n",pc_sms);

	log_info("There have new sms in module.\n");

	return 0;
}

/*	删除所有短信	  */
int delete_all_sms(serialport_ctx_t *sp)
{
	int		rv=-1;

	if((rv=send_at(sp,"AT+CMGD=1,4\r","OK",NULL,0,2))<0)		
	{
		log_error("Delete all SMS failure.\n");
		return -1;
	}
	log_info("Delete all SMS successfully!\n");

	return 0;	
}





