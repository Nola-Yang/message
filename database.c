/*********************************************************************************
 *      Copyright:  (C) 2022 Nbiot<lingyun@gail.com>
 *                  All rights reserved.
 *
 *       Filename:  database.c
 *    Description:  This file database.c
 *
 *        Version:  1.0.0(01/06/22)
 *         Author:  Nbiot <lingyun@gail.com>
 *      ChangeLog:  1, Release initial version on "01/06/22 17:40:37"
 *
 ********************************************************************************/

#include "database.h"

char		sql[128];
char		*zErrMsg=NULL;
int     	nrow=0;
int     	ncolumn = 0;


int open_database(sqlite3 **db,char *database_name)
{
	int             len;

	len = sqlite3_open(database_name,db);
	if(len)
	{
		log_error("Open database name %s failure.\n",database_name);
		sqlite3_close(*db);
		return -1;
	}
	log_info("Open a sqlite3 database name %s successfully!\n",database_name);

	return 0;
}

int	create_table(sqlite3 **db,char *table_name,char *table_attribute)
{


	snprintf(sql,sizeof(sql),"CREATE TABLE %s(%s);",table_name,table_attribute);
	log_info("sql=%s\n",sql);

	//sql="CREATE TABLE test(TEST CHAR(100));";

	if(sqlite3_exec(*db,sql,NULL,NULL,&zErrMsg)!=SQLITE_OK)
	{
		log_info("Table %s already exist\n",table_name);
	}
	else
	{
		log_info("Create table %s successfully\n",table_name);
	}

}

int insert_data(sqlite3 **db,char *table_name,char *attr,char *msg)
{
	snprintf(sql,sizeof(sql),"INSERT INTO %s(%s) VALUES('%s');",table_name,attr,msg);   //插入数据

	if(sqlite3_exec(*db,sql,NULL,NULL,&zErrMsg)!=SQLITE_OK)
	{
		sqlite3_close(*db);
		log_error("Insert %s to table %s failure:%s\n",msg,table_name,strerror(errno));
		return -1;
	}
	log_info("Insert %s to table %s successfully\n",msg,table_name);

	return 0;
}


int query_data(sqlite3 **db,char ***azResult,char *table_name)
{
	snprintf(sql,sizeof(sql),"select *from %s;",table_name);
	//sql="select *from test";

	if(sqlite3_get_table(*db,sql,azResult,&nrow,&ncolumn,&zErrMsg)!=SQLITE_OK)
	{
		sqlite3_close(*db);
		log_error("Select *from %s failure\n",table_name);
		return -1;
	}
	log_info("There are %d pieces of SMS in table %s\n",nrow,table_name);

	return nrow;
}

int delete_data(sqlite3 **db,char *table_name)
{
	snprintf(sql,sizeof(sql),"delete from %s;",table_name);
	//sql="delete from test";

	if(sqlite3_exec(*db,sql,NULL,NULL,&zErrMsg)!=SQLITE_OK)
	{
		sqlite3_close(*db);
		log_error("Delete from %s failure\n",table_name);
		return -1;
	}
	log_info("Delete SMS from table %s successfully!\n",table_name);

	return 0;
}


