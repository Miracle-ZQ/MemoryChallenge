#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>

#include "rank.h"
#include "ui.h"

#define RANK_LEVELS 3
#define RANK_TOP    5
#define RANK_NAME   32
#define RANK_FILE   "ranks.txt"

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
static int   g_loaded = 0;
static int   g_pageLevel = 1;
static char  g_player[RANK_NAME];

static const Rectangle g_backButton = { 40.0f, 520.0f, BTN_W, BTN_H };

static const char *levelName(int level)
{
    if (level == 1) return "初级";
    if (level == 2) return "中级";
    if (level == 3) return "高级";
    if (level == 4) return "自定义";
    return "未知";
}

static void sortLevel(int k)
{
    int i;
    int j;
    Score tmp;

    for (i = 1; i < g_count[k]; i++)
    {
        tmp = g_rank[k][i];
        j = i;

        while (j > 0 && g_rank[k][j - 1].seconds > tmp.seconds)
        {
            g_rank[k][j] = g_rank[k][j - 1];
            j--;
        }

        g_rank[k][j] = tmp;
    }
}

static void loadRanks(void)
{
    FILE *fp;
    char line[128];
    int k;

    for (k = 0; k < RANK_LEVELS; k++)
    {
        g_count[k] = 0;
    }

    fp = fopen(RANK_FILE, "r");

    if (fp == NULL)
    {
        g_loaded = 1;
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char name[RANK_NAME];
        int level;
        int seconds;

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

    for (k = 0; k < RANK_LEVELS; k++) sortLevel(k);

    g_loaded = 1;
}

static int writeRanks(void)
{
    FILE *fp;
    int k;
    int i;

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

int saveScore(const char name[], int level, int seconds)
{
    int k;
    int i;
    int pos;
    int cnt;
    int dup;

    if (name == NULL || name[0] == '\0') return -1;
    if (level < 1 || level > RANK_LEVELS) return -1;
    if (seconds < 0) return -1;

    if (!g_loaded) loadRanks();

    k = level - 1;

    dup = -1;
    for (i = 0; i < g_count[k]; i++)
    {
        if (strcmp(g_rank[k][i].name, name) == 0)
        {
            dup = i;
            break;
        }
    }

    if (dup >= 0 && seconds >= g_rank[k][dup].seconds) return 0;

    pos = 0;
    while (pos < g_count[k] && g_rank[k][pos].seconds <= seconds) pos++;

    if (pos >= RANK_TOP) return 0;

    if (dup >= 0)
    {
        for (i = dup; i < g_count[k] - 1; i++) g_rank[k][i] = g_rank[k][i + 1];
        g_count[k]--;
        if (pos > dup) pos--;
    }

    cnt = g_count[k];
    if (cnt < RANK_TOP) cnt++;

    for (i = cnt - 1; i > pos; i--) g_rank[k][i] = g_rank[k][i - 1];

    strncpy(g_rank[k][pos].name, name, RANK_NAME - 1);
    g_rank[k][pos].name[RANK_NAME - 1] = '\0';
    g_rank[k][pos].seconds = seconds;
    g_count[k] = cnt;

    if (!writeRanks())
    {
        g_loaded = 0;
        loadRanks();
        return -1;
    }

    return 1;
}

void setCurrentPlayer(const char name[])
{
    strncpy(g_player, name, RANK_NAME - 1);
    g_player[RANK_NAME - 1] = '\0';
}

void resetRankPage(void)
{
    resetCustomRankPage();
    g_pageLevel = 1;

    if (!g_loaded) loadRanks();
}

static Rectangle levelButtonRect(int level)
{
    Rectangle rect;

    rect.x = BTN_X0 + (level - 1) * (BTN_W + BTN_GAP);
    rect.y = BTN_Y;
    rect.width = BTN_W;
    rect.height = BTN_H;

    return rect;
}

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

void drawRankPage(Font font)
{
    int level;
    int k;
    int i;
    float w;
    char text[128];

    w = MeasureTextEx(font, "排行榜", 36.0f, 2.0f).x;
    DrawTextEx(font, "排行榜", (Vector2){ (800.0f - w) / 2.0f, 40.0f },
               36.0f, 2.0f, (Color){ 232, 240, 252, 255 });

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

    if (g_count[k] == 0)
    {
        w = MeasureTextEx(font, "暂无成绩", 24.0f, 1.0f).x;
        DrawTextEx(font, "暂无成绩", (Vector2){ (800.0f - w) / 2.0f, 300.0f },
                   24.0f, 1.0f, (Color){ 185, 198, 218, 255 });
        return;
    }

    DrawTextEx(font, "名次", (Vector2){ 150.0f, 196.0f }, 22.0f, 1.0f, (Color){ 185, 198, 218, 255 });
    DrawTextEx(font, "用户名", (Vector2){ 250.0f, 196.0f }, 22.0f, 1.0f, (Color){ 185, 198, 218, 255 });
    DrawTextEx(font, "用时", (Vector2){ 480.0f, 196.0f }, 22.0f, 1.0f, (Color){ 185, 198, 218, 255 });
    DrawRectangle(130, 226, 420, 1, (Color){ 120, 135, 158, 255 });

    for (i = 0; i < g_count[k]; i++)
    {
        float y = 244.0f + i * 44.0f;

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

#define CUSTOM_RANK_FILE "custom_ranks.txt"

typedef struct {
    char name[RANK_NAME];
    int points;
} CustomScore;

static CustomScore g_customTop[CUSTOM_RANK_TOP];
static int g_customTopCount;
static int g_customError;

static int insertCustomScore(CustomScore scores[], int *count, CustomScore score)
{
    int i, dup = -1, pos = 0;
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

static int loadCustomScores(CustomScore scores[], int *count)
{
    FILE *fp = fopen(CUSTOM_RANK_FILE, "r");
    CustomScore score;
    int fields, ok;
    *count = 0;
    if (!fp) return 1;
    while ((fields = fscanf(fp, "%31s %d", score.name, &score.points)) == 2) {
        insertCustomScore(scores, count, score);
    }
    ok = fields == EOF && !ferror(fp);
    fclose(fp);
    return ok;
}

static int writeCustomScores(const CustomScore scores[], int count)
{
    FILE *fp = fopen(CUSTOM_RANK_FILE, "w");
    int i, ok = 1;
    if (!fp) return 0;
    for (i = 0; i < count; i++) if (fprintf(fp, "%s %d\n", scores[i].name, scores[i].points) < 0) ok = 0;
    if (fclose(fp) != 0) ok = 0;
    return ok;
}

int saveCustomScore(const char name[], int finalScore)
{
    CustomScore scores[CUSTOM_RANK_TOP];
    CustomScore score;
    int count;
    if (!loadCustomScores(scores, &count)) return -1;
    strcpy(score.name, name);
    score.points = finalScore;
    if (!insertCustomScore(scores, &count, score)) return 0;
    if (!writeCustomScores(scores, count)) return -1;
    memcpy(g_customTop, scores, (size_t)count * sizeof(*scores));
    g_customTopCount = count;
    return 1;
}

int resetCustomRankPage(void)
{
    g_pageLevel = 4;
    g_customError = !loadCustomScores(g_customTop, &g_customTopCount);
    return g_customError ? -1 : 1;
}

int updateCustomRankPage(void)
{
    return isButtonClicked(g_backButton);
}

void drawCustomRankPage(Font font)
{
    int i;
    Color text = {232, 240, 252, 255};
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
        const CustomScore *score = &g_customTop[i];
        float y = 260.0f + i * 44.0f;
        if (strcmp(score->name, g_player) == 0) DrawRectangle(120, (int)y - 5, 560, 36, (Color){60, 110, 180, 120});
        DrawTextEx(font, TextFormat("%d", i + 1), (Vector2){150, y}, 22, 1, text);
        DrawTextEx(font, score->name, (Vector2){250, y}, 22, 1, text);
        DrawTextEx(font, TextFormat("%d", score->points), (Vector2){550, y}, 22, 1, text);
    }
}
