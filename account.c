#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "account.h"
#include "ui.h"
#define USERS_FILE "users.txt"
#define USER_MAX 100
#define PAGE_CENTER_X    400.0f
#define PANEL_X          140.0f
#define PANEL_Y           26.0f
#define PANEL_W          520.0f
#define PANEL_H          548.0f
#define INPUT_X          220.0f
#define INPUT_W          360.0f
#define INPUT_H           54.0f
#define NAME_BOX_Y       150.0f
#define PASSWORD_BOX_Y   238.0f
#define CONFIRM_BOX_Y    344.0f
#define STRENGTH_Y       296.0f
#define BUTTON_Y         414.0f
#define BUTTON_W         170.0f
#define BUTTON_H          52.0f
#define BUTTON_LEFT_X    220.0f
#define BUTTON_RIGHT_X   410.0f
#define MESSAGE_Y        482.0f
#define FOOT_Y           520.0f
const Rectangle nameBox = {INPUT_X, NAME_BOX_Y, INPUT_W, INPUT_H};
const Rectangle passwordBox = {INPUT_X, PASSWORD_BOX_Y, INPUT_W, INPUT_H};
const Rectangle confirmBox = {INPUT_X, CONFIRM_BOX_Y, INPUT_W, INPUT_H};
const Rectangle primaryButton = {BUTTON_LEFT_X, BUTTON_Y, BUTTON_W, BUTTON_H};
const Rectangle backButton = {BUTTON_RIGHT_X, BUTTON_Y, BUTTON_W, BUTTON_H};
char  gName[NAME_MAX];
int   gNameLength;
char  gPassword[PASSWORD_MAX];
int   gPasswordLength;
char  gConfirm[PASSWORD_MAX];
int   gConfirmLength;
int   gFocus;
int   gRegisterMode;
char  gMessage[128];
int   gMessageIsError;
const Color COLOR_PANEL       = { 248, 249, 251, 255 };
const Color COLOR_PANEL_LINE  = { 205, 211, 219, 255 };
const Color COLOR_BOX_FILL    = { 255, 255, 255, 255 };
const Color COLOR_BOX_LINE    = { 190, 196, 205, 255 };
const Color COLOR_FOCUS       = {  52, 101, 164, 255 };
const Color COLOR_TITLE       = {  32,  58, 104, 255 };
const Color COLOR_LABEL       = {  70,  78,  90, 255 };
const Color COLOR_TEXT        = {  20,  20,  20, 255 };
const Color COLOR_PLACEHOLDER = { 160, 166, 176, 255 };
const Color COLOR_OK          = {  22, 120,  60, 255 };
const Color COLOR_ERROR       = { 176,  48,  48, 255 };
const Color COLOR_HINT        = { 110, 118, 130, 255 };
const Color COLOR_FOOT        = { 130, 136, 148, 255 };
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
void drawInputBox(Font font, const char *label, Rectangle box, const char *text,
                         const char *placeholder, int focused);
void drawStrengthHint(Font font);
int checkPassword(char password[])
{
    int length;
    int hasLower = 0;
    int hasUpper = 0;
    int hasDigit = 0;
    int hasOther = 0;
    int kinds;
    int i;
    unsigned char ch;
    length = (int)strlen(password);
    if (length < 6)
    {
        return 0;
    }
    for (i = 0; i < length; i++)
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
            hasOther = 1;
        }
    }
    kinds = hasLower + hasUpper + hasDigit + hasOther;
    if (kinds <= 1)
    {
        return 0;
    }
    if (kinds == 2)
    {
        return 1;
    }
    return 2;
}
void registerUser(User user)
{
    FILE *fp;
    fp = fopen(USERS_FILE, "a");
    fprintf(fp, "%s %s\n", user.name, user.password);
    fclose(fp);
}
int loginUser(User user)
{
    User users[USER_MAX];
    int count;
    int i;
    count = loadUsers(users, USER_MAX);
    for (i = 0; i < count; i++)
    {
        if (strcmp(users[i].name, user.name) == 0 &&
            strcmp(users[i].password, user.password) == 0)
        {
            return 1;
        }
    }
    return 0;
}
int loadUsers(User users[], int maxCount)
{
    FILE *fp;
    int count = 0;
    fp = fopen(USERS_FILE, "r");
    if (fp == NULL)
    {
        return 0;
    }
    while (count < maxCount &&
           fscanf(fp, "%31s %63s", users[count].name, users[count].password) == 2)
    {
        count++;
    }
    fclose(fp);
    return count;
}
int findUser(const User users[], int count, const char name[])
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (strcmp(users[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}
void setMessage(const char *text, int isError)
{
    strncpy(gMessage, text, sizeof(gMessage) - 1);
    gMessage[sizeof(gMessage) - 1] = '\0';
    gMessageIsError = (isError != 0) ? 1 : 0;
}
void clearInputs(void)
{
    gName[0] = '\0';
    gNameLength = 0;
    gPassword[0] = '\0';
    gPasswordLength = 0;
    gConfirm[0] = '\0';
    gConfirmLength = 0;
    gFocus = 0;
}
void buildUserFromForm(User *user)
{
    strcpy(user->name, gName);
    strcpy(user->password, gPassword);
}
void handleCharInput(void)
{
    int code;
    char *text;
    int *length;
    int limit;
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
        if (code < 33 || code > 126 || *length >= limit)
        {
            continue;
        }
        text[*length] = (char)code;
        (*length)++;
        text[*length] = '\0';
    }
}
void handleBackspace(void)
{
    char *text;
    int *length;
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
    if (!IsKeyPressed(KEY_BACKSPACE) || *length <= 0)
    {
        return;
    }
    (*length)--;
    text[*length] = '\0';
}
int tryLogin(User *user)
{
    User input;
    if (gNameLength == 0)
    {
        setMessage("请输入用户名", 1);
        gFocus = 0;
        return 0;
    }
    if (gPasswordLength == 0)
    {
        setMessage("请输入密码", 1);
        gFocus = 1;
        return 0;
    }
    buildUserFromForm(&input);
    if (loginUser(input) == 1)
    {
        buildUserFromForm(user);
        setMessage("登录成功", 0);
        return 1;
    }
    setMessage("用户名或密码错误", 1);
    gFocus = 1;
    return 0;
}
int registerCurrentForm(void)
{
    User users[USER_MAX];
    User newUser;
    int count;
    int strength;
    if (gNameLength == 0)
    {
        setMessage("用户名不能为空", 1);
        gFocus = 0;
        return 0;
    }
    if (gPasswordLength == 0)
    {
        setMessage("密码不能为空", 1);
        gFocus = 1;
        return 0;
    }
    if (gConfirmLength == 0)
    {
        setMessage("请再输一次密码", 1);
        gFocus = 2;
        return 0;
    }
    if (strcmp(gPassword, gConfirm) != 0)
    {
        gConfirm[0] = '\0';
        gConfirmLength = 0;
        setMessage("密码不对，请重输", 1);
        gFocus = 2;
        return 0;
    }
    buildUserFromForm(&newUser);
    strength = checkPassword(newUser.password);
    if (strength == 0)
    {
        setMessage("密码强度不足，请修改密码", 1);
        gFocus = 1;
        return 0;
    }
    count = loadUsers(users, USER_MAX);
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
    return saveCurrentUser();
}
int saveCurrentUser(void)
{
    User newUser;
    buildUserFromForm(&newUser);
    registerUser(newUser);
    resetAccountPage(0);
    setMessage("注册成功，请登录", 0);
    return 0;
}
void resetAccountPage(int registerMode)
{
    gRegisterMode = (registerMode != 0) ? 1 : 0;
    clearInputs();
    gMessage[0] = '\0';
    gMessageIsError = 0;
}
int updateAccountPage(User *user)
{
    Vector2 mouse;
    handleCharInput();
    handleBackspace();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
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
            gFocus = 2;
        }
    }
    if (IsKeyPressed(KEY_ENTER) || isButtonClicked(primaryButton) != 0)
    {
        if (gRegisterMode != 0)
        {
            return registerCurrentForm();
        }
        return tryLogin(user);
    }
    if (isButtonClicked(backButton) != 0)
    {
        return 2;
    }
    return 0;
}
void drawTextAt(Font font, const char *text, float x, float y, int fontSize,
                       Color color)
{
    DrawTextEx(font, text, (Vector2){x, y}, (float)fontSize, 1.0f, color);
}
void drawTextCentered(Font font, const char *text, float centerX, float y,
                             int fontSize, Color color)
{
    Vector2 size = MeasureTextEx(font, text, (float)fontSize, 1.0f);
    DrawTextEx(font, text, (Vector2){centerX - size.x / 2.0f, y},
               (float)fontSize, 1.0f, color);
}
void drawInputBox(Font font, const char *label, Rectangle box, const char *text,
                         const char *placeholder, int focused)
{
    float textX = box.x + 16.0f;
    float textY = box.y + 16.0f;
    drawTextAt(font, label, box.x, box.y - 24.0f, 19, COLOR_LABEL);
    DrawRectangleRec(box, COLOR_BOX_FILL);
    DrawRectangleLinesEx(box, 2.0f, (focused != 0) ? COLOR_FOCUS : COLOR_BOX_LINE);
    if (text[0] == '\0')
    {
        drawTextAt(font, placeholder, textX, textY, 22, COLOR_PLACEHOLDER);
    }
    else
    {
        drawTextAt(font, text, textX, textY, 22, COLOR_TEXT);
    }
    if (focused != 0)
    {
        float width = MeasureTextEx(font, text, 22.0f, 1.0f).x;
        DrawRectangle((int)(textX + width + 2.0f), (int)(box.y + 14.0f),
                      2, 28, COLOR_FOCUS);
    }
}
void drawStrengthHint(Font font)
{
    int strength;
    const char *text;
    Color color;
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
void drawAccountPage(Font font)
{
    Rectangle panel = {PANEL_X, PANEL_Y, PANEL_W, PANEL_H};
    char masked[PASSWORD_MAX];
    char maskedConfirm[PASSWORD_MAX];
    DrawRectangleRec(panel, COLOR_PANEL);
    DrawRectangleLinesEx(panel, 2.0f, COLOR_PANEL_LINE);
    drawTextCentered(font, (gRegisterMode != 0) ? "注册账号" : "账号登录",
                     PAGE_CENTER_X, 54.0f, 32, COLOR_TITLE);
    drawInputBox(font, "用户名", nameBox, gName, "请输入用户名", gFocus == 0);
    memset(masked, '*', (size_t)gPasswordLength);
    masked[gPasswordLength] = '\0';
    drawInputBox(font, "密码", passwordBox, masked, "请输入密码", gFocus == 1);
    if (gRegisterMode != 0)
    {
        drawStrengthHint(font);
        memset(maskedConfirm, '*', (size_t)gConfirmLength);
        maskedConfirm[gConfirmLength] = '\0';
        drawInputBox(font, "再次输入密码", confirmBox, maskedConfirm,
                     "请再输一次密码", gFocus == 2);
    }
    drawButton(primaryButton, (gRegisterMode != 0) ? "注册" : "登录", font);
    drawButton(backButton, "返回", font);
    if (gMessage[0] != '\0')
    {
        drawTextCentered(font, gMessage, PAGE_CENTER_X, MESSAGE_Y, 19,
                         (gMessageIsError != 0) ? COLOR_ERROR : COLOR_OK);
    }
    drawTextCentered(font, "提示：用户名和密码只能使用英文字符、数字和符号",
                     PAGE_CENTER_X, FOOT_Y, 15, COLOR_FOOT);
}
