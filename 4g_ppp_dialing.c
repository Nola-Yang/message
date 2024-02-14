/*********************************************************************************
 *      Copyright:  (C) 2022 Nbiot<lingyun@gail.com>
 *                  All rights reserved.
 *
 *       Filename:  ppp_dialing.c
 *    Description:  This file ppp_dialing.c
 *                 
 *        Version:  1.0.0(29/05/22)
 *         Author:  Nbiot <lingyun@gail.com>
 *      ChangeLog:  1, Release initial version on "29/05/22 18:02:02"
 *                 
 ********************************************************************************/

#include "4g_ppp_dialing.h"


/*
int start_dialing(char *ppp_serial_name,char *script_name)
{
	int                     rc = 0;                 // 用于接收命令返回值
	int                     rv=-1;
	FILE                    *fp;
	char                    command[128];

	snprintf(command, sizeof(command), "sudo pppd %s call %s",ppp_serial_name,script_name);
	log_info("start_dialing command=%s\n",command);

	fp = popen(command, "r");
	if( NULL == fp )
	{
		log_error("popen start_dialing failure.\n");
		return -1;
	}
	log_info("popen start_dialing successfully!\n");

	rc = pclose(fp);
	if(-1 == rc)
	{
		log_error("close file point failure.\n");
		return -2;
	}

	return 0;
}
*/


int start_dialing(char *ppp_serial_name,char *script_name,char *PPP_FILE)
{
	pid_t 		pid1;
	int 		ppp_file_fd;

	if( (ppp_file_fd=open(PPP_FILE, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0 )
	{
		log_error("Redirect PPPD program standard output to file failure: %s\n", strerror(errno));
		return -1;
	}

	pid1 = fork();
	if( pid1 < 0 )
	{
		log_error("fork() create child process failure: %s\n", strerror(errno));
		return -2;
	}
	else if( pid1 == 0 ) //子进程开始运行
	{
		log_info("Child process start execute pppd program\n");

		dup2(ppp_file_fd, STDOUT_FILENO);

		//if( execl("/bin/sudo", "sudo", "pppd", "call", "rasppp", NULL) == -1 )
		if( execl("/bin/sudo", "sudo", "pppd", ppp_serial_name, "call", script_name, NULL) == -1 )
		{
			log_error(" execl 'sudo pppd %s call rasppp' error. \n",ppp_serial_name);
			return -3;
		}
	}

	return 0;
}


int ping_test(char *network_card_name,char *url_name)
{
	int                     rc = 0;                 // 用于接收命令返回值
	int                     rv=-1;
	FILE                    *fp;
	char                    command[128];
	char                    result_buf[1024];

	/*将要执行的命令写入buf*/
	snprintf(command, sizeof(command), "ping -I %s -c 2 %s",network_card_name,url_name);
	//log_info("ping_test command=%s\n",command);

	/*执行预先设定的命令，并读出该命令的标准输出*/
	fp = popen(command, "r");
	if( NULL == fp )
	{
		log_error("popen ping_test failure.\n");
		return -1;
	}
	log_info("popen ping_test successfully!\n");

	/*读ping的内容*/
	if( (rv=fread(result_buf,1,1024,fp))<0 )
	{
		log_error("Read from fp failure:%s\n",strerror(errno));
		return -2;
	}

	if(strstr(result_buf,"0\% packet loss")==NULL)
	{
		log_error("Network card 'ppp0' ping %s failure.\n",url_name);
		return -3;
	}
	log_info("Network card 'ppp0' %s successfully!\n",url_name);

	rc = pclose(fp);
	if(-1 == rc)
	{
		log_error("close file point failure.\n");
		return -4;
	}

	return 0;
}

int stop_dialing(void)
{
	int                     rc = 0;                 // 用于接收命令返回值
	int                     rv=-1;
	FILE                    *fp;
	char                    command[128];

	/*将要执行的命令写入buf*/
	snprintf(command, sizeof(command), "sudo killall pppd");
	//log_info("stop_dialing command=%s\n",command);

	/*执行预先设定的命令，并读出该命令的标准输出*/
	fp = popen(command, "r");
	if( NULL == fp )
	{
		log_error("popen stop_dialing failure.\n");
		return -1;
	}
	log_info("stop dialing successfully!\n");

	rc = pclose(fp);
	if(-1 == rc)
	{
		log_error("close file point failure.\n");
		return -2;
	}

	return 0;
}

int check_whether_dialing(char *NETWROK_CARD_FILE) 
{
	if( access(NETWROK_CARD_FILE, F_OK)!=0 )
	{
		log_info("Network card %s not exist,start dialing now!\n",NETWROK_CARD_FILE );
		return 0;
	}

	return -1;
}

