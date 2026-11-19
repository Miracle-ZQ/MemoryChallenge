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
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

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
    CustomRankConfig initial = {CUSTOM_RANK_FORMULA_VERSION, 4, 4, 5000};
    resetCustomRankPage(&initial);
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


#define CUSTOM_RANK_FILE "custom_ranks.txt"  // 自定义成绩独立存档
#define CUSTOM_RANK_TEMP "custom_ranks.txt.tmp"  // 成功写完后替换正式文件

// 只声明本模块使用的 Windows 文件替换函数，避免 windows.h 与 raylib 的窗口函数重名。
__declspec(dllimport) int __stdcall MoveFileExA(const char *oldName, const char *newName, unsigned long flags);

typedef struct {
    char name[RANK_NAME];  // 玩家名，最多 31 字节
    int points;  // 最终积分，越大越好
    long long millis;  // 正式耗时，单位毫秒，同分时越短越好
} CustomScore;

typedef struct {
    CustomRankConfig config;  // 本榜的完整配置键
    CustomScore scores[CUSTOM_RANK_TOP];  // 当前配置前五名
    int count;  // 本榜已有记录数
} CustomBoard;

static CustomBoard *g_customBoards;  // 当前页面加载的各配置榜单
static int g_customCount;  // 配置榜单个数，包含请求查看的空榜
static CustomScore g_customTop[CUSTOM_RANK_TOP];  // 所有自定义成绩中的个人最好前五名
static int g_customTopCount;  // 积分榜当前显示人数
static int g_customError;  // 读取失败时显示错误，禁止切换旧数据

/*
 * 功能：检查积分榜配置键
 * 输入：config 为待检查的配置
 * 输出：无
 * 返回值：1 合法，0 非法
 */
static int validRankConfig(const CustomRankConfig *config)
{
    return config && config->formulaVersion == CUSTOM_RANK_FORMULA_VERSION &&
        config->rows >= 2 && config->rows <= 6 && config->cols >= 2 && config->cols <= 6 &&
        config->rows * config->cols % 2 == 0 && config->previewMillis >= 1000 && config->previewMillis <= 30000;
}

/*
 * 功能：比较配置键，确定分榜与浏览顺序
 * 输入：a、b 为两个配置键
 * 输出：无
 * 返回值：负数表示 a 在前，0 表示相同，正数表示 b 在前
 */
static int compareConfig(const CustomRankConfig *a, const CustomRankConfig *b)
{
    if (a->formulaVersion != b->formulaVersion) return a->formulaVersion - b->formulaVersion;
    if (a->rows != b->rows) return a->rows - b->rows;
    if (a->cols != b->cols) return a->cols - b->cols;
    return a->previewMillis - b->previewMillis;
}

/*
 * 功能：在配置数组中查找或新增一个空榜，保持配置升序
 * 输入：boards、count 为数组地址和条数地址，config 为合法配置
 * 输出：新增时重新分配数组并更新条数
 * 返回值：榜单下标，分配失败返回 -1
 */
static int findCustomBoard(CustomBoard **boards, int *count, const CustomRankConfig *config)
{
    int i, pos = 0;  // 插入下标及移动记录的循环下标
    CustomBoard *grown;  // 扩容后的数组，失败时保留旧数组
    while (pos < *count && compareConfig(&(*boards)[pos].config, config) < 0) pos++;
    if (pos < *count && compareConfig(&(*boards)[pos].config, config) == 0) return pos;
    grown = realloc(*boards, ((size_t)*count + 1) * sizeof(*grown));
    if (!grown) return -1;
    *boards = grown;
    for (i = *count; i > pos; i--) grown[i] = grown[i - 1];
    grown[pos] = (CustomBoard){0};
    grown[pos].config = *config;
    (*count)++;
    return pos;
}

/*
 * 功能：比较两条成绩
 * 输入：a、b 为待比较成绩
 * 输出：无
 * 返回值：a 分数更高或同分更快时为 1，否则为 0；完全相同不抢先
 */
static int betterCustomScore(const CustomScore *a, const CustomScore *b)
{
    return a->points > b->points || (a->points == b->points && a->millis < b->millis);
}

/*
 * 功能：汇总所有配置的成绩，按分数保留各玩家的最好成绩及前五名
 * 输入：使用已经加载的自定义成绩
 * 输出：更新页面显示数组，不修改原始存档
 * 返回值：无
 */
static void rebuildCustomTop(void)
{
    int k, j, i, dup, pos;  // 配置、成绩、重复玩家和插入位置下标
    g_customTopCount = 0;
    for (k = 0; k < g_customCount; k++) for (j = 0; j < g_customBoards[k].count; j++) {
        CustomScore score = g_customBoards[k].scores[j];  // 本次参与排序的成绩
        dup = -1;
        for (i = 0; i < g_customTopCount; i++) if (!strcmp(g_customTop[i].name, score.name)) dup = i;
        if (dup >= 0) {
            if (!betterCustomScore(&score, &g_customTop[dup])) continue;
            for (i = dup; i < g_customTopCount - 1; i++) g_customTop[i] = g_customTop[i + 1];
            g_customTopCount--;
        }
        pos = 0;
        while (pos < g_customTopCount && !betterCustomScore(&score, &g_customTop[pos])) pos++;
        if (pos == CUSTOM_RANK_TOP) continue;
        if (g_customTopCount < CUSTOM_RANK_TOP) g_customTopCount++;
        for (i = g_customTopCount - 1; i > pos; i--) g_customTop[i] = g_customTop[i - 1];
        g_customTop[pos] = score;
    }
}

/*
 * 功能：从一行文本中读取整数，拒绝溢出或字段粘连
 * 输入：cursor 为当前文本位置，value 接收整数
 * 输出：成功后推进 cursor
 * 返回值：1 成功，0 格式错误
 */
static int readCustomInteger(char **cursor, long long *value)
{
    char *end;  // 数值解析结束位置
    errno = 0;
    *value = strtoll(*cursor, &end, 10);
    if (end == *cursor || errno == ERANGE || (*end && !isspace((unsigned char)*end))) return 0;
    *cursor = end;
    return 1;
}

/*
 * 功能：严格读取独立积分文件，不把读取失败当成空榜
 * 输入：boards、count 接收新数组和配置数，调用前置空
 * 输出：成功时填入全部配置，失败时释放已分配的数据
 * 返回值：1 成功（含文件不存在），0 读取或格式错误
 */
static int loadCustomBoards(CustomBoard **boards, int *count)
{
    FILE *fp = fopen(CUSTOM_RANK_FILE, "r");  // 独立存档，只读打开
    char line[256];  // 一条记录的文本缓冲
    int ok = 1;  // 遇到任意读取或格式错误后停止加载
    if (!fp) return errno == ENOENT;
    if (!fgets(line, sizeof(line), fp) || strcmp(line, "MC_CUSTOM_RANKS 1\n") != 0) ok = 0;
    while (ok && fgets(line, sizeof(line), fp)) {
        char *cursor = line, *name, *end;  // 当前字段位置与用户名边界
        long long values[6];  // 版本、行、列、预览毫秒、积分、耗时
        CustomRankConfig config;  // 从文本恢复的配置键
        CustomScore score = {0};  // 当前成绩
        CustomBoard *board;  // 当前记录所属榜单
        int i, k, pos;  // 字段下标、榜单下标和插入位置
        if (!strchr(line, '\n') && !feof(fp)) { ok = 0; break; }
        while (isspace((unsigned char)*cursor)) cursor++;
        if (!*cursor) continue;
        for (i = 0; i < 6; i++) if (!readCustomInteger(&cursor, &values[i])) break;
        if (i != 6) { ok = 0; break; }
        for (i = 0; i < 5; i++) if (values[i] < 0 || values[i] > INT_MAX) break;
        if (i != 5 || values[5] < 0) { ok = 0; break; }
        config = (CustomRankConfig){(int)values[0], (int)values[1], (int)values[2], (int)values[3]};
        if (!validRankConfig(&config)) { ok = 0; break; }
        while (isspace((unsigned char)*cursor)) cursor++;
        name = cursor;
        while (*cursor && !isspace((unsigned char)*cursor)) cursor++;
        end = cursor;
        while (isspace((unsigned char)*cursor)) cursor++;
        if (end == name || end - name >= RANK_NAME || *cursor) { ok = 0; break; }
        memcpy(score.name, name, (size_t)(end - name));
        score.points = (int)values[4]; score.millis = values[5];
        k = findCustomBoard(boards, count, &config);
        if (k < 0) { ok = 0; break; }
        board = &(*boards)[k];
        for (i = 0; i < board->count; i++) if (strcmp(board->scores[i].name, score.name) == 0) break;
        if (i != board->count || board->count == CUSTOM_RANK_TOP) { ok = 0; break; }
        pos = board->count;
        while (pos > 0 && betterCustomScore(&score, &board->scores[pos - 1])) {
            board->scores[pos] = board->scores[pos - 1]; pos--;
        }
        board->scores[pos] = score; board->count++;
    }
    if (ferror(fp)) ok = 0;
    if (fclose(fp) != 0) ok = 0;
    if (!ok) { free(*boards); *boards = NULL; *count = 0; }
    return ok;
}

/*
 * 功能：写入临时文件，成功关闭后替换正式存档
 * 输入：boards、count 为所有配置榜单
 * 输出：成功时更新独立积分存档，失败时保留旧文件
 * 返回值：1 保存成功，0 写入或替换失败
 */
static int writeCustomBoards(const CustomBoard *boards, int count)
{
    FILE *fp = fopen(CUSTOM_RANK_TEMP, "w");  // 同目录临时文件
    int i, j, ok;  // 配置下标、成绩下标及写入结果
    if (!fp) return 0;
    ok = fprintf(fp, "MC_CUSTOM_RANKS 1\n") > 0;
    for (i = 0; ok && i < count; i++) for (j = 0; ok && j < boards[i].count; j++) {
        const CustomRankConfig *c = &boards[i].config;  // 当前配置
        const CustomScore *s = &boards[i].scores[j];  // 当前成绩
        ok = fprintf(fp, "%d %d %d %d %d %lld %s\n", c->formulaVersion, c->rows, c->cols,
                     c->previewMillis, s->points, s->millis, s->name) > 0;
    }
    if (fflush(fp) != 0) ok = 0;
    if (fclose(fp) != 0) ok = 0;
    // 1=替换已存在文件，8=完成磁盘写入后返回；失败时不先删除原存档
    if (ok) ok = MoveFileExA(CUSTOM_RANK_TEMP, CUSTOM_RANK_FILE, 1 | 8) != 0;
    if (!ok) remove(CUSTOM_RANK_TEMP);
    return ok;
}

/*
 * 功能：提交已通关的自定义积分，各配置仅保留每人最好成绩及前五名
 * 输入：name、config 为玩家与配置，finalScore 为积分，elapsedMillis 为耗时毫秒
 * 输出：需要更新时持久保存，只有保存成功才更新页面数据
 * 返回值：1 保存成功，0 未刷新或未入榜，-1 参数或存档错误
 */
int saveCustomScore(const char name[], const CustomRankConfig *config, int finalScore, long long elapsedMillis)
{
    CustomBoard *boards = NULL, *board;  // 临时加载的完整榜单与本次修改的配置榜
    CustomScore score = {0};  // 待提交成绩
    int count = 0, k, i, dup = -1, pos;  // 配置数、下标、旧个人记录及插入位置
    size_t length;  // 用户名的字节长度
    if (!name || !validRankConfig(config) || finalScore < 0 || elapsedMillis < 0) return -1;
    length = strlen(name);
    if (!length || length >= RANK_NAME) return -1;
    for (i = 0; i < (int)length; i++) if (isspace((unsigned char)name[i])) return -1;
    strcpy(score.name, name); score.points = finalScore; score.millis = elapsedMillis;
    if (!loadCustomBoards(&boards, &count)) return -1;
    k = findCustomBoard(&boards, &count, config);
    if (k < 0) { free(boards); return -1; }
    board = &boards[k];
    for (i = 0; i < board->count; i++) if (strcmp(board->scores[i].name, name) == 0) dup = i;
    if (dup >= 0) {
        if (!betterCustomScore(&score, &board->scores[dup])) { free(boards); return 0; }
        for (i = dup; i < board->count - 1; i++) board->scores[i] = board->scores[i + 1];
        board->count--;
    }
    pos = 0;
    while (pos < board->count && !betterCustomScore(&score, &board->scores[pos])) pos++;
    if (pos == CUSTOM_RANK_TOP) { free(boards); return 0; }
    if (board->count < CUSTOM_RANK_TOP) board->count++;
    for (i = board->count - 1; i > pos; i--) board->scores[i] = board->scores[i - 1];
    board->scores[pos] = score;
    if (!writeCustomBoards(boards, count)) { free(boards); return -1; }
    free(g_customBoards); g_customBoards = boards; g_customCount = count;
    rebuildCustomTop();
    return 1;
}

/*
 * 功能：在统一排行榜页选中自定义，并读取指定配置
 * 输入：config 为合法配置键
 * 输出：更新配置、榜单或错误提示
 * 返回值：1 准备成功，-1 配置或读取失败
 */
int resetCustomRankPage(const CustomRankConfig *config)
{
    CustomBoard *boards = NULL;  // 本次读取的数据
    int count = 0;  // 配置数，包含请求查看的空配置
    g_pageLevel = 4;
    g_customError = 1;
    if (!validRankConfig(config)) return -1;
    if (!loadCustomBoards(&boards, &count)) return -1;
    free(g_customBoards); g_customBoards = boards; g_customCount = count;
    rebuildCustomTop();
    g_customError = 0;
    return 1;
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
