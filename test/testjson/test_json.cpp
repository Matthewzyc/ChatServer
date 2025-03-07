#include "json.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;
using json = nlohmann::json;

void func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    string sendBuf = js.dump();
    cout << sendBuf << endl;
    cout << js << endl;
}

void func2()
{
    json js;
    js["id"] = {1, 2, 3, 4, 5};
    js["name"] = "zhang san";
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    //下面这句和上面两句是等价的；
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
}

//json还可以直接序列化容器
void func3()
{
    json js;

    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "泰山"});
    m.insert({3, "华山"});

    js["list"] = vec;
    js["path"] = m;

    cout << js["list"] << endl;
}

string func4()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    string sendBuf = js.dump();
    return sendBuf;
}

int main()
{
    string recvBuf = func4();
    json recvJs = json::parse(recvBuf);

    cout << recvJs["msg_type"] << endl;
    cout << recvJs["from"] << endl;
    cout << recvJs["to"] << endl;
    cout << recvJs["msg"] << endl;

    return 0;
}