#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../locker/locker.h"
using namespace std;


/* 单例懒汉模式，静态成员变量 */
class connection_pool
{
public:
    void init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn);
    /* 单例模式 */
    //此处一定是static，因为不能new实例，这个public函数接口，一定是属于类的，即类可以直接调用，因此是static；
    static connection_pool*  GetInstance(){
        static connection_pool connPool;
        return &connPool;
    }; 
    MYSQL * GetConnection();    /* 获取数据库连接 */
    bool ReleaseConnection(MYSQL* conn);    /* 释放连接,放回连接池 */
    void DestroyPool();   /* 销毁所有连接 */

    int GetFreeConn(){ return FreeConn; };  /* 获取当前连接数 */

private:
    connection_pool(){
        CurConn = 0;
        FreeConn = 0;
    };
    ~connection_pool(){ DestroyPool(); };

private:
    unsigned int MaxConn;   /* 最大连接数 */
    unsigned int CurConn;   /* 当前已使用的连接数 */
    unsigned int FreeConn;  /* 当前空闲的连接数 */

private:
    locker lock;
    list<MYSQL*> connlist;  /* 连接池 */
    sem reserve;//信号量

private:
    string m_url; /* 主机地址 */
    string m_port;  /* 数据库端口号 */
    string m_user;    /* 数据库用户名 */
    string m_password;    /* 数据库密码 */
    string m_dbname;    /* 数据库名 */
};

class connectionRAII{
public:
    connectionRAII(MYSQL **conn, connection_pool* connPool);
    ~connectionRAII();
private:
    MYSQL* m_conRAII;
    connection_pool* m_poolRAII;
};







#endif