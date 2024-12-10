#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

/* GroupUser类可以直接从User类继承，因为groupuser也是user，
    具有和普通user一样的信息类型，只需要添加一个role信息就行了 */
class GroupUser : public User
{
public:
    void setRole(string role) {this->role = role;}
    string getRole() {return this->role;}

private:
    string role;
};

#endif