/* 
muduo网络库主要提供两个类
TcpServer ：用来实现服务器程序
TcpClient  ：用来实现客户端程序

muduo库本质上就是封装好的epoll + 线程池
使得开发过程中能够把精力集中在业务代码上

主要暴露两个接口 ：用户的连接和断开
                                        用户的可读写事件
 */

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

//基于muduo网络库开发服务器；
class ChatServer
{
public:
    //根据TcpServer的参数确定构造函数的参数；
    ChatServer(EventLoop* loop, 
                            const InetAddress& listenAddr, 
                            const string& nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        _server.setThreadNum(4);
    }

    void start()
    {
        _server.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << "->" 
                << conn->localAddress().toIpPort() << " state : online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << "->" 
                << conn->localAddress().toIpPort() << " state : offline" << endl;
            conn->shutdown();
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data : " << buf << " time : " << time.toString() << endl;
        conn->send(buf);
    }

    TcpServer _server;
    EventLoop* _loop;   //方便服务器运行结束；
};

int main()
{
    EventLoop loop; 
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}
