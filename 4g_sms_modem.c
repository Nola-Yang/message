#include "4g_sms_modem.h"


int g_sigkill=0;
int g_stop_dial=0;
int g_start_dial=1;
//int  sig_stop;
bool session=true;
int	mqtt_sub_receive=0;
char sub_receive_buf[1024];


int main(int argc,char **argv)
{
	struct hostent          *get_host;
	int						rv=-1;
	void                    *args;
	//msq_ctx_t               msq;
	sms_ctx_t 	     		sms;
	pthread_t             	tid;
	pthread_attr_t        	thread_attr;
	pthread_mutex_init(&sms.lock, NULL);

	sms.sp.serial_detect_flag = 0;

	if( init_sms_modem_argc(argc, argv, &sms ) <0 )
	{
		printf("Init mosquitto arguments failure:%s\n", strerror(errno));
		return -1;
	}


        /*解析配置文件*/
        //get_config(&msq);
        get_config(&sms.msq);
#if 0
        printf("platform=%s\n",msq.p_ctx.platform);
        printf("clientid=%s\n",msq.p_ctx.clientid);
        printf("hostname=%s\n",msq.p_ctx.hostname);
        printf("username=%s\n",msq.p_ctx.username);
        printf("password=%s\n",msq.p_ctx.password);
        printf("id=%s\n",msq.p_ctx.id);
        printf("sub_topic=%s\n",msq.sub_ctx.sub_topic);
        printf("pub_topic=%s\n",msq.pub_ctx.pub_topic);
        printf("port=%d\n",msq.p_ctx.port);
        printf("keepalive=%d\n",msq.p_ctx.keepalive);
        printf("cafile=%s\n",msq.p_ctx.cafile);
        printf("capath=%s\n",msq.p_ctx.capath);
        printf("certfile=%s\n",msq.p_ctx.certfile);
        printf("keyfile=%s\n",msq.p_ctx.keyfile);
#endif


	if( logger_init("stdout", LOG_LEVEL_INFO) < 0)
	{
		printf("initial logger system failure\n");
		return 1;
	}

	signal(SIGTERM,signal_func);                   
	signal(SIGUSR1,signal_func);                   
	signal(SIGUSR2,signal_func);                   


	if( thread_init(&thread_attr) )
	{
		log_error("thread init failure: %s\n", strerror(errno));
		goto cleanup;
	}

	//thread_init(&tid,thread_mqtt_work,&msq);
#if 1
	if( pthread_create(&tid, &thread_attr, thread_mqtt, &sms) )  
	{
		log_error("create thread_mqtt failure: %s\n", strerror(errno));
		goto cleanup;
	}
	log_info("create thread_mqtt successfully.\n");
#endif

#if 0 
	if( pthread_create(&tid, &thread_attr, thread_socket, &sms) )  
	{
		log_error("create thread_socket failure: %s\n", strerror(errno));
		goto cleanup;
	}
	log_info("create thread_socket successfully.\n");
#endif

#if 1	
	if( pthread_create(&tid, &thread_attr, thread_sms, &sms) )  
	{
		log_error("create thread_gsm failure: %s\n", strerror(errno));
		goto cleanup;
	}
	log_info("create thread_gsm successfully.\n");
#endif

#if 0
	if( pthread_create(&tid, &thread_attr, thread_ppp, &sms) )
	{
		log_error("create thread_ppp failure: %s\n", strerror(errno));
		goto cleanup;
	}
	log_info("create thread_ppp successfully.\n");	
#endif


	while(!g_sigkill)
	{
		if( !sms.sp.serial_detect_flag )
		{
			if( auto_detect_gsm_serialport(&sms)<0 )		//自动检测EC20F串口
			{
				sms.gsm.status = GSM_SERIALPORT_TYPE_NOTEXIST;
				log_error("Find EC20F serialport failure.\n");
				sleep(1);
				continue;
			}

			sms.gsm.set_apn_flag = 0;
			sms.sp.serial_detect_flag = 1;
		}

		sms.gsm.status = check_gsm_all_ready(&(sms.sp));
		if( sms.gsm.status == GSM_MODULE_NOTREADY )
		{
			sleep(1);
			sms.sp.serial_detect_flag = 0;
			continue;
		}
		else if( sms.gsm.status != GSM_ALL_READY )
		{
			sleep(1);
			continue;
		}

		if( !sms.gsm.set_apn_flag )
		{
			if( auto_set_apn(&sms)<0 )
				goto cleanup;

			sms.gsm.set_apn_flag = 1;

			/* query SMS center */
		}

		sleep(1000);

	}

cleanup:
	close(sms.sock.fd);
	serialport_close(&(sms.sp));
	logger_term();
	pthread_attr_destroy(&thread_attr);

	return 0;
}

int init_sms_modem_argc(int argc,char **argv,sms_ctx_t *sms )
{
	int			ch;
	char		*progname;
	char        *str1="ali";

	struct option opts[]={
		{"aliyun",no_argument,NULL,'A'},	// 指定配置文件	
		{"help",	no_argument,	  NULL,'h'},
		{NULL,0,NULL,0}
	};

	progname=basename(argv[0]);

	while((ch=getopt_long(argc,argv,"Ah",opts,NULL))!=-1)
	{
		switch(ch)
		{
			case 'A':
				strncpy(sms->msq.p_ctx.platform,str1,sizeof(sms->msq.p_ctx.platform));
				printf("platform=%s\n",sms->msq.p_ctx.platform);

				break;
		
			case 'h':
				print_usage(progname);
				return 0;
		}
	}

	return 0;
}

void print_usage(char *progname)
{
	printf("Usage:%s [OPTION]...\n",progname);
	printf("-p(--p):sepcify port\n");
	printf("-s(--s):serial name\n");
	printf("-h(--h):print help information\n");
	printf("example: ./%s -p 7001 -s /dev/ttyUSB3\n",progname);
}

void signal_func(int signum)
{
	if(signum==SIGTERM)
	{
		log_warn("SIGTERM signal detected and process exit...\n");
		g_sigkill=1;
	}
	else if(signum==SIGUSR1)
	{
		log_warn("SIGUSR1 signal detected and stop dialing...\n");
		g_stop_dial=1;
	}
	else if(signum==SIGUSR2)
	{
		log_warn("SIGUSR2 signal detected and restart dialing...\n");
		g_start_dial=1;
	}
}

int auto_detect_gsm_serialport(sms_ctx_t *sms)
{
	int		i;
	//sms->ppp.serial_name = NULL;

	for(i=5;i<7;i++)
	{
		snprintf(sms->sp.serial_name,sizeof(sms->sp.serial_name),"/dev/ttyUSB%d",i);

		if( serialport_init(&(sms->sp) )<0)            	//open and initialize serialport	
		{
			serialport_close(&(sms->sp));
			continue;
		}

		if( check_module_type(&(sms->sp),"EC20F")!=0 )
		{
			log_info("Serialport ttyUSB%d is not EC20F.\n",i);
			//serialport_close(&(sms->sp));
			continue;
		}
		log_info("Serialport ttyUSB%d is EC20F.\n",i);

		sprintf(sms->ppp.serial_name,"ttyUSB%d",i+1);
		log_info("PPP dialing serial port name is '%s'.\n",sms->ppp.serial_name);

		break;
	}

	return 0;
}

int auto_set_apn(sms_ctx_t *sms)
{
	char		apn_buf[32];
	char		mcc_buf[16];
	char		mnc_buf[16];

	if( query_module_mcc_and_mnc(&(sms->sp),mcc_buf,mnc_buf)<0 )
		return -1;

	if( query_apn(apn_buf,APN_FILE,mcc_buf,mnc_buf)<0 )
	{
		log_error("Query apn from %s failure.\n",APN_FILE);
		return -2;
	}
	log_info("Query apn from %s successfully!\n",APN_FILE);

	if( set_module_apn(&(sms->sp),apn_buf)<0 )
		return -3;

	return 0;
}


int cjson(gsm_ctx_t *gsm,char *buf)
{
	cJSON *json = NULL;
	cJSON *json_val1 = NULL;
	cJSON *json_val2 = NULL;
	cJSON *json_val3 = NULL;

	json = cJSON_Parse(buf);
	if(NULL == json)
	{
		printf("cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
	}

	if( (json_val1 = cJSON_GetObjectItem(json, "type"))==NULL )
	{
		log_error("cJSON get 'type' failure.\n");
		return -1;
	}	
	if( (json_val2 = cJSON_GetObjectItem(json, "number"))==NULL )
	{
		log_error("cJSON get 'number' failure.\n");
		return -2;
	}
	if( (json_val3 = cJSON_GetObjectItem(json, "sms"))==NULL )
	{
		log_error("cJSON get 'sms' failure.\n");
		return -3;
	}

	gsm->type=json_val1->valueint;
	strcpy(gsm->phone_num,json_val2->valuestring);
	strcpy(gsm->sms,json_val3->valuestring);

	cJSON_Delete(json);

	return 0;
}

int thread_init(pthread_attr_t *thread_attr)
{

	if( pthread_attr_init(thread_attr) )
	{
		log_error("pthread_attr_init() failure: %s\n", strerror(errno));
		return -1;
	}

	if( pthread_attr_setstacksize(thread_attr, 120*1024) )
	{
		log_error("pthread_attr_setstacksize() failure: %s\n", strerror(errno));
		return -1;
	}

	if( pthread_attr_setdetachstate(thread_attr, PTHREAD_CREATE_DETACHED) )
	{
		log_error("pthread_attr_setdetachstate() failure: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


void *thread_mqtt(void *ctx)
{

        //msq_ctx_t               *msq=(msq_ctx_t *)ctx;
        sms_ctx_t               *sms=(sms_ctx_t *)ctx;
        int                     rv=-1;
        //sqlite3                 *db;
        struct  mosquitto       *mosq=NULL;
        //char                  *get_ip;
        struct hostent          *get_host;
        char                    buf[1024];
        char                    send_json[1024];
        char                    unsend_json[1024];
        float                   temperature;
        char                    T_time[64];
        int                     log_fd;
        time_t                  current_time=0;
        time_t                  last_time=0;
        int                     need_seconds=5;
        int                     check_flag;
        int                     con_flag=-1;
        int                     connect_flag=0;
        

		char                    **azResult=NULL;
        char                    **azResult2=NULL;
        char                    gsm_buf[1024];
        int                     unsend_sms_num=0;
        int            			gsm_return_client_msg_num=0;
        char                    *str2="{\"msg\":\"这是一条4g模块发来的短信\"}";

#if 0

		if( open_database(&db,"sms_data.db")<0 ) 		 //open database and create table
			goto cleanup;
		
		if( create_table(&(sms->db),"client_sms","CLIENT_SMS CHAR(100)")<0 )
			goto cleanup;

		if( create_table(&(sms->db),"module_sms","MODULE_SMS CHAR(100)")<0 )
			goto cleanup;
			
#endif

		//strcpy(sms->msq.p_ctx.clientid,"mqttsms");
		log_info("clientid=%s\n",sms->msq.p_ctx.clientid);
		strncpy(sms->msq.p_ctx.get_ip,"101.34.10.119",sizeof(sms->msq.p_ctx.get_ip));

		rv=mosquitto_lib_init();
        if(rv!=MOSQ_ERR_SUCCESS)
		{
                log_error("mosquitto init failure:%s\n",strerror(errno));
               // return -2;
        }
        else
		{
                log_info("mosquitto init successful\n");
        }

        mosq=mosquitto_new(sms->msq.p_ctx.clientid,true,NULL);
        if(!mosq)
		{
                log_error("new pub_test failure:%s\n",strerror(errno));
               // return -3;
        }
        else{
                log_info("create mosquitto successful\n");
        }

        sms->msq.p_ctx.mosq=mosq;
        sms->msq.pub_ctx.connect_flag=connect_flag;

        //mosquitto_disconnect_callback_set(msq.p_ctx.mosq,disconnect_callback);
        mosquitto_message_callback_set(sms->msq.p_ctx.mosq,message_callback);
        //mosquitto_subscribe_callback_set(msq->p_ctx.mosq,sub_connect_callback);
        // mosquitto_connect_callback_set(msq->p_ctx.mosq,pub_connect_callback);
        mosquitto_disconnect_callback_set(sms->msq.p_ctx.mosq,pub_disconnect_callback);

        rv=mosquitto_username_pw_set(sms->msq.p_ctx.mosq, sms->msq.p_ctx.username, sms->msq.p_ctx.password);//在connect之前调用这个函数
        if(rv!=MOSQ_ERR_SUCCESS)
		{
                log_error("set username and password failure:%s\n",strerror(errno));
               // return -4;
        }


#if 1
		log_info("ip=%s\n",sms->msq.p_ctx.get_ip);
		log_info("port=%d\n",sms->msq.p_ctx.port);
		log_info("pub_topic=%s\n",sms->msq.pub_ctx.pub_topic);
		log_info("sub_topic=%s\n",sms->msq.sub_ctx.sub_topic);
#endif
		con_flag = mosquitto_connect(sms->msq.p_ctx.mosq, sms->msq.p_ctx.get_ip, sms->msq.p_ctx.port, sms->msq.p_ctx.keepalive);
		if(con_flag!=MOSQ_ERR_SUCCESS)
		{
			log_error("mosquitto_connect failure:%s\n",strerror(errno));
		}
		else
		{
                log_info("mosquitto_connect successfully\n");
                rv=mosquitto_subscribe(sms->msq.p_ctx.mosq,NULL,"17687499243",sms->msq.sub_ctx.sub_qos);
                if(rv!=MOSQ_ERR_SUCCESS)
				{
                        log_error("mosquitto sub failure:%s\n",strerror(errno));
                }
                else
				{
                        log_info("mosquitto sub success\n");
                }

                rv = mosquitto_loop_start(sms->msq.p_ctx.mosq);
                if(rv!=MOSQ_ERR_SUCCESS){
                        log_error("mosquitto_loop failure:%s\n",strerror(errno));
                        //return -1;
                }
                log_info("loop ok!\n");
				//sleep(10);
        }
		//sms->sock.query_module_unread_sms=1;
			
		

		while(!g_sigkill)
		{
			if(sms->gsm.have_data_in_gsm_database)
			{
				if( (gsm_return_client_msg_num=query_data(&(sms->db),&azResult2,"module_sms"))>0 )
				{
					log_info("There are %d pieces of GSM_RETURN_CLIENT_MSG in table gsm.\n",gsm_return_client_msg_num);
				
					strcpy(gsm_buf,azResult2[gsm_return_client_msg_num]);
					//log_info("gsm_buf=%s\n",gsm_buf);


					rv=mosquitto_publish(sms->msq.p_ctx.mosq,NULL,"17386070887",strlen(str2), gsm_buf , sms->msq.pub_ctx.pub_qos,true);
					//rv=mosquitto_publish(sms->msq.p_ctx.mosq,NULL,"17386070887",strlen(str2),str2,sms->msq.pub_ctx.pub_qos,true);
					//rv=mosquitto_publish(msq->p_ctx.mosq,NULL,msq->pub_ctx.pub_topic,strlen(send_json)+1,send_json,msq->pub_ctx.pub_qos,0);
					if(rv!=MOSQ_ERR_SUCCESS)
					{
						log_error("mosquitto_pub failure:%s\n",strerror(errno));
					}
					else
					{
						log_info("pub '%s' ok!\n",gsm_buf);
					}


					if( delete_data( &(sms->db),"module_sms" )<0 )              //
						goto cleanup;

					sms->gsm.have_data_in_gsm_database=0;
				}

				//break;
			}

			if( mqtt_sub_receive )
			{

				log_info("sms->gsm.json_buf=%s\n",sms->gsm.json_buf);
				
				log_info("sms->msq.p_ctx.recv_message:%s\n",sms->msq.p_ctx.recv_message);
				
				log_info("sub_receive_buf=%s\n", sub_receive_buf );

				if( cjson(&(sms->gsm), sub_receive_buf )<0 )
				{
					log_info("cjson sub_recv_message failure.%s\n",strerror(errno));
					goto cleanup;
				}
				else
				{
					sms->sock.command_read=1;
				}

				mqtt_sub_receive = 0;
			}


		}


		//sqlite3_close(db);
		mosquitto_lib_cleanup();
		mosquitto_destroy(mosq);

cleanup:
	g_sigkill=1;
	pthread_exit(NULL);
}


void pub_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
    msq_ctx_t           *msq = (msq_ctx_t *)obj;
	log_info("Call the function :pub_connect_callback\n");

    if(rc)
	{
		log_error("on_connect error!\n");
        exit(1);

    }
}

void pub_disconnect_callback(struct mosquitto *mosq,void *obj, int rc)
{
	msq_ctx_t       *msq=(msq_ctx_t *)obj;
    if(rc == 0)
    {
        log_warn("The client called the mosquitto_disconnect function!\n");
        msq->pub_ctx.connect_flag=1;
        return;
	}

}

void pub_publish_callback(struct mosquitto *mosq, void *obj, int rc)
{   
	log_info("Call the function: pub_publish_callback\n");
}

#if 0
void sub_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	msq_ctx_t           *msq = (msq_ctx_t *)obj;


	printf("Call the function: sub_connect_callback\n");

	if(rc){
		printf("on_connect error!\n");
		exit(1);
	}

	//mosquitto_subscribe(mosq,NULL,"17687499243",msq->sub_ctx.sub_qos);
	printf("sub ok\n");
	//log_nrml("mosquitto_subscribe connect with Topic [\"%s\"] \n", ctx->mqtt_ctx.subTopic_ack);

}
#endif

void sub_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	log_info("Call the function: sub_disconnect_callback\n");
}

#if 0
void sub_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{

}
#endif

void message_callback(struct mosquitto *mosq,void *obj, const struct mosquitto_message *message)
{
	msq_ctx_t       *msq=(msq_ctx_t *)obj;
	//  mosquitto_t     *mosquitto;
	//  mosquitto = (mosquitto_t *)obj;
	memset(msq->p_ctx.recv_message,0,sizeof(msq->p_ctx.recv_message));
	memcpy(msq->p_ctx.recv_message,message->payload,strlen(message->payload));
	
	mosquitto_loop_stop(msq->p_ctx.mosq,true);
	log_info("recv_message: %s\n", msq->p_ctx.recv_message);

	strcpy(sub_receive_buf,msq->p_ctx.recv_message);
	log_info("sub_receive_buf=%s\n",sub_receive_buf);

	if (0 == strcmp(message->payload, "quit"))
		mosquitto_disconnect(msq->p_ctx.mosq);

	mqtt_sub_receive = 1;

}


void *thread_sms(void *ctx)
{	
	char		unread_sms[128];

	sms_ctx_t *sms = (sms_ctx_t *)ctx;
	
	sms->sock.query_module_unread_sms =1;

#if 1
	if( open_database(&(sms->db),"sms.db")<0 ) 		 //open database and create table
		goto cleanup;

	if( create_table(&(sms->db),"client_sms","CLIENT_SMS CHAR(100)")<0 )
		goto cleanup;

	if( create_table(&(sms->db),"module_sms","MODULE_SMS CHAR(100)")<0 )
		goto cleanup;
#endif 



	while(!g_sigkill)
	{
		if( sms->gsm.status == GSM_ALL_READY )
		{

			if(sms->sock.query_module_unread_sms)
			{
				pthread_mutex_lock(&sms->lock);

				if( query_sms(&(sms->sp),sms->gsm.pc_sms)==0 )	//have unsend_sms in module
				{
					//log_info("pc_sms=%s\n",sms->gsm.pc_sms);
					snprintf(unread_sms,sizeof(unread_sms),"{\"unread_sms\":\"%s\"}",sms->gsm.pc_sms);
					if( insert_data(&(sms->db),"module_sms","MODULE_SMS",unread_sms)<0 )		
						goto cleanup;

					
					   //if( delete_all_sms(&(sms->sp))<0 )
					   //goto cleanup;
					
				}

				sms->gsm.have_data_in_gsm_database=1;	
				sms->sock.query_module_unread_sms=0;
				pthread_mutex_unlock(&sms->lock);
				//break;
			}

			if(sms->sock.command_read)
			{
				pthread_mutex_lock(&sms->lock);

				log_info("i in command_read part.\n");

				if(sms->gsm.type==SMS_TYPE_TEXT)
				{
					if(send_sms_text(&(sms->sp),sms->gsm.phone_num,sms->gsm.sms)==0)
					{
						strcpy(sms->sock.return_msg_buf,"{\"message\":\"send SMS(text) to client successfully!\"}");		
					}
					else
					{
						strcpy(sms->sock.return_msg_buf,"{\"message\":\"send SMS(text) to client failure.\"}");		
					}

				}
				else if(sms->gsm.type==SMS_TYPE_PDU)
				{
					if(send_sms_pdu(&(sms->sp),sms->gsm.center_num,sms->gsm.phone_num,sms->gsm.sms)==0)
					{
						strcpy(sms->sock.return_msg_buf,"{\"message\":\"send SMS(pdu) to client successfully!\"}");		
					}	
					else
					{
						strcpy(sms->sock.return_msg_buf,"{\"message\":\"send SMS(pdu) to client failure.\"}");		
					}

				}
				
				sms->sock.have_return_msg=1;	
				sms->sock.command_read=0;
				pthread_mutex_unlock(&sms->lock);
				//break;
			}
		}
		else
		{
			sleep(1);
			continue;
		}
	}


cleanup:
	g_sigkill=1;
	pthread_exit(NULL);
}

#if 0
void *thread_ppp(void *ctx) 
{
	sms_ctx_t *sms = (sms_ctx_t *)ctx;

	while(!g_sigkill)
	{
	
		if( sms->gsm.status == GSM_ALL_READY )
		{
			/*
			if(g_start_dial)
			{
			*/
			if( sms->gsm.set_apn_flag )
			{
				if( check_whether_dialing("/sys/class/net/ppp0")==0 )
				{

					if( start_dialing(sms->ppp.serial_name,"rasppp",PPP_FILE)<0 )	//fork() and execl
						goto cleanup;

					sleep(3);

					if( ping_test("ppp0","www.baidu.com")<0 )		//popen
						goto cleanup;
				}
				
				//g_start_dial=0;

			}

			/*
			if(g_stop_dial)
			{
				if( stop_dialing()<0 )		//popen
					goto cleanup;	

				g_stop_dial=0;
			}
			*/

		}//status==GSM_ALL_READY 
		else
		{
			sleep(1);
			continue;
		}

	}//while

cleanup:
		g_sigkill=1;	
		pthread_exit(NULL);
}
#endif
