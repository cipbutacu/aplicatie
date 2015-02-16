/*
 * db-EXT.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef DB_EXT_H_
#define DB_EXT_H_

#include <pthread.h>

//for mysql
#include <mysql.h>

/*login defines*/
#define MYSQL_HOST			"localhost"
#define MYSQL_USERNAME		"root"
#define MYSQL_PASSWORD      "l3dyh@wk3"
#define MYSQL_DB 			"homey"
#define MYSQL_TABLE_NAME	"devices"

/*max length of a  connection detail data*/
#define MAX_LOGIN_DATA_LEN  30u

#define MAX_QUERY_STRING_LENGTH  255u

/*max number of parameters to pass to the update thread (here: 2 bytes) */
#define MAX_NR_OF_THREAD_PARAMS	2





/*mysql connection detail structure*/
typedef struct dbMysqlSettings_stype
{
	 MYSQL *conn_ptrst;
	 MYSQL_RES *res_ptrst;
	 MYSQL_ROW row_ptrst;
	 char server_au8[MAX_LOGIN_DATA_LEN];
	 char user_au8[MAX_LOGIN_DATA_LEN];
	 char password_au8[MAX_LOGIN_DATA_LEN]; /* set me first */
	 char database_au8[MAX_LOGIN_DATA_LEN];
	 char table_au8[MAX_LOGIN_DATA_LEN];
}MysqlDB_st;



/*EXTERN VARIABLES*/
/*mysql connection details*/
extern MysqlDB_st mysqlConnect_st;
extern MYSQL_RES *result_ptr;
/*number of fields in a query*/
extern uint32_t numOfFields_u16;
/*structure containing row data*/
extern MYSQL_ROW row_st;

/* this variable is our reference to the second thread */
extern pthread_t pthreadRefreshFromDB_st;

/*updates led status according to mysql DB*/
extern uint8_t RefreshStateFromDB(void);

extern void InitDB(void);
extern void CleanupDB(void);
extern uint8_t updateDBActDevice( uint8_t deviceIndex_lu8, uint8_t stateToSet_lu8);
extern uint8_t getRefreshNeeded(void);
extern uint8_t getNumberOfConfiguredDevices(void);
#endif /* DB_EXT_H_ */
