#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
    PAGE_CUSTOM_CONFIG,
    PAGE_GAME,
    PAGE_RESULT,
    PAGE_RANK,
    PAGE_INTRODUCE
} Page;

//按钮位置
static const Rectangle loginButton = {180, 180, 200, 48};  // 登录按钮的区域，依次为 x、y、宽、高
static const Rectangle registerButton = {420, 180, 200, 48};  // 注册
static const Rectangle switchButton = {180, 180, 440, 48};  // 切换账号
static const Rectangle gameButton = {180, 260, 440, 48};  // 开始游戏
static const Rectangle rankButton = {180, 340, 440, 48};  // 排行榜
static const Rectangle introduceButton = {180, 405, 440, 40};  // 主菜单的玩法说明按钮
static const Rectangle introduceBackButton = {300, 530, 200, 44};  // 玩法说明页返回主菜单
static const Rectangle exitButton = {300, 460, 200, 48};  // 退出程序
static const Rectangle backButton = {300, 460, 200, 48};  // 返回菜单
static const Rectangle easyButton = {250, 150, 300, 48};  // 初级
static const Rectangle mediumButton = {250, 220, 300, 48};  // 中级
static const Rectangle hardButton = {250, 290, 300, 48};  // 高级

static const Rectangle customButton = {250, 360, 300, 48};  // 难度选择页的自定义挑战入口
static const Rectangle customStart = {180, 460, 210, 48};  // 自定义配置页的开始挑战按钮
static const Rectangle customBack = {410, 460, 210, 48};  // 自定义配置页返回难度选择的按钮
static const Rectangle resultBackButton = {410, 480, 210, 48};  // 自定义结算页返回菜单
static void drawCustomConfig(Font font, const CustomGameConfig *cfg);
static void drawCustomResult(Font font, const CustomGameResult *r);

// 在窗口中居中绘制一行文字
static void drawText(Font font, const char text[], float y, float size, Color color);
// 绘制主菜单
static void drawMenu(Font font, int loggedIn);
// 绘制玩法说明页
static void drawIntroduce(Font font);
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

    CustomGameConfig config = {4, 4, 5, 1.0};  // 默认 4×4、亮牌 5 秒，k 为 1.0
    CustomGameResult customResult = {0};  // 从游戏模块读取的通关快照，供结算页使用

    srand((unsigned int)time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);  // 允许调整窗口大小
    InitWindow(800, 600, "记忆力大考验");  // 创建初始大小为 800×600 的窗口
    SetWindowMinSize(800, 600);  // 限制窗口最小为 800×600
    SetTargetFPS(60);  // 设置目标帧率为每秒 60 帧
    SetExitKey(KEY_NULL);  // 禁用默认按 Esc 退出程序的行为

    if (!loadUiResources(&font)) { CloseWindow(); return 1; } ////加载字体
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
                } else if (isButtonClicked(introduceButton)) {//玩法说明
                    page = PAGE_INTRODUCE;
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
                else if (isButtonClicked(customButton)) {//自定义
                    page = PAGE_CUSTOM_CONFIG;
                    break;
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

            case PAGE_CUSTOM_CONFIG: {
                int i;  // 参数行编号：0 行数、1 列数、2 亮牌时间、3 耗时参数
                for (i = 0; i < 4; i++) {
                    // 本帧调整量：点击加号为 1，减号为 -1，未点击为 0
                    int delta = isButtonClicked((Rectangle){520, 130.0f+i*55.0f, 48, 40}) -
                                isButtonClicked((Rectangle){300, 130.0f+i*55.0f, 48, 40});
                    if (i == 0 && config.rows+delta >= 2 && config.rows+delta <= 6) config.rows += delta;
                    if (i == 1 && config.cols+delta >= 2 && config.cols+delta <= 6) config.cols += delta;
                    if (i == 2 && config.previewSeconds + delta >= 1 && config.previewSeconds + delta <= 30)
                        config.previewSeconds += delta;
                    if (i == 3) {
                        int tenths = (int)lround(config.k * 10.0) + delta;
                        if (tenths >= 1 && tenths <= 50) config.k = tenths / 10.0;
                    }
                }
                if (isButtonClicked(customBack)) page = PAGE_LEVEL;
                else if (isButtonClicked(customStart) && isEven(&config)) {
                    startCustomGame(&config);
                    page = PAGE_GAME;
                }
                break;
            }
            case PAGE_GAME:
                gameResult = updateGame(&seconds);
                if (gameResult == 1) {//游戏通关
                    saveResult = 0;
                    if (isCustomGame()) {
                        getCustomGameResult(&customResult);
                    } else if (!getGameCheats()) {// 作弊局只显示结算，不向排行榜提交成绩
                        saveResult = saveScore(user.name, level, seconds);
                    }
                    page = PAGE_RESULT;
                } else if (gameResult == 2) {//游戏中途点击返回按钮
                    page = PAGE_MENU;
                }
                break;

            case PAGE_RESULT:
                //自定义模式
                if (isCustomGame()) {
                    if (isButtonClicked(resultBackButton)) page = PAGE_MENU;
                    break;
                }
                //经典模式
                if (isButtonClicked(backButton)) {//返回菜单
                    page = PAGE_MENU;
                } else if (isButtonClicked(rankButton)) {//查看排行榜
                    setCurrentPlayer(user.name);
                    resetRankPage();
                    page = PAGE_RANK;
                }
                break;

            case PAGE_INTRODUCE:
                if (isButtonClicked(introduceBackButton)) page = PAGE_MENU;
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
            case PAGE_CUSTOM_CONFIG:
                drawCustomConfig(font, &config);
                break;
            case PAGE_GAME:
                drawGame(font);
                break;
            case PAGE_RESULT:
                if (isCustomGame()) { drawCustomResult(font, &customResult); break; }
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
            case PAGE_INTRODUCE:
                drawIntroduce(font);
                break;
            case PAGE_RANK:
                drawRankPage(font);
                break;
        }
        EndMode2D();
        EndDrawing();
    }

    unloadGameResources();
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
        drawButton(introduceButton, "玩法说明", font);
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
    drawButton(customButton, "自定义积分挑战", font);
    drawButton(backButton, "返回", font);
}

/*
 * 功能：绘制自定义挑战的参数控件、规则和配置校验提示
 * 输入：font 为共用中文字体，cfg 为当前配置
 * 输出：设置页画面，不修改配置或开始游戏
 * 返回值：无
 */
static void drawCustomConfig(Font font, const CustomGameConfig *cfg)
{
    int i, groups = cfg->rows * cfg->cols / 2;  // 控件行下标和当前棋盘的总配对组数
    const char *labels[] = {"棋盘行数", "棋盘列数", "亮牌时间", "耗时参数 k"};  // 四行参数控件的标题
    drawText(font, "自定义积分挑战", 70, 34, COLOR_TITLE);
    for (i = 0; i < 4; i++) {
        DrawTextEx(font, labels[i], (Vector2){130, 138.0f+i*55.0f}, 22, 1, COLOR_TITLE);
        drawButton((Rectangle){300, 130.0f+i*55.0f, 48, 40}, "-", font);
        drawButton((Rectangle){520, 130.0f+i*55.0f, 48, 40}, "+", font);
        const char *value;
        if (i == 3) value = TextFormat("%.1f", cfg->k);
        else if (i == 2) value = TextFormat("%d 秒", cfg->previewSeconds);
        else value = TextFormat("%d", i == 0 ? cfg->rows : cfg->cols);
        drawText(font, value, 138.0f+i*55.0f, 24, COLOR_TITLE);
    }
    drawText(font, TextFormat("牌数 %d  配对 %d 组", cfg->rows * cfg->cols, groups), 360, 20, COLOR_TITLE);
    if (isEven(cfg)) {
        CustomGameResult preview;
        calculateCustomResult(cfg, 0, 0, 0, 0.0, &preview);
        drawText(font, TextFormat("总分倍率：棋盘 %.2f 倍   开局亮牌 %.2f 倍", preview.B_plus, 1.0 / preview.K), 395, 20, COLOR_TITLE);
        drawText(font, "k 越大，最快通关加分越多，基准前后得分变化越快", 425, 18, COLOR_HINT);
        drawButton(customStart, "开始挑战", font);
    } else {
        drawText(font, "牌数必须为偶数", 425, 20, COLOR_ERROR);
    }
    drawButton(customBack, "返回", font);
    drawText(font, "亮牌时间到后自动盖牌并开始计时", 540, 18, COLOR_HINT);
}

/*
 * 功能：绘制自定义挑战的基础分、倍率、惩罚和最终积分
 * 输入：font 为共用中文字体，r 为已通关的成绩快照
 * 输出：结算页面，不修改成绩或写入文件
 * 返回值：无
 */
static void drawCustomResult(Font font, const CustomGameResult *r)
{
    drawText(font, "挑战完成", 55, 34, COLOR_TITLE);
    drawText(font, TextFormat("正确 %d 组  错误 %d 次  基础积分 %.2f", r->R, r->W, r->S), 120, 22, COLOR_TITLE);
    drawText(font, TextFormat("棋盘倍率 %.3f", r->B_plus), 165, 22, COLOR_HINT);
    drawText(font, TextFormat("开局展示 %.2f 秒 / 基准 %.2f 秒", r->T, r->T0), 210, 22, COLOR_TITLE);
    drawText(font, TextFormat("亮牌时间倍率 %.3f", r->K), 250, 22, COLOR_HINT);
    drawText(font, TextFormat("用时 %.2f 秒 / 基准 %.2f 秒", r->t, r->Tref), 290, 22, COLOR_TITLE);
    drawText(font, TextFormat("主动亮牌 %d 次  剩余 %.2f%%", r->H, 100 * r->F_H), 330, 22, COLOR_HINT);
    drawText(font, TextFormat("耗时倍率 %.3f", 1.0 - r->B_minus), 370, 22, COLOR_HINT);
    drawText(font, TextFormat("最终积分 %d 分", r->finalScore), 415, 30, COLOR_OK);
    drawButton(resultBackButton, "返回菜单", font);
}

/*
 * 功能：绘制普通模式和自定义挑战的玩法说明
 * 输入：font 为共用中文字体
 * 输出：规则文字和返回按钮，不改变游戏状态
 * 返回值：无
 */
static void drawIntroduce(Font font)
{
    drawText(font, "玩法说明", 40, 34, COLOR_TITLE);
    drawText(font, "基本玩法", 100, 24, COLOR_OK);
    drawText(font, "先记住开局展示的图案，再点击卡牌寻找相同图案。", 140, 20, COLOR_TITLE);
    drawText(font, "初级 4x4、中级 4x5 两张配对；高级 6x6 三张配对。", 175, 20, COLOR_TITLE);
    drawText(font, "错配后保持亮面并锁定 1 秒，到期同时盖回。", 210, 20, COLOR_TITLE);
    drawText(font, "亮牌可查看全部图案 4 秒；游戏中的返回按钮退出本局。", 245, 20, COLOR_TITLE);
    drawText(font, "普通模式按用时排名，使用亮牌的局不入榜。", 280, 20, COLOR_HINT);
    drawText(font, "自定义挑战", 330, 24, COLOR_OK);
    drawText(font, "行列可选 2 到 6，总牌数须为偶数；开局按设置时间亮牌。", 370, 20, COLOR_TITLE);
    drawText(font, "固定两张配对，错配立即盖回并锁定 1 秒。", 405, 20, COLOR_TITLE);
    drawText(font, "通关按棋盘及展示时间调整积分；每次主动亮牌再乘 0.97。", 440, 20, COLOR_TITLE);
    drawText(font, "自定义模式只显示本局积分。", 475, 19, COLOR_HINT);
    drawButton(introduceBackButton, "返回", font);
}
