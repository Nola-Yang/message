/********************************************************************************
 *      Copyright:  (C) 2022 Nbiot<lingyun@gail.com>
 *                  All rights reserved.
 *
 *       Filename:  ppp_dialing.h
 *    Description:  This head file ppp_dialing.h
 *
 *        Version:  1.0.0(29/05/22)
 *         Author:  Nbiot <lingyun@gail.com>
 *      ChangeLog:  1, Release initial version on "29/05/22 18:05:46"
 *                 
 ********************************************************************************/

#ifndef _PPP_DIALING_H
#define _PPP_DIALING_H

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

#include "logger.h"


//extern int start_dialing(char *ppp_serial_name,char *script_name);
extern int start_dialing(char *ppp_serial_name,char *script_name,char *PPP_FILE);
extern int ping_test(char *network_card_name,char *url_name);
extern int stop_dialing(void);
extern int check_whether_dialing(char *NETWROK_CARD_FILE);


#endif
