#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //  初始化聊天服务器对象
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);
    
    //  启动服务
    void start();

private:
    //  上报链接信息相关的回调函数
    void onConnection(const TcpConnectionPtr& conn);

    //  上报读写事件相关的回调函数
    void onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp time);

    TcpServer _server;
    EventLoop* _loop;
};

#endif