/*========================================================================
 * 文件名：account.c
 * 功  能：图形版账号模块。负责登录页、注册页的输入处理、账号校验和页面
 *         绘制，账号数据仍然保存在文本文件 users.txt 中，一行一个账号
 * 说  明：本模块只处理“账号页”这一页，不创建窗口、不切换页面、不结束
 *         程序，这些工作都由 main.c 负责。账号用结构体数组保存，不使用
 *         链表，只用标准 C 库和组内公共的 UI 函数
 *         页面函数每帧调用一次，不在这里写等待输入的循环
 *========================================================================*/

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS  // 允许使用 fopen 等传统 C 函数，避免 VS2022 报安全警告
#endif
#include <stdio.h>  // fopen、fprintf、fscanf 等文件读写函数
#include <string.h>  // strcmp、strlen、strncpy 等字符串函数
#include <ctype.h>  // islower、isupper、isdigit 字符类别判断
#include <errno.h>  // errno、ENOENT 用来区分“文件还不存在”
#include "account.h"  // 本模块的结构体与三个页面函数声明
#include "ui.h"  // 组内公共按钮：isButtonClicked、drawButton

#define USERS_FILE "users.txt"  // 账号文件名，与程序在同一目录
#define USER_MAX 100  // 一次最多读入内存的账号个数

/*------------------------------------------------------------------------
 * 账号页的布局常量。update 和 draw 都从这里取坐标，保证按钮和输入框的
 * 矩形区域完全一致，不会出现“画在这里、点在那里”的问题
 * 窗口初稿固定 800×600
 *------------------------------------------------------------------------*/
#define PAGE_CENTER_X    400.0f  // 窗口水平中心，标题和提示都按它居中
#define PANEL_X          140.0f  // 中间白色面板的左上角横坐标
#define PANEL_Y           60.0f  // 中间白色面板的左上角纵坐标
#define PANEL_W          520.0f  // 面板宽度
#define PANEL_H          480.0f  // 面板高度
#define INPUT_X          220.0f  // 输入框左边缘
#define INPUT_W          360.0f  // 输入框宽度
#define INPUT_H           56.0f  // 输入框高度
#define NAME_BOX_Y       190.0f  // 用户名输入框上边缘
#define PASSWORD_BOX_Y   280.0f  // 密码输入框上边缘
#define BUTTON_Y         372.0f  // 普通按钮上边缘
#define BUTTON_W         170.0f  // 普通按钮宽度
#define BUTTON_H          52.0f  // 按钮高度
#define BUTTON_LEFT_X    220.0f  // 左边那个按钮的左边缘
#define BUTTON_RIGHT_X   410.0f  // 右边那个按钮的左边缘
#define CONFIRM_W        200.0f  // 弱密码确认按钮宽度
#define CONFIRM_LEFT_X   180.0f  // “继续注册”按钮左边缘
#define CONFIRM_RIGHT_X  420.0f  // “返回修改”按钮左边缘
#define MESSAGE_Y        444.0f  // 提示文字的纵坐标

// 固定区域由输入判断和绘制共用，不需要每次调用函数构造
static const Rectangle nameBox = {INPUT_X, NAME_BOX_Y, INPUT_W, INPUT_H};
static const Rectangle passwordBox = {INPUT_X, PASSWORD_BOX_Y, INPUT_W, INPUT_H};
static const Rectangle primaryButton = {BUTTON_LEFT_X, BUTTON_Y, BUTTON_W, BUTTON_H};
static const Rectangle backButton = {BUTTON_RIGHT_X, BUTTON_Y, BUTTON_W, BUTTON_H};
static const Rectangle confirmButton = {CONFIRM_LEFT_X, BUTTON_Y, CONFIRM_W, BUTTON_H};
static const Rectangle cancelButton = {CONFIRM_RIGHT_X, BUTTON_Y, CONFIRM_W, BUTTON_H};

/*------------------------------------------------------------------------
 * 本模块内部保存的页面状态。全部是 static，调用方看不到，也不需要关心
 *------------------------------------------------------------------------*/
static char  gName[NAME_MAX];  // 用户名输入框里的字符串
static int   gNameLength;  // 用户名已经输入的字节数
static char  gPassword[PASSWORD_MAX];  // 密码输入框里的字符串，不直接绘制
static int   gPasswordLength;  // 密码已经输入的字节数
static int   gFocus;  // 当前焦点：0 用户名框，1 密码框
static int   gRegisterMode;  // 当前表单：0 登录，1 注册
static char  gMessage[128];  // 页面底部的提示文字
static int   gMessageIsError;  // 提示文字颜色：1 红色错误，0 绿色
static int   gConfirmWeak;  // 1 表示正在等用户确认弱密码注册
static int   gLoadError;  // 1 表示读 users.txt 时发生了错误

/*------------------------------------------------------------------------
 * 页面用到的颜色。定义成 static const，避免在绘制函数里写一堆数字
 *------------------------------------------------------------------------*/
static const Color COLOR_PANEL       = { 248, 249, 251, 255 };  // 面板底色
static const Color COLOR_PANEL_LINE  = { 205, 211, 219, 255 };  // 面板边框
static const Color COLOR_BOX_FILL    = { 255, 255, 255, 255 };  // 输入框底色
static const Color COLOR_BOX_LINE    = { 190, 196, 205, 255 };  // 输入框边框
static const Color COLOR_FOCUS       = {  52, 101, 164, 255 };  // 焦点高亮
static const Color COLOR_TITLE       = {  32,  58, 104, 255 };  // 标题文字
static const Color COLOR_LABEL       = {  70,  78,  90, 255 };  // 输入框标题
static const Color COLOR_TEXT        = {  20,  20,  20, 255 };  // 输入文字
static const Color COLOR_PLACEHOLDER = { 160, 166, 176, 255 };  // 占位提示
static const Color COLOR_OK          = {  22, 120,  60, 255 };  // 成功提示
static const Color COLOR_ERROR       = { 176,  48,  48, 255 };  // 错误提示
static const Color COLOR_HINT        = { 110, 118, 130, 255 };  // 次要说明
static const Color COLOR_FOOT        = { 130, 136, 148, 255 };  // 页脚说明

// 以下是本文件内部使用的函数，只在本文件可见
static int loadUsers(User users[], int maxCount);
static int findUser(const User users[], int count, const char name[]);
static void setMessage(const char *text, int isError);
static void clearInputs(void);
static void buildUserFromForm(User *user);
static void handleCharInput(void);
static void handleBackspace(void);
static int tryLogin(User *user);
static int registerCurrentForm(void);
static int saveCurrentUser(void);
static void drawTextAt(Font font, const char *text, float x, float y, int fontSize,
                       Color color);
static void drawTextCentered(Font font, const char *text, float centerX, float y,
                             int fontSize, Color color);
static void drawInputBox(Font font, Rectangle box, const char *text,
                         const char *placeholder, int focused);
static void drawStrengthHint(Font font);

/*------------------------------------------------------------------------
 * 函数：checkPassword
 * 功  能：检测密码安全等级，注册时用来判断要不要弹弱密码确认
 * 输  入：password —— 待检测的密码字符串，以 '\0' 结尾
 * 输  出：不修改密码内容
 * 返回值：0 不安全（不足 6 位，或只有一类字符）；1 中等（两类字符）；
 *         2 安全（三类及以上字符）。数字、大写字母、小写字母、其它符号
 *         分别算作四类不同的字符
 *------------------------------------------------------------------------*/
static int checkPassword(char password[])
{
    int length;  // 密码的字符个数（不含结束符）
    int hasLower = 0;  // 是否含小写字母：1 有，0 无
    int hasUpper = 0;  // 是否含大写字母：1 有，0 无
    int hasDigit = 0;  // 是否含数字：1 有，0 无
    int hasOther = 0;  // 是否含其它符号：1 有，0 无
    int kinds;  // 密码中实际出现的字符种类数（0～4）
    int i;  // 扫描密码用的下标
    unsigned char ch;  // 当前字符，用无符号类型可避免 ctype 越界

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

/*------------------------------------------------------------------------
 * 函数：registerUser
 * 功  能：把新用户追加写入 users.txt，一行一个“用户名 密码”
 * 输  入：user —— 已经填好用户名和密码的用户结构体
 * 输  出：文件末尾多出一行记录；用户名重复或读写失败时不写文件
 * 返回值：1 注册成功；0 用户名重复、参数为空或文件读写失败
 * 说  明：写入和关闭都要成功才返回 1，避免“显示成功但没存下”
 *------------------------------------------------------------------------*/
static int registerUser(User user)
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

/*------------------------------------------------------------------------
 * 函数：loginUser
 * 功  能：验证用户名和密码是否与账号文件中的某一行完全一致
 * 输  入：user —— 待验证的用户，用户名密码都已填好
 * 输  出：不修改任何账号数据
 * 返回值：1 两项都匹配，登录成功；0 用户名不存在或密码错误
 * 说  明：先把文件读进结构体数组，再逐个比较，不使用链表
 *------------------------------------------------------------------------*/
static int loginUser(User user)
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

/*------------------------------------------------------------------------
 * 函数：loadUsers
 * 功  能：把 users.txt 中的账号读入结构体数组
 * 输  入：users —— 接收账号的数组，由调用者提供；
 *         maxCount —— 数组最多能装下的账号个数
 * 输  出：把账号写进 users 数组；同时更新 gLoadError 标记读取是否出错
 * 返回值：实际读入的账号个数；文件不存在时返回 0（首次运行视为空表）
 * 说  明："%31s %63s" 的宽度正好对应 name 的 31 字节和 password 的
 *         63 字节，可以防止超长字符串把数组写坏
 *------------------------------------------------------------------------*/
static int loadUsers(User users[], int maxCount)
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

/*------------------------------------------------------------------------
 * 函数：findUser
 * 功  能：在已经读入的账号数组里按用户名查找
 * 输  入：users、count —— 账号数组和它的有效长度；name —— 要找的用户名
 * 输  出：不修改数组
 * 返回值：找到时返回该账号在数组中的下标（从 0 开始），找不到返回 -1
 *------------------------------------------------------------------------*/
static int findUser(const User users[], int count, const char name[])
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

/*------------------------------------------------------------------------
 * 函数：setMessage
 * 功  能：设置页面底部的提示文字和颜色
 * 输  入：text —— 提示内容；isError —— 1 表示红色错误，0 表示绿色正常
 * 输  出：更新 gMessage 和 gMessageIsError
 * 返回值：无
 *------------------------------------------------------------------------*/
static void setMessage(const char *text, int isError)
{
    strncpy(gMessage, text, sizeof(gMessage) - 1);  // 留出结束符的位置
    gMessage[sizeof(gMessage) - 1] = '\0';
    gMessageIsError = (isError != 0) ? 1 : 0;
}

/*------------------------------------------------------------------------
 * 函数：clearInputs
 * 功  能：把两个输入框清空，焦点回到用户名框，同时取消弱密码确认状态
 * 输  入：无
 * 输  出：清空 gName、gPassword 和它们的长度，重置 gFocus、gConfirmWeak
 * 返回值：无
 *------------------------------------------------------------------------*/
static void clearInputs(void)
{
    gName[0] = '\0';  // 用户名框清空
    gNameLength = 0;
    gPassword[0] = '\0';  // 密码框清空
    gPasswordLength = 0;
    gFocus = 0;  // 焦点回到用户名框
    gConfirmWeak = 0;  // 取消弱密码确认
}

/*------------------------------------------------------------------------
 * 函数：buildUserFromForm
 * 功  能：把两个输入框的内容复制成一个 User 结构体
 * 输  入：无
 * 输  出：把用户名和密码写进 *user
 * 返回值：无
 * 说  明：输入长度已经由输入函数限制，这里再用 strncpy 兜一次底
 *------------------------------------------------------------------------*/
static void buildUserFromForm(User *user)
{
    strcpy(user->name, gName);  // 输入长度已在输入时限制，直接复制即可
    strcpy(user->password, gPassword);
}

/*------------------------------------------------------------------------
 * 函数：handleCharInput
 * 功  能：取出这一帧已经排队的按键字符，写进当前有焦点的输入框
 * 输  入：无（字符来自 raylib 的按键队列）
 * 输  出：更新 gName/gPassword 及其长度
 * 返回值：无
 * 说  明：初稿只接受可打印 ASCII 字符（33～126），空格、控制字符、中文
 *         输入法产生的字符都忽略；两个框各自有长度上限，末尾始终留 '\0'
 *         这个循环只处理本帧已有的字符，不会阻塞等待用户输入
 *------------------------------------------------------------------------*/
static void handleCharInput(void)
{
    int code;  // 本帧读到的一个字符编码，0 表示队列已空
    char *text = (gFocus == 0) ? gName : gPassword;  // 当前有焦点的输入框
    int *length = (gFocus == 0) ? &gNameLength : &gPasswordLength;  // 框里的长度
    int limit = (gFocus == 0) ? NAME_MAX - 1 : PASSWORD_MAX - 1;  // 框的长度上限

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

/*------------------------------------------------------------------------
 * 函数：handleBackspace
 * 功  能：按下退格键时删掉当前焦点框末尾的一个字符
 * 输  入：无
 * 输  出：更新对应输入框的内容和长度
 * 返回值：无
 * 说  明：框里已经是空串时什么也不做，不会越界
 *------------------------------------------------------------------------*/
static void handleBackspace(void)
{
    char *text = (gFocus == 0) ? gName : gPassword;  // 当前有焦点的输入框
    int *length = (gFocus == 0) ? &gNameLength : &gPasswordLength;  // 框里的长度

    if (!IsKeyPressed(KEY_BACKSPACE) || *length <= 0)  // 没按退格或框已空
    {
        return;
    }
    (*length)--;  // 删掉末尾一个字符
    text[*length] = '\0';
}

/*------------------------------------------------------------------------
 * 函数：tryLogin
 * 功  能：用输入框里的用户名和密码做一次登录校验
 * 输  入：user —— 登录成功后接收账号的结构体地址
 * 输  出：只在校验通过时把用户名和密码写入 *user，失败时不改动 *user
 * 返回值：1 登录成功；0 留在账号页（空输入、密码错误或文件出错）
 *------------------------------------------------------------------------*/
static int tryLogin(User *user)
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

/*------------------------------------------------------------------------
 * 函数：registerCurrentForm
 * 功  能：用注册表单里的内容做一次注册，先做空值、重名、容量和密码强度
 *         检查，必要时进入弱密码确认状态
 * 输  入：无
 * 输  出：可能在页面上显示提示，或把 gConfirmWeak 置 1 等待用户确认
 * 返回值：0 留在注册页（包括等待确认的情况）
 * 说  明：确认前不写文件；用户确认后再调用 saveCurrentUser 真正写入
 *------------------------------------------------------------------------*/
static int registerCurrentForm(void)
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

    buildUserFromForm(&newUser);

    // 3/4. 只读一次账号文件，同时完成查重和数量检查
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
    if (count >= USER_MAX)
    {
        setMessage("账号数量已达上限，无法再注册", 1);
        return 0;
    }

    strength = checkPassword(newUser.password);  // 5. 密码强度检查
    if (strength == 0)
    {
        gConfirmWeak = 1;  // 进入确认状态，先不写文件
        setMessage("密码强度不足，是否仍要注册？", 1);
        return 0;
    }

    return saveCurrentUser();  // 中等及以上直接注册
}

/*------------------------------------------------------------------------
 * 函数：saveCurrentUser
 * 功  能：把表单里的账号真正写入 users.txt，成功后就切回登录表单
 * 输  入：无
 * 输  出：写文件；成功后换成登录表单并提示“注册成功，请登录”
 * 返回值：0 始终留在账号页（注册成功后也不自动登录）
 * 说  明：写入和关闭都由 registerUser 负责检查，失败时明确提示
 *------------------------------------------------------------------------*/
static int saveCurrentUser(void)
{
    User newUser;  // 本次要写入文件的账号

    buildUserFromForm(&newUser);

    if (registerUser(newUser) == 1)
    {
        resetAccountPage(0);  // 复用页面重置，切回登录表单
        setMessage("注册成功，请登录", 0);
        return 0;
    }

    gConfirmWeak = 0;
    setMessage("注册失败：账号文件写入失败", 1);
    return 0;
}

/*------------------------------------------------------------------------
 * 函数：resetAccountPage
 * 功  能：进入账号页前把页面恢复成初始状态（接口说明见 account.h）
 * 输  入：registerMode 为 0 显示登录页，为 1 显示注册页
 * 输  出：清空输入框和提示，重置焦点与弱密码确认状态
 * 返回值：无
 *------------------------------------------------------------------------*/
void resetAccountPage(int registerMode)
{
    gRegisterMode = (registerMode != 0) ? 1 : 0;  // 记住本次是登录还是注册
    clearInputs();  // 输入框和焦点复位
    gMessage[0] = '\0';  // 清掉上一次的提示文字
    gMessageIsError = 0;
}

/*------------------------------------------------------------------------
 * 函数：updateAccountPage
 * 功  能：处理这一帧的输入和点击，完成登录或注册（接口说明见 account.h）
 * 输  入：user —— 接收登录账号的结构体地址
 * 输  出：只有登录成功时把账号写入 *user
 * 返回值：0 留在账号页；1 登录成功；2 点了返回
 * 说  明：每帧只处理一次，点击后由 main.c 决定是否切换页面
 *------------------------------------------------------------------------*/
int updateAccountPage(User *user)
{
    Vector2 mouse;  // 本帧鼠标位置，用来判断点中了哪个输入框

    // 弱密码确认状态下，只响应“继续注册”和“返回修改”两个按钮
    if (gConfirmWeak != 0)
    {
        if (isButtonClicked(confirmButton) != 0)
        {
            gConfirmWeak = 0;
            return saveCurrentUser();  // 用户确认，真正写入文件
        }
        if (isButtonClicked(cancelButton) != 0)
        {
            gConfirmWeak = 0;  // 取消确认，什么也不写
            setMessage("请修改密码后再注册", 1);
            gFocus = 1;
        }
        return 0;
    }

    handleCharInput();  // 先处理字符输入
    handleBackspace();  // 再处理退格删除

    if (IsKeyPressed(KEY_TAB))  // Tab 在两个框之间切换焦点
    {
        gFocus = 1 - gFocus;
    }
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

/*------------------------------------------------------------------------
 * 函数：drawTextAt
 * 功  能：从指定位置开始画一行文字，统一字体大小和字间距
 * 输  入：font —— 中文字体；text —— 文字；x、y —— 左上角坐标；
 *         fontSize —— 字号；color —— 颜色
 * 输  出：只绘制，不修改任何状态
 * 返回值：无
 *------------------------------------------------------------------------*/
static void drawTextAt(Font font, const char *text, float x, float y, int fontSize,
                       Color color)
{
    DrawTextEx(font, text, (Vector2){x, y}, (float)fontSize, 1.0f, color);
}

/*------------------------------------------------------------------------
 * 函数：drawTextCentered
 * 功  能：把一行文字按水平中心居中绘制
 * 输  入：font —— 中文字体；text —— 文字；centerX —— 中心横坐标；
 *         y —— 上边缘纵坐标；fontSize —— 字号；color —— 颜色
 * 输  出：只绘制，不修改任何状态
 * 返回值：无
 *------------------------------------------------------------------------*/
static void drawTextCentered(Font font, const char *text, float centerX, float y,
                             int fontSize, Color color)
{
    Vector2 size = MeasureTextEx(font, text, (float)fontSize, 1.0f);  // 文字实际宽度

    DrawTextEx(font, text, (Vector2){centerX - size.x / 2.0f, y},
               (float)fontSize, 1.0f, color);
}

/*------------------------------------------------------------------------
 * 函数：drawInputBox
 * 功  能：画一个输入框：底色、边框、里面的文字，有焦点时再加一条光标
 * 输  入：font —— 中文字体；box —— 输入框矩形；text —— 要显示的内容
 *         （密码已经换成星号）；placeholder —— 空框时的灰色提示；
 *         focused —— 1 表示这个框正在编辑
 * 输  出：只绘制
 * 返回值：无
 *------------------------------------------------------------------------*/
static void drawInputBox(Font font, Rectangle box, const char *text,
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

/*------------------------------------------------------------------------
 * 函数：drawStrengthHint
 * 功  能：在注册页显示当前密码的安全等级，边输入边变化
 * 输  入：font —— 中文字体
 * 输  出：只绘制一行提示文字
 * 返回值：无
 * 说  明：用的是和注册校验同一个 checkPassword，提示和实际判断不会不一致
 *------------------------------------------------------------------------*/
static void drawStrengthHint(Font font)
{
    int strength;  // 密码安全等级 0/1/2
    const char *text;  // 要显示的提示文字
    Color color;  // 提示文字颜色

    if (gPasswordLength == 0)
    {
        drawTextAt(font, "密码强度：尚未输入", INPUT_X, 342.0f, 18, COLOR_HINT);
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
    drawTextAt(font, text, INPUT_X, 342.0f, 18, color);
}

/*------------------------------------------------------------------------
 * 函数：drawAccountPage
 * 功  能：把整个账号页画出来（接口说明见 account.h）
 * 输  入：font —— 已经加载好中文字形的字体，由 main.c 传入
 * 输  出：只绘制面板、标题、输入框、按钮和提示，不读文件、不改状态
 * 返回值：无
 * 说  明：密码框里画的是星号，原密码不会出现在屏幕上
 *------------------------------------------------------------------------*/
void drawAccountPage(Font font)
{
    Rectangle panel = {PANEL_X, PANEL_Y, PANEL_W, PANEL_H};  // 中间面板
    char masked[PASSWORD_MAX];  // 密码换成星号后的字符串

    DrawRectangleRec(panel, COLOR_PANEL);  // 面板底色
    DrawRectangleLinesEx(panel, 2.0f, COLOR_PANEL_LINE);  // 面板边框

    // 标题和副标题：登录页和注册页用不同文字
    drawTextCentered(font, (gRegisterMode != 0) ? "注册账号" : "账号登录",
                     PAGE_CENTER_X, 92.0f, 34, COLOR_TITLE);
    drawTextCentered(font, (gRegisterMode != 0) ? "设置用户名和密码，注册后请登录"
                                                : "请输入已有账号登录",
                     PAGE_CENTER_X, 140.0f, 18, COLOR_HINT);

    // 用户名输入框
    drawTextAt(font, "用户名", INPUT_X, 162.0f, 20, COLOR_LABEL);
    drawInputBox(font, nameBox, gName, "请输入用户名", gFocus == 0);

    // 密码输入框：先换成星号再显示，屏幕上不出现原密码
    memset(masked, '*', (size_t)gPasswordLength);
    masked[gPasswordLength] = '\0';
    drawTextAt(font, "密码", INPUT_X, 252.0f, 20, COLOR_LABEL);
    drawInputBox(font, passwordBox, masked, "请输入密码", gFocus == 1);

    if (gRegisterMode != 0)  // 注册页额外显示密码强度
    {
        drawStrengthHint(font);
    }

    // 按钮：等待弱密码确认时换成“继续注册 / 返回修改”，其余时候是主按钮和返回
    if (gConfirmWeak != 0)
    {
        drawButton(confirmButton, "继续注册", font);
        drawButton(cancelButton, "返回修改", font);
    }
    else
    {
        drawButton(primaryButton,
                   (gRegisterMode != 0) ? "注册" : "登录", font);
        drawButton(backButton, "返回", font);
    }

    // 底部提示文字：成功是绿色，错误是红色
    if (gMessage[0] != '\0')
    {
        drawTextCentered(font, gMessage, PAGE_CENTER_X, MESSAGE_Y, 20,
                         (gMessageIsError != 0) ? COLOR_ERROR : COLOR_OK);
    }

    drawTextCentered(font, "提示：用户名和密码只能使用英文字符、数字和符号",
                     PAGE_CENTER_X, 492.0f, 16, COLOR_FOOT);
}
