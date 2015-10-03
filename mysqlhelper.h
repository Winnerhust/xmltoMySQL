#ifndef MYSQLHELPER_H
#define MYSQLHELPER_H

#include <string>
#include <vector>

using namespace std;
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
                  void **conn);

/**
 *  功能描述:关闭数据库连接
 *  @param conn 数据库连接句柄
 *
 *  @return 无
 *  */
void closeDBConnect(void *conn);

/**
 *  功能描述:关闭SQL语句句柄
 *  @param st   预编译后的句柄
 *
 *  @return 无
 *  */
void closeStmtHandle(void *st);

/**
 *  功能描述:预编译MYSQL SQL语句
 *  @param conn 数据库连接
 *  @param sql  SQL字符串语句
 *  @param length SQL字符串语句长度
 *  @param st   输出参数，保存预编译后的句柄
 *
 *  @return 成功返回0,失败返回其他值
 *  */
int prepareStatement(void **st, void *conn, const char *sql, size_t length);

/**
 *  功能描述:执行SQL语句
 *  @param conn 数据库连接句柄
 *  @param sql SQL语句
 *
 *  @return 成功返回0，失败返回其他值
 *  */
int executeSQL(void *conn, const char *sql);

/**
 *  功能描述:打印最后一次错误信息
 *  @param conn 数据库连接句柄
 *  @param msg 消息头
 *
 *  @return 无
 *  */
void printDBError(void *conn, const char *msg);


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
                         const vector<const char *> &valueList);
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
                           void **st);
#endif
