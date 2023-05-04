#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"
#include "../log/log.h"
#include "../locker/locker.h"

using namespace std;

void connection_pool::init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int maxConn){
    m_url = url; 
    m_user = User;
    m_password = PassWord; 
    m_dbname = DataBaseName; 
    m_port = Port;
    MaxConn = maxConn;
    lock.lock();
    for(int i = 0; i < MaxConn; i++){
        MYSQL* con = nullptr;
        con = mysql_init(con);
        if(con == nullptr){
            //LOG_ERROR("[%s:%d]: mysql_init false!", __FILE__, __LINE__);
            cout << "Error:" << mysql_error(con);
            exit(1);
        }
        con = mysql_real_connect(con, m_url.c_str(), m_user.c_str(), m_password.c_str(), m_dbname.c_str(), Port, nullptr, 0);
        if(con == nullptr){
            cerr << "Failed to connect to database: " << mysql_error(con) << endl;
            LOG_ERROR("[%s:%d]: mysql_real_connect false!", __FILE__, __LINE__);
            exit(1);
        }
        connlist.push_back(con);
        ++FreeConn;
    }
    reserve = sem(FreeConn);
    MaxConn = FreeConn; /* 更新为实际创建成功的mysql连接数 */
    lock.unlock();
}

/* 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数 */
MYSQL * connection_pool::GetConnection(){
    MYSQL* con = nullptr;
    if(0 == connlist.size()){
        return nullptr;
    }
    reserve.wait();
    lock.lock();
    con = connlist.front();
    connlist.pop_front();
    --FreeConn;
    ++CurConn;
    lock.unlock();
    return con;
}

/* 释放当前使用的连接 */
bool connection_pool::ReleaseConnection(MYSQL* conn){
    if(nullptr == conn){
        return false;
    }
    lock.lock();
    connlist.push_back(conn);
    ++FreeConn;
    --CurConn;
    lock.unlock();
    reserve.post(); /* +1操作 */
    return true;
}

/* 销毁数据库连接池，所有连接 */
void connection_pool::DestroyPool(){
    lock.lock();
    if(connlist.size() > 0){
        list<MYSQL*>::iterator it;
        for(it = connlist.begin(); it != connlist.end(); it++){
            MYSQL *con = *it;
            mysql_close(con);
        }
        CurConn = 0;
        FreeConn = 0;
        connlist.clear();
    }
    lock.unlock();
}

/* 不直接调用获取和释放连接的接口，将其封装起来，通过RAII机制进行获取和释放。 */
connectionRAII::connectionRAII(MYSQL **SQL, connection_pool* connPool){
    *SQL = connPool->GetConnection();
    m_conRAII = *SQL;
    m_poolRAII = connPool;
}
/* 可以不用自己释放，RAII对象析构时自动释放连接 */
connectionRAII::~connectionRAII(){
    m_poolRAII->ReleaseConnection(m_conRAII);
}









