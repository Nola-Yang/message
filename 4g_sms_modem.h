#ifndef _SMS_MODEM_H_
#define _SMS_MODEM_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <ctype.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <netdb.h>
#include <dirent.h>
#include <linux/module.h>
#include <openssl/ssl.h>

#include "iniparser.h"
#include "mosquitto.h"
#include "4g_platform.h"
#include "4g_serialport.h"
#include "4g_at.h"
#include "logger.h"
#include "cJSON.h"
#include "database.h"
#include "4g_query_apn.h"
#include "4g_ppp_dialing.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
//#define SERIALPORT_SET gprs.sp.speed=115200;gprs.sp.flow_ctrl=0;gprs.sp.databits=8;gprs.sp.stopbits=1;gprs.sp.checkbits=0;
#define PPP_FILE "/tmp/.ppp.log"
//#define PING_FILE "/tmp/.ping.log"
#define   APN_FILE  "/home/nbiot/pengjiawei/sms-modem/apns-full-conf.xml"

typedef struct socket_ctx_s
{
        int             fd;
        int             port;
        char            ip[64];
        int             client_fd;
        int             command_read;
        int             query_module_unread_sms;
        char            return_msg_buf[64];
        int             have_return_msg;
}socket_ctx_t;

typedef struct ppp_ctx_s
{
        char            serial_name[32];
        char            *apn;
}ppp_ctx_t;

typedef struct sms_ctx_s
{
        socket_ctx_t            sock;
        serialport_ctx_t        sp;
        gsm_ctx_t               gsm;
        ppp_ctx_t               ppp;
        msq_ctx_t               msq;
        pthread_mutex_t         lock;
        sqlite3                 *db;
}sms_ctx_t;


void pub_publish_callback(struct mosquitto *mosq, void *obj, int rc);
void pub_connect_callback(struct mosquitto *mosq, void *obj, int rc);
void pub_disconnect_callback(struct mosquitto *mosq,void *obj, int rc);
//void message_callback(struct mosquitto *mosq,void *obj, const struct mosquitto_message *message);
//void sub_connect_callback(struct mosquitto *mosq, void *obj, int rc);
void sub_disconnect_callback(struct mosquitto *mosq, void *obj, int rc);
//void sub_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

extern int  init_sms_modem_argc(int argc,char **argv,sms_ctx_t *sms);
//extern int  init_sms_modem_argc(int argc,char **argv,sms_ctx_t *sms,msq_ctx_t *msq);
extern void print_usage(char *progname);
extern void signal_func(int signum);
extern int  auto_detect_gsm_serialport(sms_ctx_t *sms);
extern int  auto_set_apn(sms_ctx_t *sms);
extern int  socket_server_init(socket_ctx_t *sock);
extern int  cjson(gsm_ctx_t *gsm,char *buf);
extern int  thread_init(pthread_attr_t *thread_attr);
extern void *thread_socket(void *ctx);
extern void *thread_sms(void *ctx);
extern void *thread_ppp(void *ctx);
//extern int    start_dialing(char *ppp_serial_name,char *script_name);
//extern int  ping_test(char *network_card_name,char *url_name);
//extern int  stop_dialing(void);
extern void *thread_mqtt(void *ctx);
extern void pub_disconnect_callback(struct mosquitto *mosq,void *obj, int rc);
extern void message_callback(struct mosquitto *mosq,void *obj, const struct mosquitto_message *message);



#endif
