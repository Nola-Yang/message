#ifndef _AT_H_
#define _AT_H_

enum status
{
	GSM_SERIALPORT_TYPE_NOTEXIST,
	GSM_MODULE_NOTREADY,
	GSM_SIM_NOTREADY,
	GSM_SIM_REGISTER_NOTREADY,
	GSM_SIM_SIGNAL_NOTGOOD,
	GSM_ALL_READY,
};

enum type
{
	SMS_TYPE_TEXT,
	SMS_TYPE_PDU,
};

typedef struct gsm_ctx_s
{
	char	json_buf[128];
	char	sms[128];
	char	pc_sms[128];
	char	center_num[128];
	char	phone_num[128];
	int		type;
	int		status;
	int		index;
	int		set_apn_flag;
	int 	have_data_in_gsm_database;
}gsm_ctx_t;

extern int send_at(serialport_ctx_t *sp,char *at,char *expect,char *buf,int bufsize,int timeout);         //把发AT，收AT封装到一起
extern int query_module_mcc_and_mnc(serialport_ctx_t *sp,char *mcc,char *mnc);
extern int set_module_apn(serialport_ctx_t *sp,char *apn);

extern int check_gsm_ready(serialport_ctx_t *sp);	//发送AT后实际发送的指令是：AT<CR>也就是“AT\r”，而我收到的OK其实是：<CR><LF><OK><CR><LF>，也就是“\r\nOK\r\n”	
extern int check_module_type(serialport_ctx_t *sp,char *module_type);
extern int check_sim_pretent(serialport_ctx_t *sp);	//AT+CPIN? 查看SIM卡是否正常，返回ready则表示正常
extern int check_sim_register(serialport_ctx_t *sp);	//AT+CREG? 查询模块是否注册上GSM网络，返回+GREG：0,0表示未注册，终端在搜寻新的运营商，+GREG：0,1表示注册本地网络
extern int check_gsm_signal(serialport_ctx_t *sp);             //AT+CSQ查询信号强度，第一个值正常范围为16-31，99为不正常
extern int check_gsm_all_ready(serialport_ctx_t *sp);

extern int get_center_number(serialport_ctx_t *sp,char *center_num);      //获取中心号码
//extern int send_sms_text(serialport_ctx_t *sp,gsm_ctx_t *gsm);          //传你想发短信的号码和内容
extern int send_sms_text(serialport_ctx_t *sp,char *phone_num,char *sms);          //传你想发短信的号码和内容

extern int utf8_to_unicode( char* utf8_buf, char* unic_buf );
extern int Hex2Str( const char *sSrc, char *sDest, int nSrcLen );

extern int handle_center_number(char *center_num);		//处理中心号码
extern int handle_phone_number(char *phone_num);           //这里传的phone_num要加86,如8617687499243
extern int handle_unicode(char *sms);		//传要发的中文转的unicode

extern int send_sms_pdu(serialport_ctx_t *sp,char *center_num,char *phone_num,char *sms);	//把前面三个函数封装到一起,完成发送流程

extern int set_ring(serialport_ctx_t *sp);
extern int call_phone(serialport_ctx_t *sp,gsm_ctx_t *gsm);
extern int answer_phone(serialport_ctx_t *sp);
extern int hangup_phone(serialport_ctx_t *sp);

extern int delete_a_sms(serialport_ctx_t *sp,int index);
extern int read_sms(serialport_ctx_t *sp,char *phone_num,int index);

extern int query_sms(serialport_ctx_t *sp,char *pc_sms);
extern int delete_all_sms(serialport_ctx_t *sp);

#endif

