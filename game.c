#include <stdlib.h>
#include "game.h"
#include "ui.h"

// 棋盘数据只由本模块使用，其他模块通过接口取得游戏结果
static int cards[6][6];  // 每个位置的牌面编号，相同编号表示同一种图案
static int state[6][6];  // 牌的状态：0 盖住，1 本轮翻开，2 已配对
static int selectedRows[3], selectedCols[3];  // 本轮所选牌的行、列下标，高级最多选三张
static int rows, cols, count, groupSize;  // 本局行数、列数、当前已选张数、每组需要的张数
static int currentLevel, elapsedSeconds;  // 本局难度和盖牌后已用的整秒数
static int matchErrors;  // 本局配对错误次数，选满一组但不相同时加一次
static double previewEnd, startTime, hideTime;  // 开局亮牌截止时刻、正式计时起点、错牌盖回时刻，单位为秒
static int previewSeconds;  // 当前难度开局亮牌的总秒数
static int cheatCount;  // 本局手动亮牌的使用次数，开局自动亮牌不计入
static double revealEnd;  // 手动亮牌结束的时刻，单位为秒

typedef enum {
    GAME_PREVIEW,   // 开局自动亮牌，供玩家记忆
    GAME_SELECT,    // 玩家点击选牌，进行配对
    GAME_REVEAL,    // 玩家使用手动亮牌，期间不能选牌
    GAME_WAIT,      // 配对失败，等待错牌盖回
    GAME_FINISHED   // 全部配对成功，本局通关
} GamePhase;
static GamePhase phase = GAME_PREVIEW;  // 游戏当前阶段，开局从自动亮牌开始

#define CARD_POOL_SIZE 36  // 图片库中的图案总数
static Texture2D faces[CARD_POOL_SIZE];  // 正面图片纹理，按牌面编号取用
static Texture2D back;  // 所有卡牌共用的背面图片纹理

// 每次根据窗口和棋盘计算，不再固定卡牌宽高和间距
typedef struct {
    float left, top;  // 棋盘左上角在界面布局中的坐标
    float cardWidth, cardHeight, gap;  // 单张牌的宽、高，以及相邻牌之间的间距
    Rectangle backButton;  // 本帧返回按钮的绘制和点击区域
    Rectangle revealButton;  // 本帧手动亮牌按钮的绘制和点击区域
} GameLayout;


static void initBoard(int level);
static void shuffleBoard(void);
static int checkMatch(void);
static int isFinished(void);

static float getPreviewProgress(void);
static GameLayout getGameLayout(void);
static Rectangle cardBounds(GameLayout layout, int row, int col);
static void drawCardPicture(Texture2D picture, Rectangle bounds);

/*
 * 功能：加载 36 种正面和卡背，窗口创建后调用一次
 * 输入：无，读取 assets/cards 中 back.png 和 00.png 到 35.png
 * 输出：保存卡牌正面和卡背纹理
 * 返回值：无
 */
void loadGameResources(void)
{
    int i;  // 遍历图片库中的正面图片
    back = LoadTexture("assets/cards/back.png");//加载背面卡片
    SetTextureFilter(back, TEXTURE_FILTER_BILINEAR);//设置双线性过滤，让图片放大、缩小时的边缘更平滑
    for (i = 0; i < CARD_POOL_SIZE; i++) {
        faces[i] = LoadTexture(TextFormat("assets/cards/%02d.png", i));
        SetTextureFilter(faces[i], TEXTURE_FILTER_BILINEAR);
    }
}

/*
 * 功能：开始新的一局，重置棋盘、时间和选牌记录
 * 输入：level 为 1 初级、2 中级、3 高级
 * 输出：生成并打乱棋盘，进入亮牌阶段
 * 返回值：无
 */
void startGame(int level)
{
    currentLevel = level;
    initBoard(level);
    shuffleBoard();
    count = 0;
    elapsedSeconds = 0;
    matchErrors = 0;
    cheatCount = 0;
    revealEnd = 0;
    startTime = 0;
    hideTime = 0;
    previewSeconds = level == 1 ? 4 : level == 2 ? 6 : 8;
    previewEnd = GetTime() + previewSeconds;
    phase = GAME_PREVIEW;
}

/*
 * 功能：处理本帧的选牌、盖牌和通关，不等待输入或暂停窗口
 * 输入：seconds 是接收用时的地址，鼠标由 raylib 读取
 * 输出：更新棋盘；只有通关时才写入 *seconds，整秒用时从开局盖牌后计算
 * 返回值：0 继续，1 通关，2 退出本局
 */
int updateGame(int *seconds)
{
    int r, c, i;  // 棋盘行下标、列下标，以及本轮选牌数组的循环下标
    double now = GetTime();  // 本帧当前时刻，单位为秒，用于判断各阶段是否到时
    GameLayout layout;  // 本帧使用的棋盘布局和按钮区域

    if (phase == GAME_FINISHED) {//游戏通关，传秒数出去
        *seconds = elapsedSeconds;
        return 1;
    }
    layout = getGameLayout();  // 获取本帧卡牌布局和按钮区域
    if (isButtonClicked(layout.backButton)) {  // 点击返回，通知主函数退出本局// 亮牌和等待盖回期间，仍然可以退出
        return 2;
    }
    if (phase == GAME_PREVIEW) {  // 开局亮牌阶段，暂不接受选牌
        if (now >= previewEnd) {  // 记忆时间到，开始计时并进入选牌阶段
            startTime = now;
            phase = GAME_SELECT;
        }
        return 0;  // 刚盖牌这一帧不同时接收选牌
    }

    elapsedSeconds = (int)(now - startTime);  // 更新整秒用时，包含手动亮牌和错牌等待
    if (phase == GAME_REVEAL) {  // 手动亮牌阶段，暂不接受选牌
        if (now >= revealEnd) {  // 手动亮牌结束，恢复选牌
            phase = GAME_SELECT;
        }
        return 0;  // 亮牌期间不允许选牌或重复触发
    }
    if (phase == GAME_SELECT && isButtonClicked(layout.revealButton)) {  // 点击亮牌按钮，记录次数并亮牌四秒
        cheatCount++;
        revealEnd = now + 4.0;
        phase = GAME_REVEAL;
        return 0;
    }
    if (phase == GAME_WAIT) {  // 错牌展示阶段，等待盖回
        if (now >= hideTime) {  // 等待结束，盖回错牌并恢复选牌
            for (i = 0; i < count; i++) {
                state[selectedRows[i]][selectedCols[i]] = 0;
            }
            count = 0;
            phase = GAME_SELECT;
        }
        return 0;
    }
    for (r = 0; r < rows; r++) {  // 遍历棋盘，寻找被点击的牌
        for (c = 0; c < cols; c++) {
            if (state[r][c] != 0 || !isButtonClicked(cardBounds(layout, r, c))) {
                continue;  // 跳过已翻开、已配对或未被点中的牌
            }
            state[r][c] = 1;  // 翻开当前卡牌
            selectedRows[count] = r;  // 记录所选卡牌的行列位置
            selectedCols[count] = c;
            count++;  // 本轮已选张数加一
            if (count == groupSize) {  // 选满一组后进行配对判断
                if (checkMatch()) {  // 图案全部相同，将本组标记为已配对
                    for (i = 0; i < count; i++) {
                        state[selectedRows[i]][selectedCols[i]] = 2;
                    }
                    count = 0;
                    if (isFinished()) {  // 全部配对完成，写回用时并通知主函数通关
                        phase = GAME_FINISHED;
                        *seconds = elapsedSeconds;
                        return 1;
                    }
                } else {
                    matchErrors++;  // 配对失败，累计错误次数并安排一秒后盖回
                    hideTime = now + 1.0;
                    phase = GAME_WAIT;
                }
            }
            return 0;  // 一次点击最多选择一张牌
        }
    }
    return 0;
}

/*
 * 功能：查询本局配对错误次数，供结算页面显示
 * 输入：无
 * 输出：无，不修改计数
 * 返回值：本局完整选牌后配对失败的次数，新一局从 0 开始
 */
int getGameMistakes(void)
{
    return matchErrors;
}

/*
 * 功能：查询本局手动亮牌的使用次数
 * 输入：无
 * 输出：无，不修改计数；开局自动亮牌不计入
 * 返回值：本局作弊次数，新一局从 0 开始
 */
int getGameCheats(void)
{
    return cheatCount;
}

/*
 * 功能：绘制棋盘、阶段提示、用时和返回按钮
 * 输入：font 为主函数提供的中文字体
 * 输出：当前画面，不改变棋盘、阶段或计时
 * 返回值：无
 */
void drawGame(Font font)
{
    int r, c;  // 遍历棋盘使用的行、列下标
    Rectangle bounds;  // 当前这张牌的绘制区域，与点击判定区域一致
    Color fill;  // 当前卡牌的背景颜色，已配对时使用不同颜色
    const char *label;  // 本次要绘制的状态文字
    GameLayout layout = getGameLayout();  // 按当前窗口尺寸计算得到的本帧布局
    DrawTextEx(font, (currentLevel == 1 ? "初级" : currentLevel == 2 ? "中级" : "高级"),
               (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 10, 8}, 18, 1, DARKGRAY);
    DrawTextEx(font, TextFormat("作弊：%d 次", cheatCount),
               (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 90, 10}, 16, 1, DARKGRAY);
    if (phase == GAME_PREVIEW || phase == GAME_REVEAL) {
        label = phase == GAME_PREVIEW ? "记忆倒计时" : "亮牌倒计时";
    } else {
        label = TextFormat("用时：%d 秒   配对错误：%d 次", elapsedSeconds, matchErrors);
    }
    DrawTextEx(font, label, (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 10, 32}, 16, 1, DARKGRAY);
    if (phase == GAME_PREVIEW || phase == GAME_REVEAL) {
        // 使用剩余时间的比例控制宽度，逐帧连续缩短，而不是每秒跳一格
        float left = -getUiCamera().offset.x / getUiCamera().zoom + 10;  // 窗口左侧留边换算到界面坐标后的位置
        Rectangle track = {left + 170, 34, layout.revealButton.x - left - 182, 10};  // 倒计时进度条完整的背景区域
        Rectangle progress = track;  // 进度条当前已填充区域，宽度随后按剩余时间缩短
        float ratio = getPreviewProgress();
        // 剩余时间少的时候条会变黄、变红
        Color barColor = ratio > 0.5f ? (Color){ 90, 170, 110, 255 }
                       : ratio > 0.2f ? (Color){ 225, 170, 60, 255 }
                                     : (Color){ 205, 80, 80, 255 };
        progress.width *= ratio;
        DrawRectangleRec(track, (Color){218, 226, 236, 255});
        DrawRectangleRec(progress, barColor);
    }
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            bounds = cardBounds(layout, r, c);
            fill = state[r][c] == 2 ? (Color){200, 235, 210, 255} : WHITE;
            if (phase != GAME_PREVIEW && phase != GAME_REVEAL && state[r][c] == 0) {
                DrawRectangleRec(bounds, (Color){75, 125, 185, 255});
                drawCardPicture(back, bounds);
            } else {
                DrawRectangleRec(bounds, fill);
                drawCardPicture(faces[cards[r][c]], bounds);
            }
            DrawRectangleLinesEx(bounds, state[r][c] == 2 ? 3.0f : 1.0f,
                                 state[r][c] == 2 ? DARKGREEN : GRAY);
        }
    }
    drawButton(layout.revealButton, phase == GAME_REVEAL ? "亮牌中" :
               phase == GAME_SELECT ? "亮牌" : "暂不可用", font);
    drawButton(layout.backButton, "返回", font);
}

/*
 * 功能：按难度随机抽取图案，按组生成牌面并清空状态
 * 输入：level 为已经检查有效的难度
 * 输出：更新内部数组、行列数及每组张数
 * 返回值：无
 */
static void initBoard(int level)
{
    int pool[CARD_POOL_SIZE];  // 候选图案编号，抽取时不重复
    int i, j, temp, kinds;
    int r, c, number = 0;  // 初始化用的行、列下标，以及已填入的牌数
    rows = level == 3 ? 6 : 4;
    cols = level == 1 ? 4 : level == 2 ? 5 : 6;
    groupSize = level == 3 ? 3 : 2;
    kinds = rows * cols / groupSize;  // 初、中、高级分别抽取 8、10、12 种
    for (i = 0; i < CARD_POOL_SIZE; i++) pool[i] = i;
    // 每次从剩余候选中抽一个编号，交换到前面，保证本局图案互不重复
    for (i = 0; i < kinds; i++) {
        j = i + rand() % (CARD_POOL_SIZE - i);

        temp = pool[i];
        pool[i] = pool[j];
        pool[j] = temp;
    }
    for (r = 0; r < 6; r++) {
        for (c = 0; c < 6; c++) {
            cards[r][c] = 0;
            state[r][c] = 0;
        }
    }
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            cards[r][c] = pool[number / groupSize];
            number++;
        }
    }
}

/*
 * 功能：从最后一张向前随机交换牌的位置
 * 输入：使用内部棋盘和行列数
 * 输出：打乱 cards，各种牌的数量不变
 * 返回值：无
 */
static void shuffleBoard(void)
{
    int i, j, temp;  // 洗牌时的当前位置、随机交换位置，以及交换用的临时编号
    for (i = rows * cols - 1; i > 0; i--) {
        j = rand() % (i + 1);

        temp = cards[i / cols][i % cols];
        cards[i / cols][i % cols] = cards[j / cols][j % cols];
        cards[j / cols][j % cols] = temp;
    }
}

/*
 * 功能：判断本轮所选牌是否全部相同
 * 输入：使用已记录的选牌坐标和 count
 * 输出：不修改棋盘
 * 返回值：1 相同，0 不同
 */
static int checkMatch(void)
{
    int i;  // 循环下标，用于遍历图片或本轮所选的牌
    int first = cards[selectedRows[0]][selectedCols[0]];  // 本轮第一张牌的编号，其他牌都与它比较
    for (i = 1; i < count; i++) {
        if (cards[selectedRows[i]][selectedCols[i]] != first) return 0;
    }
    return 1;
}

/*
 * 功能：判断所有牌是否已经配对
 * 输入：使用内部 state、rows 和 cols
 * 输出：不修改棋盘
 * 返回值：全部为 2 返回 1，否则返回 0
 */
static int isFinished(void)
{
    int r, c;  // 遍历棋盘使用的行、列下标
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if (state[r][c] != 2) return 0;
        }
    }
    return 1;
}

/*
 * 功能：计算开局或手动亮牌阶段还剩多少比例
 * 输入：使用当前阶段的截止时间、亮牌总秒数和当前时间
 * 输出：无，不改变游戏状态
 * 返回值：剩余时间除以总时长所得的比例
 */
static float getPreviewProgress(void)
{
    float progress;  // 剩余亮牌时间的比例
    if (phase == GAME_REVEAL) {
        progress = (float)((revealEnd - GetTime()) / 4.0);
    } else {
        progress = (float)((previewEnd - GetTime()) / previewSeconds);
    }
    return progress;
}

/*
 * 功能：根据窗口大小和行列数分配棋盘区域
 * 输入：使用当前窗口尺寸和本局 rows、cols
 * 输出：无，不修改棋盘；上方留一条状态区，其余空间尽量分给卡牌
 * 返回值：卡牌宽高、间距、起点和右上角返回按钮的区域
 */
static GameLayout getGameLayout(void)
{
    Camera2D camera = getUiCamera();  // 窗口对应的缩放与居中参数，用于布局坐标换算
    GameLayout layout;  // 本帧使用的棋盘布局和按钮区域
    // 相机为其他页面留出的两侧空白，在游戏页也用来摆牌
    float width = GetScreenWidth() / camera.zoom;  // 窗口宽度换算成界面布局坐标后的长度
    float height = GetScreenHeight() / camera.zoom;  // 窗口高度换算成界面布局坐标后的长度
    float margin = 10;  // 棋盘与窗口边缘保留的距离，使用界面布局单位
    float boardTop = 62;  // 棋盘顶部位置，上方留给状态文字和按钮
    float availableHeight = height - boardTop - margin;  // 扣除状态栏和底部留边后的可用高度

    layout.gap = availableHeight / 90;
    if (layout.gap < 4) layout.gap = 4;
    if (layout.gap > 10) layout.gap = 10;
    layout.left = -camera.offset.x / camera.zoom + margin;
    layout.top = boardTop;
    layout.cardWidth = (width - margin * 2 - (cols - 1) * layout.gap) / cols;
    layout.cardHeight = (availableHeight - (rows - 1) * layout.gap) / rows;
    layout.backButton = (Rectangle){layout.left + width - margin * 2 - 90, 10, 90, 36};
    layout.revealButton = (Rectangle){layout.backButton.x - 100, 10, 90, 36};
    // 取两个方向能容纳的较小边长，保证卡牌为正方形
    if (layout.cardWidth < layout.cardHeight) {
        layout.cardHeight = layout.cardWidth;
    } else {
        layout.cardWidth = layout.cardHeight;
    }
    // 状态栏位置不变，正方形棋盘在下方可用区域中居中
    layout.left += (width - margin * 2 -
                   (cols * layout.cardWidth + (cols - 1) * layout.gap)) / 2;
    layout.top += (availableHeight -
                  (rows * layout.cardHeight + (rows - 1) * layout.gap)) / 2;
    return layout;
}

/*
 * 功能：计算指定卡牌的实际矩形，绘制和点击判断共用此函数
 * 输入：layout 为本帧计算的布局，row、col 为有效行列下标
 * 输出：无
 * 返回值：使用动态卡牌大小和间距计算的矩形
 */
static Rectangle cardBounds(GameLayout layout, int row, int col)
{
    return (Rectangle){layout.left + col * (layout.cardWidth + layout.gap),
                       layout.top + row * (layout.cardHeight + layout.gap),
                       layout.cardWidth, layout.cardHeight};
}

/*
 * 功能：将图片等比例缩放到卡牌内部
 * 输入：picture 是有效纹理，bounds 是卡牌矩形
 * 输出：图片画面，不改变资源
 * 返回值：无
 */
static void drawCardPicture(Texture2D picture, Rectangle bounds)
{
    float padding = (bounds.width < bounds.height ? bounds.width : bounds.height) * 0.08f;  // 图片与卡牌边框的留白，按短边的比例计算
    float sx = (bounds.width - padding * 2) / picture.width;  // 图片按可用宽度放入卡牌时的缩放比例
    float sy = (bounds.height - padding * 2) / picture.height;  // 图片按可用高度放入卡牌时的缩放比例
    float scale = sx < sy ? sx : sy;  // 取较小缩放比例，保证图片完整显示且不变形
    Rectangle dest = {bounds.x + (bounds.width - picture.width * scale) / 2,  // 图片最终绘制的位置和大小，下面两行继续填写矩形成员
                      bounds.y + (bounds.height - picture.height * scale) / 2,
                      picture.width * scale, picture.height * scale};
    DrawTexturePro(picture, (Rectangle){0, 0, (float)picture.width, (float)picture.height},//截取图片是否完整
                   dest, (Vector2){0, 0}, 0, WHITE);//dest：画的位置与大小；{0,0}原点偏移；0偏移角；WHITE不改变颜色
}
