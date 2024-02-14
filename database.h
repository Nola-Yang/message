/********************************************************************************
 *      Copyright:  (C) 2022 Nbiot<lingyun@gail.com>
 *                  All rights reserved.
 *
 *       Filename:  database.h
 *    Description:  This head file 
 *
 *        Version:  1.0.0(01/06/22)
 *         Author:  Nbiot <lingyun@gail.com>
 *      ChangeLog:  1, Release initial version on "01/06/22 19:05:56"
 *                 
 ********************************************************************************/

#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <sqlite3.h>
#include "logger.h"

extern int open_database(sqlite3 **db,char *database_name);
extern int create_table(sqlite3 **db,char *table_name,char *table_attribute);
extern int insert_data(sqlite3 **db,char *table_name,char *attr,char *msg);
extern int query_data(sqlite3 **db,char ***azResult,char *table_name);
extern int delete_data(sqlite3 **db,char *table_name);

#endif

