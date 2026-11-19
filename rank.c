#define _CRT_SECURE_NO_WARNINGS

/*
 * rank.c 排行榜：成绩保存 + 排行榜页面
 *
 * ranks.txt 每行：难度 用户名 用时
 * 每档最多保留 5 名，同一玩家只留更短的那次，同用时时先入榜的在前
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "rank.h"
#include "ui.h"

#define RANK_LEVELS 3
#define RANK_TOP    5
#define RANK_NAME   32
#define RANK_FILE   "ranks.txt"

// 页面布局：绘制和点击共用这几个值
#define BTN_W       150.0f
#define BTN_H       44.0f
#define BTN_Y       112.0f
#define BTN_GAP     20.0f
#define BTN_X0      70.0f

typedef struct
{
    char name[RANK_NAME];
    int  seconds;
} Score;

static Score g_rank[RANK_LEVELS][RANK_TOP];
static int   g_count[RANK_LEVELS];
static int   g_loaded = 0;       // 是否已从文件读过
static int   g_readError = 0;    // 读取失败时禁止写文件
static int   g_pageLevel = 1;
static char  g_player[RANK_NAME];   // 当前玩家名，用于高亮自己那一行    // 排行榜当前显示第几档

static const Rectangle g_backButton = { 40.0f, 520.0f, BTN_W, BTN_H };

/*
 * 功能：难度编号转中文名
 */
static const char *levelName(int level)
{
    if (level == 1) return "初级";
    if (level == 2) return "中级";
    if (level == 3) return "高级";
    if (level == 4) return "自定义";
    return "未知";
}

/*
 * 功能：把某一档成绩按用时从小到大排序
 * 输入：k 是难度下标
 * 输出：直接调整数组，同用时的记录保持原顺序（插入排序是稳定的）
 * 返回值：无
 */
static void sortLevel(int k)//排行
{
    int i;
    int j;
    Score tmp;

    for (i = 1; i < g_count[k]; i++)
    {
        tmp = g_rank[k][i];
        j = i;

        // 只有严格更慢才后移，保证同用时先入榜的仍在前面
        while (j > 0 && g_rank[k][j - 1].seconds > tmp.seconds)
        {
            g_rank[k][j] = g_rank[k][j - 1];
            j--;
        }

        g_rank[k][j] = tmp;
    }
}

/*
 * 功能：从 ranks.txt 读取成绩
 * 输入：无
 * 输出：更新成绩数组和条数；文件不存在按空榜处理
 * 返回值：无
 */
static void loadRanks(void)
{
    FILE *fp;
    char line[128];
    int k;

    // 清空，避免重复读取时数据叠加
    for (k = 0; k < RANK_LEVELS; k++)
    {
        g_count[k] = 0;
    }

    g_readError = 0;
    errno = 0;

    fp = fopen(RANK_FILE, "r");

    if (fp == NULL)
    {
        // 文件不存在是第一次运行的正常情况；其它原因（被占用、无权限）
        // 说明读取失败，此时不能当成空榜，否则保存会把原成绩覆盖掉
        if (errno == ENOENT) g_loaded = 1;
        else g_readError = 1;

        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char name[RANK_NAME];
        int level;
        int seconds;

        // 坏行直接跳过
        if (sscanf(line, "%d %31s %d", &level, name, &seconds) != 3) continue;
        if (level < 1 || level > RANK_LEVELS) continue;
        if (seconds < 0) continue;
        if (g_count[level - 1] >= RANK_TOP) continue;

        k = level - 1;
        strncpy(g_rank[k][g_count[k]].name, name, RANK_NAME - 1);
        g_rank[k][g_count[k]].name[RANK_NAME - 1] = '\0';
        g_rank[k][g_count[k]].seconds = seconds;
        g_count[k]++;
    }

    fclose(fp);

    // 文件可能被手工改过，读入后统一排序
    for (k = 0; k < RANK_LEVELS; k++) sortLevel(k);

    g_loaded = 1;
}

/*
 * 功能：把成绩写回 ranks.txt
 * 输入：无
 * 输出：覆盖写文件
 * 返回值：成功 1，失败 0
 */
static int writeRanks(void)
{
    FILE *fp;
    int k;
    int i;

    if (g_readError) return 0;      // 读过一次失败就别再覆盖原文件

    fp = fopen(RANK_FILE, "w");
    if (fp == NULL) return 0;

    for (k = 0; k < RANK_LEVELS; k++)
    {
        for (i = 0; i < g_count[k]; i++)
        {
            fprintf(fp, "%d %s %d\n", k + 1, g_rank[k][i].name, g_rank[k][i].seconds);
        }
    }

    fclose(fp);
    return 1;
}

/*
 * 功能：提交成绩，保留各档前五名和同一玩家的最好成绩
 * 输入：name 用户名，level 难度 1、2、3，seconds 非负用时
 * 输出：需要更新时修改数组并写文件
 * 返回值：1 更新成功，0 未刷新或未进前五，-1 参数或文件错误
 */
int saveScore(const char name[], int level, int seconds)
{
    int k;
    int i;
    int pos;
    int cnt;
    int dup;

    // 参数检查（0 秒是合法成绩）
    if (name == NULL || name[0] == '\0') return -1;
    if (level < 1 || level > RANK_LEVELS) return -1;
    if (seconds < 0) return -1;

    if (!g_loaded) loadRanks();     // 第一次用到才读盘
    if (g_readError) return -1;

    k = level - 1;

    // 找同名玩家
    dup = -1;
    for (i = 0; i < g_count[k]; i++)
    {
        if (strcmp(g_rank[k][i].name, name) == 0)
        {
            dup = i;
            break;
        }
    }

    // 同名不更快就不刷新
    if (dup >= 0 && seconds >= g_rank[k][dup].seconds) return 0;

    // 找插入位置：第一个比自己慢的人前面
    // 用 <= 让同用时后来者靠后
    pos = 0;
    while (pos < g_count[k] && g_rank[k][pos].seconds <= seconds) pos++;

    if (pos >= RANK_TOP) return 0;  // 比第五名还慢，进不了榜

    // 自己原来在榜上就先删掉旧记录
    if (dup >= 0)
    {
        for (i = dup; i < g_count[k] - 1; i++) g_rank[k][i] = g_rank[k][i + 1];
        g_count[k]--;
        if (pos > dup) pos--;
    }

    // 腾空位并插入（从后往前挪，否则会覆盖自己）
    cnt = g_count[k];
    if (cnt < RANK_TOP) cnt++;

    for (i = cnt - 1; i > pos; i--) g_rank[k][i] = g_rank[k][i - 1];

    strncpy(g_rank[k][pos].name, name, RANK_NAME - 1);
    g_rank[k][pos].name[RANK_NAME - 1] = '\0';
    g_rank[k][pos].seconds = seconds;
    g_count[k] = cnt;

    // 保存失败就把内存恢复成文件里的内容，免得下次被误判
    if (!writeRanks())
    {
        g_loaded = 0;
        loadRanks();
        return -1;
    }

    return 1;
}

/*
 * 功能：记录当前玩家名
 * 输入：name 为用户名
 * 输出：保存到 g_player
 * 返回值：无
 */
void setCurrentPlayer(const char name[])
{
    strncpy(g_player, name, RANK_NAME - 1);
    g_player[RANK_NAME - 1] = '\0';
}

/*
 * 功能：准备排行榜页面
 * 输入：无
 * 输出：默认显示初级，必要时读一次文件
 * 返回值：无
 */
void resetRankPage(void)
{
    resetCustomRankPage();
    g_pageLevel = 1;

    if (!g_loaded) loadRanks();
}

/*
 * 功能：取得某一档难度按钮的位置
 * 输入：level 是难度 1、2、3
 * 输出：无
 * 返回值：按钮矩形
 */
static Rectangle levelButtonRect(int level)
{
    Rectangle rect;

    rect.x = BTN_X0 + (level - 1) * (BTN_W + BTN_GAP);
    rect.y = BTN_Y;
    rect.width = BTN_W;
    rect.height = BTN_H;

    return rect;
}

/*
 * 功能：处理难度选择和返回按钮
 * 输入：无，读本帧鼠标
 * 输出：修改当前显示的难度
 * 返回值：0 留在本页，1 返回主菜单
 */
int updateRankPage(void)
{
    int level;

    for (level = 1; level <= RANK_LEVELS + 1; level++)
    {
        if (isButtonClicked(levelButtonRect(level)))
        {
            g_pageLevel = level;
            return 0;
        }
    }

    if (g_pageLevel == 4) return updateCustomRankPage();
    if (isButtonClicked(g_backButton)) return 1;

    return 0;
}

/*
 * 功能：绘制当前难度的排行榜
 * 输入：font 为中文字体
 * 输出：标题、难度按钮、成绩列表，或“暂无成绩”
 * 返回值：无
 */
void drawRankPage(Font font)
{
    int level;
    int k;
    int i;
    float w;
    char text[128];

    // 标题居中
    w = MeasureTextEx(font, "排行榜", 36.0f, 2.0f).x;
    DrawTextEx(font, "排行榜", (Vector2){ (800.0f - w) / 2.0f, 40.0f },
               36.0f, 2.0f, (Color){ 232, 240, 252, 255 });

    // 四个榜单选项，当前选项下方画蓝线
    for (level = 1; level <= RANK_LEVELS + 1; level++)
    {
        Rectangle rect = levelButtonRect(level);

        drawButton(rect, levelName(level), font);

        if (level == g_pageLevel)
        {
            DrawRectangle((int)rect.x, (int)(rect.y + rect.height + 4.0f),
                          (int)rect.width, 3, (Color){ 70, 130, 190, 255 });
        }
    }

    drawButton(g_backButton, "返回", font);

    if (g_pageLevel == 4) { drawCustomRankPage(font); return; }
    k = g_pageLevel - 1;

    // 空榜：只画一行提示，页面照常可以切换和返回
    if (g_count[k] == 0)
    {
        w = MeasureTextEx(font, "暂无成绩", 24.0f, 1.0f).x;
        DrawTextEx(font, "暂无成绩", (Vector2){ (800.0f - w) / 2.0f, 300.0f },
                   24.0f, 1.0f, (Color){ 185, 198, 218, 255 });
        return;
    }

    // 表头
    DrawTextEx(font, "名次", (Vector2){ 150.0f, 196.0f }, 22.0f, 1.0f, (Color){ 185, 198, 218, 255 });
    DrawTextEx(font, "用户名", (Vector2){ 250.0f, 196.0f }, 22.0f, 1.0f, (Color){ 185, 198, 218, 255 });
    DrawTextEx(font, "用时", (Vector2){ 480.0f, 196.0f }, 22.0f, 1.0f, (Color){ 185, 198, 218, 255 });
    DrawRectangle(130, 226, 420, 1, (Color){ 120, 135, 158, 255 });

    // 成绩列表
    for (i = 0; i < g_count[k]; i++)
    {
        float y = 244.0f + i * 44.0f;

        // 自己那一行加半透明深蓝底
        if (strcmp(g_rank[k][i].name, g_player) == 0)
            DrawRectangle(120, (int)y - 6, 440, 36, (Color){ 60, 110, 180, 120 });

        sprintf(text, "%d", i + 1);
        DrawTextEx(font, text, (Vector2){ 150.0f, y }, 22.0f, 1.0f, (Color){ 232, 240, 252, 255 });

        DrawTextEx(font, g_rank[k][i].name, (Vector2){ 250.0f, y },
                   22.0f, 1.0f, (Color){ 232, 240, 252, 255 });

        sprintf(text, "%d 秒", g_rank[k][i].seconds);
        DrawTextEx(font, text, (Vector2){ 480.0f, y }, 22.0f, 1.0f, (Color){ 232, 240, 252, 255 });
    }
}


#define CUSTOM_RANK_FILE "custom_ranks.txt"  // 每行保存用户名和最高分
#define CUSTOM_RANK_TEMP "custom_ranks.txt.tmp"  // 写完后替换正式文件

// 使用 Windows 文件替换函数，避免 windows.h 与 raylib 的窗口函数重名
__declspec(dllimport) int __stdcall MoveFileExA(const char *oldName, const char *newName, unsigned long flags);

typedef struct {
    char name[RANK_NAME];  // 玩家名
    int points;  // 玩家最高分
} CustomScore;

static CustomScore g_customTop[CUSTOM_RANK_TOP];  // 自定义积分前五名
static int g_customTopCount;  // 当前上榜人数
static int g_customError;  // 本次文件读取是否失败

/*
 * 功能：将一条成绩插入前五名，同名只保留最高分
 * 输入：scores、count 为榜单数组与人数地址，score 为提交成绩
 * 输出：按分数降序更新数组，同分保持原顺序
 * 返回值：1 榜单更新，0 未刷新或未入榜
 */
static int insertCustomScore(CustomScore scores[], int *count, CustomScore score)
{
    int i, dup = -1, pos = 0;  // 循环下标、同名记录和插入位置
    for (i = 0; i < *count; i++) if (!strcmp(scores[i].name, score.name)) dup = i;
    if (dup >= 0) {
        if (score.points <= scores[dup].points) return 0;
        for (i = dup; i < *count - 1; i++) scores[i] = scores[i + 1];
        (*count)--;
    }
    while (pos < *count && scores[pos].points >= score.points) pos++;
    if (pos == CUSTOM_RANK_TOP) return 0;
    if (*count < CUSTOM_RANK_TOP) (*count)++;
    for (i = *count - 1; i > pos; i--) scores[i] = scores[i - 1];
    scores[pos] = score;
    return 1;
}

/*
 * 功能：读取用户名和最高分
 * 输入：scores、count 接收榜单及人数
 * 输出：按分数降序排列的前五名，文件不存在时为空榜
 * 返回值：1 读取成功，0 文件错误
 */
static int loadCustomScores(CustomScore scores[], int *count)
{
    FILE *fp = fopen(CUSTOM_RANK_FILE, "r");  // 独立积分文件
    CustomScore score;  // 当前读入的用户名和分数
    int fields, ok;  // 每行读到的字段数和最终读取结果
    *count = 0;
    if (!fp) return errno == ENOENT;
    while ((fields = fscanf(fp, "%31s %d", score.name, &score.points)) == 2) {
        insertCustomScore(scores, count, score);
    }
    ok = fields == EOF && !ferror(fp);
    fclose(fp);
    return ok;
}

/*
 * 功能：保存自定义积分榜
 * 输入：scores、count 为前五名和人数
 * 输出：每行写入用户名、最高分
 * 返回值：1 保存成功，0 文件错误
 */
static int writeCustomScores(const CustomScore scores[], int count)
{
    FILE *fp = fopen(CUSTOM_RANK_TEMP, "w");  // 先写临时文件，再替换原文件
    int i, ok = 1;  // 当前成绩下标与写入结果
    if (!fp) return 0;
    for (i = 0; i < count; i++) if (fprintf(fp, "%s %d\n", scores[i].name, scores[i].points) < 0) ok = 0;
    if (fclose(fp) != 0) ok = 0;
    if (ok) ok = MoveFileExA(CUSTOM_RANK_TEMP, CUSTOM_RANK_FILE, 1 | 8) != 0;
    if (!ok) remove(CUSTOM_RANK_TEMP);
    return ok;
}

/*
 * 功能：提交已通关的最终积分，各玩家只保留最高分
 * 输入：name 为已登录用户名，finalScore 为最终积分
 * 输出：更新独立积分文件及页面榜单
 * 返回值：1 保存成功，0 未刷新或未入榜，-1 文件错误
 */
int saveCustomScore(const char name[], int finalScore)
{
    CustomScore scores[CUSTOM_RANK_TOP];  // 本次处理的榜单副本
    CustomScore score;  // 本局提交的成绩
    int count;  // 读取到的上榜人数
    if (!loadCustomScores(scores, &count)) return -1;
    strcpy(score.name, name);
    score.points = finalScore;
    if (!insertCustomScore(scores, &count, score)) return 0;
    if (!writeCustomScores(scores, count)) return -1;
    memcpy(g_customTop, scores, (size_t)count * sizeof(*scores));
    g_customTopCount = count;
    return 1;
}

/*
 * 功能：读取自定义积分榜并选择自定义选项
 * 输入：无
 * 输出：更新前五名或读取错误提示
 * 返回值：1 成功，-1 文件错误
 */
int resetCustomRankPage(void)
{
    g_pageLevel = 4;
    g_customError = !loadCustomScores(g_customTop, &g_customTopCount);
    return g_customError ? -1 : 1;
}

/*
 * 功能：处理自定义积分榜的返回操作
 * 输入：无，使用本帧鼠标点击
 * 输出：无
 * 返回值：0 留在榜单，1 返回菜单
 */
int updateCustomRankPage(void)
{
    return isButtonClicked(g_backButton);
}

/*
 * 功能：绘制自定义积分前五名，顶部选项由 drawRankPage 绘制
 * 输入：font 为公共中文字体
 * 输出：名次、用户名和分数，不读写文件
 * 返回值：无
 */
void drawCustomRankPage(Font font)
{
    int i;  // 当前绘制的成绩行号
    Color text = {232, 240, 252, 255};  // 暗色背景上的正文颜色
    if (g_customError) {
        DrawTextEx(font, "积分榜读取失败", (Vector2){300, 295}, 22, 1, text);
        return;
    }
    if (!g_customTopCount) {
        DrawTextEx(font, "暂无成绩", (Vector2){330, 295}, 24, 1, text);
        return;
    }
    DrawTextEx(font, "名次", (Vector2){150, 210}, 22, 1, text);
    DrawTextEx(font, "用户名", (Vector2){250, 210}, 22, 1, text);
    DrawTextEx(font, "分数", (Vector2){550, 210}, 22, 1, text);
    for (i = 0; i < g_customTopCount; i++) {
        const CustomScore *score = &g_customTop[i];  // 当前行的玩家及分数
        float y = 260.0f + i * 44.0f;  // 五名成绩的纵坐标
        if (strcmp(score->name, g_player) == 0) DrawRectangle(120, (int)y - 5, 560, 36, (Color){60, 110, 180, 120});
        DrawTextEx(font, TextFormat("%d", i + 1), (Vector2){150, y}, 22, 1, text);
        DrawTextEx(font, score->name, (Vector2){250, y}, 22, 1, text);
        DrawTextEx(font, TextFormat("%d", score->points), (Vector2){550, y}, 22, 1, text);
    }
}
