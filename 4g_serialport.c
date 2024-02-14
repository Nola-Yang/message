#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>

#include "4g_serialport.h"
#include "logger.h"

/*
   int serialport_open(serialport_ctx_t *sp)
   {
   sp->fd=open(sp->serial_name,O_RDWR|O_NOCTTY|O_NDELAY);
   if(sp->fd<0)
   {
   log_error("can't open serial port\n");
   return -1;
   }
   log_info("open serial port '%s' successfully and fd=%d\n",sp->serial_name,sp->fd);

   return 0;
   }
   */

int serialport_close(serialport_ctx_t *sp)
{
	close(sp->fd);

	return 0;
}

int serialport_init(serialport_ctx_t *sp)
{
	struct termios  options;
	int             i;
	int				rv=-1;
	int   			speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};
	int   			name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};
	
	sp->speed=115200;
	sp->flow_ctrl=0;
	sp->databits=8;
	sp->stopbits=1;
	sp->checkbits=0;

	sp->fd=open(sp->serial_name,O_RDWR|O_NOCTTY);		//open serialport
	if(sp->fd<0)
	{
		log_error("can't open serial port\n");
		return -1;
	}

	/* 设置串口为阻塞态 */
	if((rv = fcntl(sp->fd,F_SETFL,0)) < 0)
	{
		printf("Serial port Fcntl check faile.\n");
		return -1;
	}
	
	log_info("open serial port '%s' successfully and fd=%d\n",sp->serial_name,sp->fd);
	
	if(tcgetattr(sp->fd,&options)!=0)
	{
		log_error("tcgetattr() failure:%s\n",strerror(errno));
		return -1;
	}
	//log_info("tcgetattr() successfully!\n");

	//设置输入输出波特率
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
	{
		if  (sp->speed == name_arr[i])
		{
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);
		}
	}

	//不会占用串口，能读取输入数据
	options.c_cflag |= CLOCAL|CREAD;

	switch(sp->flow_ctrl)
	{
		case 0:         //不使用流控制
			options.c_cflag &=~ CRTSCTS;
			break;

		case 1:         //使用硬件流控制
			options.c_cflag |= CRTSCTS;
			break;

		case 2:         //使用软件流控制
			options.c_cflag |= IXON | IXOFF | IXANY;
			break;
	}

	//设置数据位
	options.c_cflag &=~ CSIZE;
	switch(sp->databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;
	}

	//设置停止位
	switch(sp->stopbits)
	{
		case 1:
			options.c_cflag &=~ CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;
	}

	//设置校检位
	switch(sp->checkbits)
	{
		case 0: //无校验位
			options.c_cflag &=~ PARENB;
			options.c_iflag &=~ INPCK;
			break;

		case 1: //奇
			options.c_cflag |= PARENB;
			options.c_cflag |= PARODD;
			options.c_iflag |= (INPCK|ISTRIP);//inpck打开奇偶校验,istrip去掉字符第八位
			break;

		case 2: //偶
			options.c_cflag |= PARENB;
			options.c_cflag |= ~PARODD;
			options.c_cflag |= (INPCK|ISTRIP);
			break;
	}

	//设置等待时间和最小接收字符
	options.c_cc[VTIME]=0;
	options.c_cc[VMIN]=0;

	tcflush(sp->fd,TCIFLUSH);   //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读

	if(tcsetattr(sp->fd,TCSANOW,&options)!=0)
	{
		log_error("tcsetattr() failure:%s\n",strerror(errno));
		return -1;
	}
	//log_info("tcsetattr() successfully!\n");

	return 0;
}

int serialport_receive(serialport_ctx_t *sp,char *rbuf,int rbuf_len,int timeout)
{
	int             rv=-1;
	int             len=-1;
	fd_set          rd_set;
	struct timeval 	time_out;

	if(!rbuf||rbuf_len<=0)
	{
		log_error("receive func get invalid parameter\n");
		return -1;
	}

	if(timeout)
	{
		time_out.tv_sec=(time_t)timeout;
		time_out.tv_usec=0;

		FD_ZERO(&rd_set);
		FD_SET(sp->fd,&rd_set);

		rv=select(sp->fd+1,&rd_set,NULL,NULL,&time_out);
		//rv=select(sp->fd+1,&rd_set,NULL,NULL,NULL);
		if(rv<0)
		{
			log_error("select failure:%s\n",strerror(errno));
			return -1;
		}
		else if(rv==0)
		{
			log_error("select timeout\n");
			return -1;
		}

	}

	sleep(timeout);

	//if(FD_ISSET(sp->fd,&rd_set))
	//{
	memset(rbuf,0,sizeof(rbuf));
	if((len=read(sp->fd,rbuf,rbuf_len))<0)
	{
		log_error("read from serial port failure:%s\n",strerror(errno));
		return -1;
	}
	//log_info("read %d bytes data from serial port fd=%d:'%s'\n",len,sp->fd,rbuf);
	//}

	return 0;
}

int serialport_send(serialport_ctx_t *sp,char *send,int data_len)
{
	int len=-1;

	len=write(sp->fd,send,data_len);
	if(len<0)
	{
		log_error("write failure:%s\n",strerror(errno));
		return -1;
	}
	//log_info("write '%d' btyes '%s' to serial port successfully!\n",data_len,send);

	return 0;
}

