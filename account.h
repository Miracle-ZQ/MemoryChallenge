/*账号模块的头文件。
定义用户结构体 User，并声明登录页、注册页。
需要的三个页面函数：resetAccountPage、updateAccountPage、drawAccountPage
进入页面前调 reset，之后每帧先调update 处理输入，再调 draw 绘制。*/
#ifndef ACCOUNT_H  // 防止本头文件被重复包含
#define ACCOUNT_H

#include "raylib.h"  // 页面函数要用到 Font 类型

#define NAME_MAX     32  // 用户名字符数组长度：31 个字符 + 结束符 '\0'
#define PASSWORD_MAX 64  // 密码字符数组长度：63 个字符 + 结束符 '\0'

/*User结构体，保存一个用户的登录信息，与 users.txt 中的一行记录一一对应。
*/
typedef struct
{
    char name[NAME_MAX];  // 用户名，最多 31 字节，不含空白字符
    char password[PASSWORD_MAX];  // 登录密码，最多 63 字节，不含空白字符
} User;

/*resetAccountPage函数，准备登录页或注册页，把页面恢复成刚进入时的样子
（清空用户名、密码、确认密码三个输入框，焦点回到用户名框，清除提示文字。）
*/
void resetAccountPage(int registerMode);//registerMode 为 0 显示登录页，为 1 显示注册页

/*updateAccountPage函数，处理这一帧的键盘输入（输入字符、退格、回车提交）
和鼠标点击（点输入框切换焦点、点按钮），完成一次登录或注册。
注册页要求两次输入的密码一致，并且密码强度达到中等及以上，否则不写文件。
返回值为0 表示留在账号页继续显示；1 表示登录成功，main.c 可以切到玩家菜单；
2 表示用户点了“返回”，由 main.c 决定回到入口菜单。
*/
int updateAccountPage(User *user);//user 是接收登录账号的结构体地址，由 main.c 提供

/*drawAccountPage函数，把账号页画出来，
包括标题、输入框（注册页多一个确认密码框）、按钮、密码强度提示和页面底部的操作提示。
 */
void drawAccountPage(Font font);//font 是已经加载好中文字形的字体，由 main.c 传入

#endif  // ACCOUNT_H
