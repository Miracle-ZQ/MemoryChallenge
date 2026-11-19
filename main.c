#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "ui.h"
#include "rank.h"
#include "account.h"
#include "game.h"

// 界面文字配色：背景图偏暗，所以文字统一用浅色
#define COLOR_TITLE (Color){ 240, 246, 255, 255 }
#define COLOR_HINT  (Color){ 190, 202, 220, 255 }
#define COLOR_OK    (Color){ 120, 220, 160, 255 }
#define COLOR_WARN  (Color){ 250, 200, 120, 255 }
#define COLOR_ERROR (Color){ 255, 130, 130, 255 }

// 页面编号只由主函数管理，其他模块通过返回值通知主函数
typedef enum {
    PAGE_ENTRY,
    PAGE_ACCOUNT,
    PAGE_MENU,
    PAGE_LEVEL,
    PAGE_GAME,
    PAGE_RESULT,
    PAGE_RANK
} Page;

//按钮位置
static const Rectangle loginButton = {180, 180, 200, 48};  // 登录按钮的区域，依次为 x、y、宽、高
static const Rectangle registerButton = {420, 180, 200, 48};  // 注册
static const Rectangle switchButton = {180, 180, 440, 48};  // 切换账号
static const Rectangle gameButton = {180, 260, 440, 48};  // 开始游戏
static const Rectangle rankButton = {180, 340, 440, 48};  // 排行榜
static const Rectangle exitButton = {300, 460, 200, 48};  // 退出程序
static const Rectangle backButton = {300, 460, 200, 48};  // 返回菜单
static const Rectangle easyButton = {250, 180, 300, 48};  // 初级
static const Rectangle mediumButton = {250, 260, 300, 48};  // 中级
static const Rectangle hardButton = {250, 340, 300, 48};  // 高级

// 在窗口中居中绘制一行文字
static void drawText(Font font, const char text[], float y, float size, Color color);
// 绘制主菜单
static void drawMenu(Font font, int loggedIn);
// 绘制难度选择页
static void drawLevels(Font font);

/*
 * 功能：创建图形窗口，处理菜单、难度选择和排行榜页面
 * 输入：无参数，通过鼠标选择页面
 * 输出：登录后的游戏、结算和排行榜，通关后保存一次成绩
 * 返回值：0 正常退出，1 窗口或字体初始化失败
 */
int main(void)
{
    Font font = {0};  // 界面共用的字体，加载后交给各页面绘制文字
    // 直接在高分辨率窗口绘制，保留各模块原来的布局坐标
    Camera2D camera = {0};  // 界面缩放和居中位置，鼠标坐标也按它换算
    Page page = PAGE_ENTRY;  // 当前显示的页面，启动时先显示登录入口
    User user = {0};  // 当前登录账号，未登录时内容为空
    int accountResult;  // 账号页返回的结果：0 继续，1 登录成功，2 返回
    int running = 1;  // 程序是否继续运行：1 继续，0 退出
    int level = 0;  // 本局难度：1 初级，2 中级，3 高级，0 尚未选择
    int seconds = 0;  // 游戏模块返回的通关用时，单位为秒
    int gameResult;  // 游戏更新结果：0 继续，1 通关，2 退出本局
    int saveResult = 0;  // 排行榜保存结果：1 成功，0 未更新，-1 出错

    srand((unsigned int)time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);  // 允许调整窗口大小
    InitWindow(800, 600, "记忆力大考验");  // 创建初始大小为 800×600 的窗口
    SetWindowMinSize(800, 600);  // 限制窗口最小为 800×600
    SetTargetFPS(60);  // 设置目标帧率为每秒 60 帧
    SetExitKey(KEY_NULL);  // 禁用默认按 Esc 退出程序的行为

    loadUiResources(&font);//加载字体
    loadGameResources();//加载卡牌
    loadBackgrounds();//加载页面背景图

    while (running && !WindowShouldClose()) {
        camera = getUiCamera();
        // 先扣除居中留白，再将鼠标坐标换回原来的 800x600 布局
        SetMouseOffset(-(int)camera.offset.x, -(int)camera.offset.y);
        SetMouseScale(1.0f / camera.zoom, 1.0f / camera.zoom);
        // 每帧只更新一个页面，切换后不会重复处理同一次点击
        switch (page) {
            case PAGE_ENTRY:
                if (isButtonClicked(loginButton)) {//登录
                    resetAccountPage(0);
                    page = PAGE_ACCOUNT;
                } else if (isButtonClicked(registerButton)) {//注册
                    resetAccountPage(1);
                    page = PAGE_ACCOUNT;
                } else if (isButtonClicked(exitButton)) {//退出
                    running = 0;
                }
                break;

            case PAGE_ACCOUNT:
                accountResult = updateAccountPage(&user);
                if (accountResult == 1) {//登陆成功
                    page = PAGE_MENU;
                } else if (accountResult == 2) {//返回
                    page = PAGE_ENTRY;
                }
                break;

            case PAGE_MENU:
                if (isButtonClicked(switchButton)) {//切换账号
                    user = (User){0};
                    page = PAGE_ENTRY;
                } else if (isButtonClicked(gameButton)) {//游戏
                    page = PAGE_LEVEL;
                } else if (isButtonClicked(rankButton)) {//排行榜
                    setCurrentPlayer(user.name);
                    resetRankPage();
                    page = PAGE_RANK;
                } else if (isButtonClicked(exitButton)) {//退出
                    running = 0;
                }
                break;

            case PAGE_LEVEL:
                if (isButtonClicked(easyButton)) {//初级
                    level = 1;
                }
                else if (isButtonClicked(mediumButton)) {//中级
                    level = 2;
                }
                else if (isButtonClicked(hardButton)) {//高级
                    level = 3;
                }
                else if (isButtonClicked(backButton)) {//返回
                    page = PAGE_MENU;
                    break;
                }
                else {
                    break;  // 没有点击任何按钮
                }

                startGame(level);
                page = PAGE_GAME;
                break;

            case PAGE_GAME:
                gameResult = updateGame(&seconds);
                if (gameResult == 1) {//游戏通关
                    saveResult = 0;
                    if (!getGameCheats()) {// 作弊局只显示结算，不向排行榜提交成绩
                        saveResult = saveScore(user.name, level, seconds);
                    }
                    page = PAGE_RESULT;
                } else if (gameResult == 2) {//游戏中途点击返回按钮
                    page = PAGE_MENU;
                }
                break;

            case PAGE_RESULT:
                if (isButtonClicked(backButton)) {//返回菜单
                    page = PAGE_MENU;
                } else if (isButtonClicked(rankButton)) {//查看排行榜
                    setCurrentPlayer(user.name);
                    resetRankPage();
                    page = PAGE_RANK;
                }
                break;

            case PAGE_RANK:
                if (updateRankPage()) {
                    page = PAGE_MENU;
                }
                break;
        }

        BeginDrawing();
        ClearBackground((Color){245, 248, 252, 255});
        // 背景和遮罩按真实窗口尺寸画（在相机之外），全屏或宽屏都能铺满
        if (page == PAGE_GAME)      drawBackground(1);
        else if (page == PAGE_RANK) drawBackground(2);
        else                        drawBackground(0);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      (Color){ 8, 12, 20, 110 });

        BeginMode2D(camera);
        switch (page) {
            case PAGE_ENTRY:
                drawMenu(font, 0);
                break;
            case PAGE_ACCOUNT:
                drawAccountPage(font);
                break;
            case PAGE_MENU:
                drawMenu(font, 1);
                drawText(font, TextFormat("当前用户：%s", user.name), 125, 20, COLOR_HINT);
                break;
            case PAGE_LEVEL:
                drawLevels(font);
                break;
            case PAGE_GAME:
                drawGame(font);
                break;
            case PAGE_RESULT:
                drawText(font, "恭喜通关", 90, 36, COLOR_TITLE);
                drawText(font, TextFormat("用时：%d 秒", seconds), 180, 28, COLOR_TITLE);
                drawText(font, TextFormat("配对错误：%d 次   作弊：%d 次", getGameMistakes(), getGameCheats()), 215, 20, COLOR_HINT);
                if (getGameCheats() != 0) {
                    drawText(font, "本局使用了亮牌，不计入排行榜", 250, 22, COLOR_WARN);
                } else {
                    if (saveResult == 1) {
                        drawText(font, "成绩已保存", 250, 24, COLOR_OK);
                    } else if (saveResult == 0) {
                        drawText(font, "未刷新成绩", 250, 24, COLOR_HINT);
                    } else {
                        drawText(font, "成绩保存失败", 250, 24, COLOR_ERROR);
                    }
                }
                drawButton(rankButton, "排行榜", font);
                drawButton(backButton, "返回", font);
                break;
            case PAGE_RANK:
                drawRankPage(font);
                break;
        }
        EndMode2D();
        EndDrawing();
    }

    unloadBackgrounds();
    unloadUiResources(font);
    CloseWindow();
    return 0;
}

/*
 * 功能：居中绘制一行文字
 * 输入：font 为字体，text 为文字，y 为纵坐标，size 为字号
 * 输出：屏幕中的文字，不修改页面状态
 * 返回值：无
 */
static void drawText(Font font, const char text[], float y, float size, Color color)
{
    Vector2 width = MeasureTextEx(font, text, size, 1);  // 文字绘制后的宽和高，用来计算水平居中位置
    DrawTextEx(font, text, (Vector2){(800 - width.x) / 2, y}, size, 1, color);
}

/*
 * 功能：绘制三行主菜单和退出按钮
 * 输入：font 为中文字体，loggedIn 为 0 显示登录入口，为 1 显示玩家菜单
 * 输出：登录入口或当前玩家的主菜单
 * 返回值：无
 */
static void drawMenu(Font font, int loggedIn)
{
    drawText(font, "记忆力大考验", 70, 40, COLOR_TITLE);
    if (loggedIn) {
        drawButton(switchButton, "切换账号", font);
        drawButton(gameButton, "开始游戏", font);
        drawButton(rankButton, "排行榜", font);
    } else {
        drawButton(loginButton, "登录", font);
        drawButton(registerButton, "注册", font);
        drawText(font, "请先登录", 285, 24, COLOR_HINT);
    }
    drawButton(exitButton, "退出", font);
}

/*
 * 功能：绘制三个难度和返回按钮
 * 输入：font 为中文字体
 * 输出：难度选择页面，不开始游戏或保存成绩
 * 返回值：无
 */
static void drawLevels(Font font)
{
    drawText(font, "选择难度", 80, 36, COLOR_TITLE);
    drawButton(easyButton, "初级 4x4", font);
    drawButton(mediumButton, "中级 4x5", font);
    drawButton(hardButton, "高级 6x6", font);
    drawButton(backButton, "返回", font);
}
