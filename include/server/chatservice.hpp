#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include "redis.hpp"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "offlinemessagemodel.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

//  处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;

// 业务类
class ChatService
{
public:
    //  获取单例对象的接口函数
    static ChatService* instance();
    //  处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  处理注册业务
    void regist(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  处理客户端注销业务
    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);    
    //  一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  群组聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //  处理redis服务器订阅的消息
    void handleRedisSubscribeMessage(int userid, string msg);
    //  处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    //  处理服务器异常，重置业务
    void reset();
    //  获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    //  单例模式，构造函数私有化
    ChatService();
    //  存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;
    //  存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    //  定义互斥锁，保护_userConnMap
    mutex _connMutex;

    //  实例化一个用户数据操作类对象
    UserModel _userModel;
    //  实例化一个好友列表操作类对象
    FriendModel _friendmodel;
    //  实例化一个群组操作类对象
    GroupModel _groupModel;
    //  实例化一个离线消息操作类对象
    OfflineMsgModel _offlineMsgModel;
    //  实例化一个Redis操作类对象
    Redis _redis;
};

#endif