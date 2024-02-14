#ifndef _SERIALPORT_H_
#define _SERIALPORT_H_

typedef struct serialport_ctx_s
{
	int			fd;		//文件描述符
	int 		speed;		//波特率
	int 		flow_ctrl;	//流控制
	int			databits;	//数据位
	int 		stopbits;	//停止位
	int 		checkbits;	//校检位
	char		serial_name[16];//串口名
	int			serial_detect_flag;	//自动检测串口flag

	//struct termios  options;	//串口原始属性
    //pthread_mutex_t         lock;
}serialport_ctx_t;

//extern int serialport_open(serialport_ctx_t *sp);
extern int serialport_close(serialport_ctx_t *sp);
extern int serialport_init(serialport_ctx_t *sp);
extern int serialport_receive(serialport_ctx_t *sp,char *rbuf,int rbuf_len,int timeout);
extern int serialport_send(serialport_ctx_t *sp,char *send,int data_len);

#endif

