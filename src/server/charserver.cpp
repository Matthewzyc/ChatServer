#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include <string>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

//  初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr, const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    //  注册链接回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    //  注册读写回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    //  设置线程数量
    _server.setThreadNum(4);
}

//  启动服务；
void ChatServer::start()
{
    _server.start();
}

//  上报链接信息相关的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //  用户断开连接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

//  上报读写事件相关的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    //  数据反序列化
    json js = json::parse(buf);
    //  调用在其他文件中写好的业务代码，实现网络模块和业务模块的解耦
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //  回调消息绑定好的事假处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}