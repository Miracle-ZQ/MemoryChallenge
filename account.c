/*图形版账号模块。负责登录页、注册页的输入处理、账号校验和页面绘制，
账号数据保存在文本文件 users.txt 中。
注册页要求二次输入密码，并且密码强度至少为中等才允许注册成功。
*/

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>  // islower、isupper、isdigit 字符类别判断
#include <errno.h>  // errno、ENOENT 用来区分“文件还不存在”
#include "account.h"  // 本模块的结构体与三个页面函数声明
#include "ui.h"  // 组内公共按钮：isButtonClicked、drawButton

#define USERS_FILE "users.txt"  // 账号文件名，与程序在同一目录
#define USER_MAX 100  // 一次最多读入内存的账号个数

/*账号页的布局常量。
*/
#define PAGE_CENTER_X    400.0f  // 窗口水平中心，标题和提示按它居中
#define PANEL_X          140.0f  // 中间白色面板的左上角横坐标
#define PANEL_Y           26.0f  // 中间白色面板的左上角纵坐标
#define PANEL_W          520.0f  // 面板宽度
#define PANEL_H          548.0f  // 面板高度
#define INPUT_X          220.0f  // 输入框左边缘
#define INPUT_W          360.0f  // 输入框宽度
#define INPUT_H           54.0f  // 输入框高度
#define NAME_BOX_Y       150.0f  // 用户名输入框上边缘
#define PASSWORD_BOX_Y   238.0f  // 密码输入框上边缘
#define CONFIRM_BOX_Y    344.0f  // 确认密码框上边缘，只有注册页才画
#define STRENGTH_Y       296.0f  // 密码强度提示的纵坐标
#define BUTTON_Y         414.0f  // 普通按钮上边缘
#define BUTTON_W         170.0f  // 普通按钮宽度
#define BUTTON_H          52.0f  // 按钮高度
#define BUTTON_LEFT_X    220.0f  // 左边那个按钮的左边缘
#define BUTTON_RIGHT_X   410.0f  // 右边那个按钮的左边缘
#define MESSAGE_Y        482.0f  // 提示文字的纵坐标
#define FOOT_Y           520.0f  // 页脚说明文字的纵坐标

// 固定区域的常量，更新和绘制都用这几个矩形，保证点到的位置和画出来的位置一致
const Rectangle nameBox = {INPUT_X, NAME_BOX_Y, INPUT_W, INPUT_H};
const Rectangle passwordBox = {INPUT_X, PASSWORD_BOX_Y, INPUT_W, INPUT_H};
const Rectangle confirmBox = {INPUT_X, CONFIRM_BOX_Y, INPUT_W, INPUT_H};
const Rectangle primaryButton = {BUTTON_LEFT_X, BUTTON_Y, BUTTON_W, BUTTON_H};
const Rectangle backButton = {BUTTON_RIGHT_X, BUTTON_Y, BUTTON_W, BUTTON_H};

//状态变量，保存账号页面当前的状态（登录页和注册页共用）
char  gName[NAME_MAX];  // 用户名输入框里的字符串
int   gNameLength;  // 用户名已经输入的字节数
char  gPassword[PASSWORD_MAX];  // 密码输入框里的字符串
int   gPasswordLength;  // 密码已经输入的字节数
char  gConfirm[PASSWORD_MAX];  // 确认密码框里的字符串，只有注册页用
int   gConfirmLength;  // 确认密码已经输入的字节数
int   gFocus;  // 当前焦点：0 用户名框，1 密码框，2 确认密码框
int   gRegisterMode;  // 当前表单：0 登录，1 注册
char  gMessage[128];  // 页面底部的提示文字
int   gMessageIsError;  // 提示文字颜色：1 红色错误，0 绿色
int   gLoadError;  // 1 表示读 users.txt 时发生了错误

//页面用到的颜色
const Color COLOR_PANEL       = { 248, 249, 251, 255 };  // 面板底色
const Color COLOR_PANEL_LINE  = { 205, 211, 219, 255 };  // 面板边框
const Color COLOR_BOX_FILL    = { 255, 255, 255, 255 };  // 输入框底色
const Color COLOR_BOX_LINE    = { 190, 196, 205, 255 };  // 输入框边框
const Color COLOR_FOCUS       = {  52, 101, 164, 255 };  // 焦点高亮
const Color COLOR_TITLE       = {  32,  58, 104, 255 };  // 标题文字
const Color COLOR_LABEL       = {  70,  78,  90, 255 };  // 输入框标题
const Color COLOR_TEXT        = {  20,  20,  20, 255 };  // 输入文字
const Color COLOR_PLACEHOLDER = { 160, 166, 176, 255 };  // 占位提示
const Color COLOR_OK          = {  22, 120,  60, 255 };  // 成功提示
const Color COLOR_ERROR       = { 176,  48,  48, 255 };  // 错误提示
const Color COLOR_HINT        = { 110, 118, 130, 255 };  // 次要说明
const Color COLOR_FOOT        = { 130, 136, 148, 255 };  // 页脚说明

// 以下是本文件内部使用的函数
int loadUsers(User users[], int maxCount);
int findUser(const User users[], int count, const char name[]);
void setMessage(const char *text, int isError);
void clearInputs(void);
void buildUserFromForm(User *user);
void handleCharInput(void);
void handleBackspace(void);
int tryLogin(User *user);
int registerCurrentForm(void);
int saveCurrentUser(void);
void drawTextAt(Font font, const char *text, float x, float y, int fontSize,
                       Color color);
void drawTextCentered(Font font, const char *text, float centerX, float y,
                             int fontSize, Color color);
void drawInputBox(Font font, Rectangle box, const char *text,
                         const char *placeholder, int focused);
void drawStrengthHint(Font font);

/*checkPassword函数,检测密码安全等级。
返回值：0 不安全（不足 6 位，或只有一类字符）；1 中等（两类字符）；
2 安全（三类及以上字符）。数字、大写字母、小写字母、其它符号分别算作四类不同的字符
被registerCurrentForm函数引用：注册时用来判断要不要弹弱密码确认
被drawStrengthHint函数引用：显示代码安全等级时用来判断
*/
int checkPassword(char password[])
{
    int length;  // 密码的字符个数（不含结束符）
    int hasLower = 0;  // 是否含小写字母：1 有，0 无
    int hasUpper = 0;  // 是否含大写字母：1 有，0 无
    int hasDigit = 0;  // 是否含数字：1 有，0 无
    int hasOther = 0;  // 是否含其它符号：1 有，0 无
    int kinds;  // 密码中实际出现的字符种类数（0～4）
    int i;  // 扫描密码用的下标
    unsigned char ch;  // 当前字符

    length = (int)strlen(password);  // 取密码长度
    if (length < 6)  // 长度不足 6 位一律不安全
    {
        return 0;
    }

    for (i = 0; i < length; i++)  // 逐个字符统计所属类别
    {
        ch = (unsigned char)password[i];
        if (islower(ch))
        {
            hasLower = 1;
        }
        else if (isupper(ch))
        {
            hasUpper = 1;
        }
        else if (isdigit(ch))
        {
            hasDigit = 1;
        }
        else
        {
            hasOther = 1;  // 其它符号也算一类
        }
    }

    kinds = hasLower + hasUpper + hasDigit + hasOther;  // 出现过的类别数
    if (kinds <= 1)  // 只有一类：不安全
    {
        return 0;
    }
    if (kinds == 2)  // 两类：中等安全
    {
        return 1;
    }
    return 2;  // 三类及以上：安全
}

/*registerUser函数，把新用户追加写入 users.txt，一行一个“用户名 密码”
返回值：1 注册成功；0 用户名重复、参数为空或文件读写失败
*/
int registerUser(User user)
{
    FILE *fp;  // 指向 users.txt 的文件指针
    fp = fopen(USERS_FILE, "a");  // 追加打开，文件不存在时自动创建
    if (fp == NULL)
    {
        return 0;  // 文件建立失败
    }

    // 每条记录写成“用户名 密码”一行，中间用一个空格分隔
    if (fprintf(fp, "%s %s\n", user.name, user.password) < 0)
    {
        fclose(fp);
        return 0;  // 写入失败
    }

    if (fclose(fp) != 0)  // 关闭失败说明数据可能没完整落盘
    {
        return 0;
    }
    return 1;  // 注册成功
}

/*loginUser函数：验证用户名和密码是否与账号文件中的某一行完全一致
返回值：1 两项都匹配，登录成功；0 用户名不存在或密码错误
*/
int loginUser(User user)
{
    User users[USER_MAX];  // 从文件读入的账号数组
    int count;  // 实际读入的账号个数
    int i;  // 遍历账号数组用的下标

    count = loadUsers(users, USER_MAX);  // 读入全部账号
    for (i = 0; i < count; i++)
    {
        // 用户名和密码必须同时相同才算登录成功
        if (strcmp(users[i].name, user.name) == 0 &&
            strcmp(users[i].password, user.password) == 0)
        {
            return 1;
        }
    }
    return 0;  // 用户名不存在或密码错误
}

/*loadUsers函数：把 users.txt 中的账号读入结构体数组，同时更新 gLoadError 标记读取是否出错
返回值：实际读入的账号个数；文件不存在时返回 0（首次运行视为空表）
*/
int loadUsers(User users[], int maxCount)//users：接收账号的数组 
                                                //maxCount：数组最多能装下的账号个数
{
    FILE *fp;  // 指向 users.txt 的文件指针
    int count = 0;  // 已经读入的账号个数

    gLoadError = 0;  // 每次读取前先清掉上一次的错误标记

    fp = fopen(USERS_FILE, "r");
    if (fp == NULL)  // 打开失败：区分“还没有文件”和“真的出错”
    {
        if (errno != ENOENT)  // ENOENT 表示文件不存在，属首次运行
        {
            gLoadError = 1;
        }
        return 0;
    }

    // 每次读两个字段，读到文件末尾或数组装满就停止
    while (count < maxCount &&
           fscanf(fp, "%31s %63s", users[count].name, users[count].password) == 2)
    //"%31s %63s" 对应 name 的 31 字节和 password 的63 字节，防止超长字符串损坏数组
    {
        count++;
    }

    if (ferror(fp) != 0)  // 读的过程中出错，提示调用者
    {
        gLoadError = 1;
    }

    fclose(fp);
    return count;
}

/* findUser函数，在已经读入的账号数组里按用户名查找
返回值：找到时返回该账号在数组中的下标（从 0 开始），找不到返回 -1
*/
int findUser(const User users[], int count, const char name[])//users、count：账号数组和它的有效长度
                                                                     //name：要找的用户名
{
    int i;  // 遍历数组用的下标

    for (i = 0; i < count; i++)
    {
        if (strcmp(users[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

/*setMessage函数，设置页面底部的提示文字和颜色，输出时更新 gMessage 和 gMessageIsError
*/
void setMessage(const char *text, int isError)//text：提示内容；isError：1 表示红色错误，0 表示绿色正常
{
    strncpy(gMessage, text, sizeof(gMessage) - 1);  // 留出结束符的位置
    gMessage[sizeof(gMessage) - 1] = '\0';
    gMessageIsError = (isError != 0) ? 1 : 0;
}

/*clearInputs函数，把三个输入框清空(清空 gName、gPassword、gConfirm 和它们的长度)，
焦点回到用户名框
*/
void clearInputs(void)
{
    gName[0] = '\0';  // 用户名框清空
    gNameLength = 0;
    gPassword[0] = '\0';  // 密码框清空
    gPasswordLength = 0;
    gConfirm[0] = '\0';  // 确认密码框清空
    gConfirmLength = 0;
    gFocus = 0;  // 焦点回到用户名框
}

/*buildUserFromForm函数，把两个输入框的内容复制成一个 User 结构体
输出时把用户名和密码写进 *user
*/
void buildUserFromForm(User *user)
{
    strcpy(user->name, gName); 
    strcpy(user->password, gPassword);
}

/*handleCharInput函数，取出这一帧已经排队的按键字符，写进当前有焦点的输入框
输出时更新 gName/gPassword/gConfirm 及其长度
只接受可打印 ASCII 字符（33～126），空格、控制字符、中文
*/
void handleCharInput(void)
{
    int code;  // 本帧读到的一个字符编码，0 表示队列已空
    char *text;  // 当前有焦点的输入框
    int *length;  // 当前输入框已经输入的字节数
    int limit;  // 当前输入框的长度上限，要留一位给结束符

    if (gFocus == 0)
    {
        text = gName;
        length = &gNameLength;
        limit = NAME_MAX - 1;
    }
    else if (gFocus == 1)
    {
        text = gPassword;
        length = &gPasswordLength;
        limit = PASSWORD_MAX - 1;
    }
    else
    {
        text = gConfirm;
        length = &gConfirmLength;
        limit = PASSWORD_MAX - 1;
    }

    while ((code = GetCharPressed()) > 0)
    {
        // 只收可打印 ASCII，超长或多字节字符直接丢掉
        if (code < 33 || code > 126 || *length >= limit)
        {
            continue;
        }
        text[*length] = (char)code;
        (*length)++;
        text[*length] = '\0';  // 末尾始终留结束符
    }
}

/*handleBackspace函数，按下退格键时删掉当前焦点框末尾的一个字符
输出时更新对应输入框的内容和长度
*/
void handleBackspace(void)
{
    char *text;  // 当前有焦点的输入框
    int *length;  // 当前输入框已经输入的字节数

    if (gFocus == 0)
    {
        text = gName;
        length = &gNameLength;
    }
    else if (gFocus == 1)
    {
        text = gPassword;
        length = &gPasswordLength;
    }
    else
    {
        text = gConfirm;
        length = &gConfirmLength;
    }

    if (!IsKeyPressed(KEY_BACKSPACE) || *length <= 0)  // 没按退格或框已空
    {
        return;
    }
    (*length)--;  // 删掉末尾一个字符
    text[*length] = '\0';
}

/*tryLogin函数，用输入框里的用户名和密码做一次登录校验
返回值：1 登录成功；0 留在账号页（空输入、密码错误或文件出错）
*/
int tryLogin(User *user)//user：登录成功后接收账号的结构体地址
{
    User input;  // 本次尝试登录的账号

    if (gNameLength == 0)  // 用户名为空
    {
        setMessage("请输入用户名", 1);
        gFocus = 0;
        return 0;
    }
    if (gPasswordLength == 0)  // 密码为空
    {
        setMessage("请输入密码", 1);
        gFocus = 1;
        return 0;
    }

    buildUserFromForm(&input);  // 先拷到临时变量再校验

    if (loginUser(input) == 1)
    {
        buildUserFromForm(user);  // 只有成功才写回调用者的变量
        setMessage("登录成功", 0);
        return 1;
    }

    // 文件读取出错和密码错误要分开提示
    setMessage((gLoadError != 0) ? "账号文件读取失败，请检查 users.txt"
                                 : "用户名或密码错误", 1);
    gFocus = 1;
    return 0;
}

/*registerCurrentForm函数，用注册表单里的内容做一次注册
先检查空值、两次密码是否一致、密码强度，再检查重名和账号数量
密码强度不足时直接拒绝，用户必须把密码改到中等及以上才能注册成功
输出时可能在页面上显示提示，并把焦点移到需要修改的输入框
返回值：0 始终留在注册页（注册成功由 saveCurrentUser 切换成登录表单）
*/
int registerCurrentForm(void)
{
    User users[USER_MAX];  // 用来数一下已经有多少个账号
    User newUser;  // 本次准备注册的新用户
    int count;  // 已有账号个数
    int strength;  // 密码安全等级 0/1/2

    if (gNameLength == 0)  // 1. 用户名不能为空
    {
        setMessage("用户名不能为空", 1);
        gFocus = 0;
        return 0;
    }
    if (gPasswordLength == 0)  // 2. 密码不能为空
    {
        setMessage("密码不能为空", 1);
        gFocus = 1;
        return 0;
    }
    if (gConfirmLength == 0)  // 3. 确认密码不能为空
    {
        setMessage("请再输一次密码", 1);
        gFocus = 2;
        return 0;
    }
    if (strcmp(gPassword, gConfirm) != 0)  // 4. 两次输入的密码必须一样
    {
        gConfirm[0] = '\0';  // 清掉确认框，让用户对着密码框重输一遍
        gConfirmLength = 0;
        setMessage("密码不对，请重输", 1);
        gFocus = 2;
        return 0;
    }

    buildUserFromForm(&newUser);

    strength = checkPassword(newUser.password);  // 5. 弱密码直接拒绝，不写文件
    if (strength == 0)
    {
        setMessage("密码强度不足，请修改密码", 1);
        gFocus = 1;
        return 0;
    }

    // 6/7. 只读一次账号文件，同时完成查重和数量检查
    count = loadUsers(users, USER_MAX);
    if (gLoadError != 0)
    {
        setMessage("账号文件读取失败，请检查 users.txt", 1);
        return 0;
    }
    if (findUser(users, count, newUser.name) >= 0)
    {
        setMessage("该用户名已被注册，请换一个", 1);
        gFocus = 0;
        return 0;
    }
    if (count >= USER_MAX)
    {
        setMessage("账号数量已达上限，无法再注册", 1);
        return 0;
    }

    return saveCurrentUser();  // 中等及以上才真正写入文件
}

/*saveCurrentUser函数，把表单里的账号真正写入 users.txt，
成功后换成登录表单并提示“注册成功，请登录”
返回值：0 始终留在账号页（注册成功后也不自动登录）
写入和关闭由 registerUser 负责检查，失败时明确提示
*/
int saveCurrentUser(void)
{
    User newUser;  // 本次要写入文件的账号

    buildUserFromForm(&newUser);

    if (registerUser(newUser) == 1)
    {
        resetAccountPage(0);  // 复用页面重置，切回登录表单
        setMessage("注册成功，请登录", 0);
        return 0;
    }

    setMessage("注册失败：账号文件写入失败", 1);
    return 0;
}

/*resetAccountPage函数，进入账号页前把页面恢复成初始状态（接口说明见 account.h）:
清空输入框和提示，重置焦点与弱密码确认状态
*/
void resetAccountPage(int registerMode)//registerMode 为 0 显示登录页，为 1 显示注册页
{
    gRegisterMode = (registerMode != 0) ? 1 : 0;  // 记住本次是登录还是注册
    clearInputs();  // 输入框和焦点复位
    gMessage[0] = '\0';  // 清掉上一次的提示文字
    gMessageIsError = 0;
}

/*updateAccountPage函数,处理这一帧的输入和点击，完成登录或注册（接口说明见 account.h）
输出：只有登录成功时把账号写入 *user
返回值：0 留在账号页；1 登录成功；2 点了返回
说明：点击后由 main.c 决定是否切换页面
 */
int updateAccountPage(User *user)//user:接收登录账号的结构体地址
{
    Vector2 mouse;  // 本帧鼠标位置，用来判断点中了哪个输入框

    handleCharInput();  // 先处理字符输入
    handleBackspace();  // 再处理退格删除

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  // 点输入框切换焦点
    {
        mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, nameBox))
        {
            gFocus = 0;
        }
        else if (CheckCollisionPointRec(mouse, passwordBox))
        {
            gFocus = 1;
        }
        else if (gRegisterMode != 0 && CheckCollisionPointRec(mouse, confirmBox))
        {
            gFocus = 2;  // 确认密码框只在注册页存在
        }
    }

    // 回车和主按钮是同一条路：登录页去登录，注册页去注册
    if (IsKeyPressed(KEY_ENTER) || isButtonClicked(primaryButton) != 0)
    {
        if (gRegisterMode != 0)
        {
            return registerCurrentForm();
        }
        return tryLogin(user);
    }

    if (isButtonClicked(backButton) != 0)  // 返回入口菜单
    {
        return 2;
    }

    return 0;  // 本帧没有完成什么，继续留在账号页
}

/*drawTextAt函数,从指定位置开始画一行文字，统一字体大小和字间距
输入：font:中文字体；text:文字；x、y:左上角坐标；fontSize:字号；color:颜色
*/
void drawTextAt(Font font, const char *text, float x, float y, int fontSize,
                       Color color)
{
    DrawTextEx(font, text, (Vector2){x, y}, (float)fontSize, 1.0f, color);
}

/*drawTextCentered函数,把一行文字按水平中心居中绘制
输入：font：中文字体；text：文字；centerX：中心横坐标；y：上边缘纵坐标；fontSize：字号；color：颜色
*/
void drawTextCentered(Font font, const char *text, float centerX, float y,
                             int fontSize, Color color)
{
    Vector2 size = MeasureTextEx(font, text, (float)fontSize, 1.0f);  // 文字实际宽度

    DrawTextEx(font, text, (Vector2){centerX - size.x / 2.0f, y},
               (float)fontSize, 1.0f, color);
}

/*drawInputBox函数，画一个输入框：底色、边框、里面的文字，有焦点时再加一条光标
输入：font：中文字体；box：输入框矩形；text：要显示的内容（密码已经换成星号）；
placeholder：空框时的灰色提示；focused：1 表示这个框正在编辑
*/
void drawInputBox(Font font, Rectangle box, const char *text,
                         const char *placeholder, int focused)
{
    Vector2 textSize;  // 已输入文字的宽度，用来放光标
    float textX;  // 文字左边缘横坐标
    float textY;  // 文字上边缘纵坐标
    const char *visible = text;
    float available = box.width - 38.0f;  // 左右留白及光标宽度

    DrawRectangleRec(box, COLOR_BOX_FILL);  // 输入框底色
    DrawRectangleLinesEx(box, (focused != 0) ? 2.5f : 1.5f,
                         (focused != 0) ? COLOR_FOCUS : COLOR_BOX_LINE);

    textX = box.x + 16.0f;
    textY = box.y + (INPUT_H - 22.0f) / 2.0f;

    // 输入只含 ASCII；显示能放下的末尾，保留完整账号和密码用于验证。
    while (visible[0] != '\0' &&
           MeasureTextEx(font, visible, 22.0f, 1.0f).x > available)
    {
        visible++;
    }
    // 裁剪区域使用窗口像素坐标，不会自动随二维相机缩放
    {
        Camera2D camera = getUiCamera();
        Vector2 clip = GetWorldToScreen2D((Vector2){textX, box.y + 4}, camera);
        BeginScissorMode((int)clip.x, (int)clip.y,
                         (int)((box.width - 32) * camera.zoom),
                         (int)((box.height - 8) * camera.zoom));
    }

    if (text[0] == '\0')  // 空框显示灰色提示词
    {
        drawTextAt(font, placeholder, textX, textY, 22, COLOR_PLACEHOLDER);
    }
    else
    {
        drawTextAt(font, visible, textX, textY, 22, COLOR_TEXT);
    }

    if (focused != 0)  // 有焦点时在文字后面画一条竖线当光标
    {
        textSize = MeasureTextEx(font, visible, 22.0f, 1.0f);
        DrawRectangle((int)(textX + textSize.x + 2.0f), (int)(box.y + 14.0f),
                      2, 28, COLOR_FOCUS);
    }
    EndScissorMode();
}

/*drawStrengthHint函数，在注册页显示当前密码的安全等级，边输入边变化
*/
void drawStrengthHint(Font font)//font：中文字体
{
    int strength;  // 密码安全等级 0/1/2
    const char *text;  // 要显示的提示文字
    Color color;  // 提示文字颜色

    if (gPasswordLength == 0)
    {
        drawTextAt(font, "密码强度：尚未输入", INPUT_X, STRENGTH_Y, 16, COLOR_HINT);
        return;
    }

    strength = checkPassword(gPassword);
    if (strength == 0)
    {
        text = "密码强度：不安全（少于 6 位或只有一类字符）";
        color = COLOR_ERROR;
    }
    else if (strength == 1)
    {
        text = "密码强度：中等（包含两类字符）";
        color = COLOR_HINT;
    }
    else
    {
        text = "密码强度：安全（包含三类及以上字符）";
        color = COLOR_OK;
    }
    drawTextAt(font, text, INPUT_X, STRENGTH_Y, 16, color);
}

/*drawAccountPage函数，把整个账号页画出来（接口说明见 account.h）
*/
void drawAccountPage(Font font)//font：已经加载好中文字形的字体，由 main.c 传入
{
    Rectangle panel = {PANEL_X, PANEL_Y, PANEL_W, PANEL_H};  // 中间面板
    char masked[PASSWORD_MAX];  // 密码换成星号后的字符串
    char maskedConfirm[PASSWORD_MAX];  // 确认密码换成星号后的字符串

    DrawRectangleRec(panel, COLOR_PANEL);  // 面板底色
    DrawRectangleLinesEx(panel, 2.0f, COLOR_PANEL_LINE);  // 面板边框

    // 标题和副标题：登录页和注册页用不同文字
    drawTextCentered(font, (gRegisterMode != 0) ? "注册账号" : "账号登录",
                     PAGE_CENTER_X, 54.0f, 32, COLOR_TITLE);
    drawTextCentered(font, (gRegisterMode != 0) ? "设置用户名和密码，注册后请登录"
                                                : "请输入已有账号登录",
                     PAGE_CENTER_X, 98.0f, 17, COLOR_HINT);

    // 用户名输入框
    drawTextAt(font, "用户名", INPUT_X, 126.0f, 19, COLOR_LABEL);
    drawInputBox(font, nameBox, gName, "请输入用户名", gFocus == 0);

    // 密码输入框：先换成星号再显示，屏幕上不出现原密码
    memset(masked, '*', (size_t)gPasswordLength);
    masked[gPasswordLength] = '\0';
    drawTextAt(font, "密码", INPUT_X, 214.0f, 19, COLOR_LABEL);
    drawInputBox(font, passwordBox, masked, "请输入密码", gFocus == 1);

    if (gRegisterMode != 0)  // 注册页额外显示密码强度和确认密码框
    {
        drawStrengthHint(font);

        memset(maskedConfirm, '*', (size_t)gConfirmLength);
        maskedConfirm[gConfirmLength] = '\0';
        drawTextAt(font, "再次输入密码", INPUT_X, 320.0f, 19, COLOR_LABEL);
        drawInputBox(font, confirmBox, maskedConfirm, "请再输一次密码", gFocus == 2);
    }

    // 主按钮和返回按钮：登录页是“登录”，注册页是“注册”
    drawButton(primaryButton, (gRegisterMode != 0) ? "注册" : "登录", font);
    drawButton(backButton, "返回", font);

    // 底部提示文字：成功是绿色，错误是红色
    if (gMessage[0] != '\0')
    {
        drawTextCentered(font, gMessage, PAGE_CENTER_X, MESSAGE_Y, 19,
                         (gMessageIsError != 0) ? COLOR_ERROR : COLOR_OK);
    }

    drawTextCentered(font, "提示：用户名和密码只能使用英文字符、数字和符号",
                     PAGE_CENTER_X, FOOT_Y, 15, COLOR_FOOT);
}
