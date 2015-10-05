#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <mysql.h>
#include "mysqlhelper.h"

using namespace std;

/**
 *  功能描述:打印最后一次错误信息
 *  @param conn 数据库连接句柄
 *  @param msg 消息头
 *
 *  @return 无
 *  */
void printDBError(void *conn, const char *msg)
{
    if (msg) {
        printf("%s: %s\n", msg, mysql_error((MYSQL *)conn));
    } else {
        puts(mysql_error((MYSQL *)conn));
    }
}

/**
 *  功能描述:执行SQL语句
 *  @param conn 数据库连接句柄
 *  @param sql SQL语句
 *
 *  @return 成功返回0，失败返回其他值
 *  */
int executeSQL(void *conn, const char *sql)
{
    /*query the database according the sql*/
    if (mysql_real_query((MYSQL *)conn, sql, strlen(sql))) { // 如果失败
        return -1;    // 表示失败
    }

    return 0; // 成功执行
}

/**
 *  功能描述:初始化数据库连接
 *  @param hostName 数据库所在的主机名或IP地址
 *  @param userName 用户名
 *  @param passWord 密码
 *  @param tblName  表名
 *  @param dbPort   端口
 *  @param conn 输出参数，数据库连接句柄
 *
 *  @return 成功返回0，失败返回其他值
 *  */
int initDBConnect(const char *hostName,
                  const char *userName,
                  const char *passWord,
                  const char *tblName,
                  int dbPort,
                  void **conn)
{
    // init the database connection
    *conn = mysql_init(NULL);

    /* connect the database */
    if (!mysql_real_connect((MYSQL *)*conn, hostName, userName, passWord,
                            tblName, dbPort, NULL, 0)) { // 如果失败
        fprintf(stderr, "init connect failed\n");
        return -1;
    }

    // 是否连接已经可用
    if (executeSQL(*conn, "set names utf8")) { // 如果失败
        return -1;
    }

    return 0; // 返回成功
}

/**
 *  功能描述:关闭数据库连接
 *  @param conn 数据库连接句柄
 *
 *  @return 无
 *  */
void closeDBConnect(void *conn)
{
    mysql_close((MYSQL *)conn);
    return;
}

/**
 *  功能描述:关闭SQL语句句柄
 *  @param st   预编译后的句柄
 *
 *  @return 无
 *  */
void closeStmtHandle(void *st)
{
    mysql_stmt_close((MYSQL_STMT *)st);
    return;
}

/**
 *  功能描述:预编译MYSQL SQL语句
 *  @param conn 数据库连接
 *  @param sql  SQL字符串语句
 *  @param length SQL字符串语句长度
 *  @param st   输出参数，保存预编译后的句柄
 *
 *  @return 成功返回0,失败返回其他值
 *  */
int prepareStatement(void **raw_st, void *raw_conn, const char *sql, size_t length)
{
    MYSQL *conn = (MYSQL *)raw_conn;
    MYSQL_STMT **st = (MYSQL_STMT **)raw_st;

    printf("[INFO]prepare sql=%s\n", sql);

    *st =  mysql_stmt_init(conn);

    if (*st == NULL) {
        printDBError(conn, "[ERROR]prepareStatement init failed");
        return -1;
    }

    if (mysql_stmt_prepare(*st, sql, length)) {
        printDBError(conn, "[ERROR]prepareStatement prepare failed");
        return -1;
    }

    return 0;
}

/**
 *  功能描述:执行SQL插入的钩子函数
 *  @param raw_conn 数据库连接
 *  @param raw_st 预编译后的语句句柄
 *  @param columnsType 每一个列的值的类型
 *  @param valueList 从xml中获取的单行的值列表
 *
 *  @return 成功返回0,失败返回其他值
 *  */
int ExecuteImportSQLHook(void *raw_conn,
                         void *raw_st,
                         const vector<string> &columnsType,
                         const vector<const char *> &valueList)
{
    int n = valueList.size();

    int numbers[n];
    MYSQL_BIND  params[n];

    MYSQL *conn = (MYSQL *)raw_conn;
    MYSQL_STMT *st = (MYSQL_STMT *)raw_st;

    memset(numbers, 0, sizeof(numbers));
    memset(params, 0, sizeof(params));

    for (size_t i = 0 ; i < valueList.size(); ++i) {

        if (columnsType[i] == "long") {
            numbers[i] = atoi(valueList[i]);

            params[i].buffer_type = MYSQL_TYPE_LONG; //设置参数的数据类型
            params[i].buffer = &numbers[i]; //参数传值
        } else if (columnsType[i] == "string") {
            params[i].buffer_type = MYSQL_TYPE_STRING; //设置参数的数据类型
            params[i].buffer = (void *)valueList[i];
            params[i].buffer_length = strlen(valueList[i]);
        } else {
            fprintf(stderr, "[ERROR] unknow filed type:[%s]\n", columnsType[i].c_str());
            return -1;
        }
    }

    if (mysql_stmt_bind_param(st, params)) {
        printDBError(conn, "[ERROR]bind error\n");
        return -1;
    }

    if (mysql_stmt_execute(st)) {         //执行与语句句柄相关的预处理
        printDBError(conn, "[ERROR]exec bind error\n");
        return -1;
    }

    return 0;
}

/**
 *  功能描述:拼接需要预编译的SQL语句,形如
 *    "INSERT INTO `database`.`tblName`(filedList) VALUES(?,?,...,?)"
 *  @param
 *
 *  @return 成功返回SQL的语句字符串，失败返回空字符串
 *  */
string joinSql(const string &database, const string &tblName, const vector<string> &columnsTblName)
{
    string filedList;
    string valueList;
    string sql;

    for (size_t i = 0; i < columnsTblName.size(); ++i) {
        filedList += string("`") + columnsTblName[i] + string("`");
        filedList += ",";
        valueList += "?,";
    }

    if (filedList.length() > 1) {
        filedList.erase(filedList.length() - 1);
    }

    if (valueList.length() > 1) {
        valueList.erase(valueList.length() - 1);
    }


    /* "INSERT INTO `database`.`tblName`(filedList) VALUES(valueList)"  */
    sql = string("INSERT INTO `") + database + string("`.`") + tblName + string("`(")
          + filedList + string(") VALUES(")
          + valueList + string(")");

    return sql;
}


/**
 *  功能描述:预编译要导入的SQL语句
 *  @param conn 数据库连接
 *  @param database 数据库名
 *  @param tblName 表名
 *  @param columnsTblName 表中的字段名
 *  @param st   输出参数，保存预编译后的句柄
 *
 *  @return 成功返回0,失败返回其他值
 *  */
int prepareInsertStatement(void *conn,
                           const string &database,
                           const string &tblName,
                           const vector<string> columnsTblName,
                           void **st)
{
    string sql = joinSql(database, tblName, columnsTblName);

    return prepareStatement(st, conn, sql.c_str(), sql.length());
}
