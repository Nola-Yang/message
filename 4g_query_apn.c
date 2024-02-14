#include <stdio.h>
#include <string.h>
#include <libxml2/libxml/xmlmemory.h>
#include <libxml2/libxml/parser.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlstring.h>
#include "4g_query_apn.h"

//#define   FILE_NAME  "/home/nbiot/pengjiawei/sms_-modem/apns-full-conf.xml"

int query_apn(char *apn_buf,char *file_name,char *mcc_buf,char *mnc_buf)
{
	xmlDocPtr doc;        /* xml document tree */
	xmlNodePtr cur;       /* xml node */
	xmlChar *mcc;         /* Operator MCC */
	xmlChar *mnc;         /* Operator MNC */
	xmlChar *apn;         /* pppd APN */
	//char    *qmcc = "460";
	//char    *qmnc = "03";
	char	*qmcc = mcc_buf;
	char	*qmnc = mnc_buf;
	char	*ppp_apn;

	if( !file_name )
	{
		printf("Invalid input arguments\n");
	}

	doc = xmlReadFile(file_name, "UTF-8", XML_PARSE_RECOVER);  //先读入需要解析的xml文件
	if (doc == NULL)
	{
		log_error("Failed to parse xml file:%s\n", file_name);
		goto cleanup;
	}

	cur = xmlDocGetRootElement(doc);  //获取根节点
	if (cur == NULL)
	{
		log_error("Root is empty.\n");
		goto cleanup;
	}

	if ( xmlStrcmp(cur->name, (const xmlChar *)"apns") )
	{
		log_error("The root is not apns.\n");
		goto cleanup;
	}

	cur = cur->xmlChildrenNode;
	
	while (cur != NULL)
	{
		if ( xmlStrcmp(cur->name, (const xmlChar *)"apn")==0 )
		{
			mcc = xmlGetProp(cur, "mcc");
			mnc = xmlGetProp(cur, "mnc");

			if( xmlStrcmp(mcc, (const xmlChar *)qmcc)==0 && xmlStrcmp(mnc, (const xmlChar *)qmnc)==0 )
			{
				apn = xmlGetProp(cur, "apn");
				//log_info("mcc:%s mnc:%s apn:%s \n",mcc, mnc, apn);
				
				ppp_apn = (char *)apn;
				//log_info("ppp_apn=%s\n",ppp_apn);
				strcpy(apn_buf,ppp_apn);				
				//log_info("apn_buf=%s\n",apn_buf);
				
				break;
			}
		}
		cur = cur->next;
	}

	if (doc)
	{
		xmlFreeDoc(doc);
	}

	return 0;

cleanup:
	if (doc)
	{
		xmlFreeDoc(doc);
	}

	return -1;
}

 
