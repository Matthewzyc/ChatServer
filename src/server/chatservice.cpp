#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>

using namespace std;
using namespace muduo;

//  获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

//  定义消息并注册对应的回调函数
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REGIST_MSG, std::bind(&ChatService::regist, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    //  连接redis服务器
    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

//  获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //  记录错误日志， msgid没有对应的事件处理回调
    if (_msgHandlerMap.find(msgid) == _msgHandlerMap.end())
    {
        //  返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

//  处理登录业务
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    json response;

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            //  该用户已经登录，重复登录
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";         
        }
        else
        {
            /*
                登录成功
            */

            //  记录用户连接
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            //  id用户登录成功后，向redis订阅
            _redis.subscribe(id);

            //  改变用户登录状态
            user.setState("online");
            _userModel.updateState(user);

            //  反馈登录成功信息
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();   

            //  查询该用户是否有离线储存的信息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                //  读取完毕后清空该用户离线消息
                _offlineMsgModel.remove(id);
            }

            //  查询该用户的好友信息并返回
            vector<User> friendVec = _friendmodel.query(id);
            if (!friendVec.empty())
            {
                vector<string> infoVec;
                for (User &user : friendVec)
                {
                    json infoJs;
                    infoJs["id"] = user.getId();
                    infoJs["name"] = user.getName();
                    infoJs["state"] = user.getState();
                    infoVec.push_back(infoJs.dump());
                }
                response["friends"] = infoVec;
            }

            //  查询用户的群组信息
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> gpInfoVec;
                for (Group &group : groupVec)
                {
                    json gpInfoJs;
                    gpInfoJs["id"] = group.getId();
                    gpInfoJs["groupname"] = group.getName();
                    gpInfoJs["groupdesc"] = group.getDesc();
                    vector<string> userVec;
                    for (GroupUser &user : group.getUser())
                    {
                        json infoJs;
                        infoJs["id"] = user.getId();
                        infoJs["name"] = user.getName();
                        infoJs["state"] = user.getState();
                        infoJs["role"] = user.getRole();
                        userVec.push_back(infoJs.dump());
                    }
                    gpInfoJs["users"] = userVec;
                    gpInfoVec.push_back(gpInfoJs.dump());
                }
                response["groups"] = gpInfoVec;
            }
        }
    }
    else
    {
        //  未来做出具体业务实现时可做细分，先合并处理
        if (user.getId() == id)
        {
            //  用户存在，但是密码错误
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "password is invalid!";              
        }
        else if (user.getId() == -1)
        {
            //  用户不存在
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "id does not exist!";
        }
    }            
    conn->send(response.dump());   
}

//  处理注册业务
void ChatService::regist(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        //  注册成功
        json response;
        response["msgid"] = REGIST_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        //  注册失败
        json response;
        response["msgid"] = REGIST_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

//  一对一业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    //  判断用户是否在线
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            //  toid在线，转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    } 

    //  toid不在线，消息离线保存
    _offlineMsgModel.insert(toid, js.dump());
}

//  添加好友业务
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["userid"].get<int>();
    int friendid = js["friendid"].get<int>();

    //  存储好友信息
    _friendmodel.insert(userid, friendid);
}

//  创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //  存储创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        //  存储群组创建人的信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

//  加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

//  群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUser(userid, groupid);
    
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {

        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            //  转发群消息
            it->second->send(js.dump());
        }
        else
        {
            //  查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                //  存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

//  处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                //  记录准备更新状态的用户的id
                user.setId(it->first);
                //  从map表删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //  用户注销，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //  更新状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);        
    }
}

void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}

//  处理客户端注销业务
void ChatService::logout(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //  用户注销，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //  更新状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);        
}

//  处理服务器异常，重置业务
void ChatService::reset()
{
    // 把所有online状态用户设置为offline
    _userModel.resetState();
}