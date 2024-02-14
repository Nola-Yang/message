#ifndef _QUERY_APN_H_
#define _QUERY_APN_H_

#include <stdio.h>
#include <string.h>
#include <libxml2/libxml/xmlmemory.h>
#include <libxml2/libxml/parser.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlstring.h>
#include "4g_serialport.h"
#include "4g_at.h"
#include "logger.h"
//#define   FILE_NAME  "/home/nbiot/pengjiawei/sms_-modem/apns-full-conf.xml"

extern int query_apn(char *apn_buf,char *file_name,char *mcc_buf,char *mnc_buf);


#endif

