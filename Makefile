CC = gcc
C_NAME = 4g
C_NAME2 = logger.c cJSON.c database.c
DEST_NAME = 4g_sms_modem

LIBPATH_A = ../sqlite3/lib/
LIBPATH_B = ../libxml2/lib/
LIBPATH_C = ../iniparser-3.1/
LIB_A += -L ${LIBPATH_A} -lsqlite3 -lm -lpthread
LIB_B += -L ${LIBPATH_B} -lxml2 -lmosquitto
LIB_C += -L ${LIBPATH_C} -liniparser

CLIENT_HF_A += -I ../sqlite3/include/
CLIENT_HF_B += -I ../libxml2/include/libxml2

all:
        @echo "please input 'make 4g'"

test:
        #客户端执行(@可关闭本条语句)

4g:
        ${CC} ${C_NAME}_*.c ${C_NAME2} -g -o ${DEST_NAME} ${CLIENT_HF_A} ${LIB_A} ${CLIENT_HF_B} ${LIB_B} ${LIB_C}

clean:
        $(RM) ${DEST_NAME}