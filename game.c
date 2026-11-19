#include <stdlib.h>
#include <math.h>
#include "game.h"
#include "ui.h"


static int cards[6][6];
static int state[6][6];
static int selectedRows[3], selectedCols[3];
static int rows, cols, count, groupSize;
static int currentLevel, elapsedSeconds;
static int matchErrors;
static double previewEnd, startTime, hideTime;
static double previewSeconds;
static int cheatCount;
static double revealEnd;

static int customMode;
static int successfulMatches;
static CustomGameConfig customConfig;
static CustomGameResult customResult;
static double preciseElapsedSeconds;







static double clampDouble(double v, double lo, double hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}







int validateCustomConfig(const CustomGameConfig *cfg)
{
    return !(cfg->rows * cfg->cols % 2);
}








static void calculateCustomResult(const CustomGameConfig *cfg, int correct,
    int wrong, int reveals, double elapsed, CustomGameResult *out)
{
    CustomGameResult r = {0};
    double adjusted;
    double raw;
    r.groups = correct; r.correct = correct; r.wrong = wrong; r.reveals = reveals;
    r.baseScore = 100 * correct - 40 * wrong; r.elapsedSeconds = elapsed;
    r.boardMultiplier = clampDouble(1 + .04 * (r.groups - 8), .5, 1.5);
    r.previewMultiplier = clampDouble(1 + .05 * (5 - cfg->previewSeconds), .5, 1.5);
    r.plusMultiplier = r.boardMultiplier * r.previewMultiplier;
    r.minusMultiplier = clampDouble(2 - r.previewMultiplier, .5, 2);
    r.revealFactor = pow(.97, reveals);
    r.timePenalty = .30 * clampDouble(elapsed / (5 * r.groups), 0, 1);
    adjusted = 100 * correct * r.plusMultiplier - 40.0 * wrong * r.minusMultiplier;
    raw = fmax(0, adjusted * r.revealFactor * (1 - r.timePenalty));
    r.finalScore = (int)floor(raw + .5); *out = r;
}







int isCustomGame(void) { return customMode; }

typedef enum {
    GAME_PREVIEW,
    GAME_SELECT,
    GAME_REVEAL,
    GAME_WAIT,
    GAME_FINISHED
} GamePhase;
static GamePhase phase = GAME_PREVIEW;

#define CARD_POOL_SIZE 36
static Texture2D faces[CARD_POOL_SIZE];
static Texture2D back;


typedef struct {
    float left, top;
    float cardWidth, cardHeight, gap;
    Rectangle backButton;
    Rectangle revealButton;
} GameLayout;


static void initBoard(int level);
static void shuffleBoard(void);
static int checkMatch(void);
static int isFinished(void);

static float getPreviewProgress(void);
static GameLayout getGameLayout(void);
static Rectangle cardBounds(GameLayout layout, int row, int col);
static void drawCardPicture(Texture2D picture, Rectangle bounds);







void loadGameResources(void)
{
    int i;
    back = LoadTexture("assets/cards/back.png");
    SetTextureFilter(back, TEXTURE_FILTER_BILINEAR);
    for (i = 0; i < CARD_POOL_SIZE; i++) {
        faces[i] = LoadTexture(TextFormat("assets/cards/%02d.png", i));
        SetTextureFilter(faces[i], TEXTURE_FILTER_BILINEAR);
    }
}







void startGame(int level)
{
    customMode = 0;
    successfulMatches = 0;
    preciseElapsedSeconds = 0;
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







void startCustomGame(const CustomGameConfig *cfg)
{
    customConfig = *cfg;
    startGame(1);
    customMode = 1;
    initBoard(0);
    shuffleBoard();
    previewSeconds = cfg->previewSeconds;
    previewEnd = GetTime() + previewSeconds;
}







void getCustomGameResult(CustomGameResult *out)
{
    *out = customResult;
}







void unloadGameResources(void)
{
    int i;
    UnloadTexture(back);
    for (i = 0; i < CARD_POOL_SIZE; i++) {
        UnloadTexture(faces[i]);
    }
}







int updateGame(int *seconds)
{
    int r, c, i;
    double now = GetTime();
    GameLayout layout;

    layout = getGameLayout();
    if (isButtonClicked(layout.backButton)) {
        return 2;
    }
    if (phase == GAME_PREVIEW) {
        if (now >= previewEnd) {
            startTime = now;
            phase = GAME_SELECT;
        }
        return 0;
    }

    preciseElapsedSeconds = now - startTime;
    elapsedSeconds = (int)preciseElapsedSeconds;
    if (phase == GAME_REVEAL) {
        if (now >= revealEnd) {
            phase = GAME_SELECT;
        }
        return 0;
    }
    if (phase == GAME_WAIT) {
        if (now >= hideTime) {
            for (i = 0; i < count; i++) {
                state[selectedRows[i]][selectedCols[i]] = 0;
            }
            count = 0;
            phase = GAME_SELECT;
        }

        if (phase == GAME_WAIT) return 0;
    }
    if (phase == GAME_SELECT && (!customMode || count == 0) && isButtonClicked(layout.revealButton)) {
        cheatCount++;
        revealEnd = now + 4.0;
        phase = GAME_REVEAL;
        return 0;
    }
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if (state[r][c] != 0 || !isButtonClicked(cardBounds(layout, r, c))) {
                continue;
            }
            state[r][c] = 1;
            selectedRows[count] = r;
            selectedCols[count] = c;
            count++;
            if (count == groupSize) {
                if (checkMatch()) {
                    for (i = 0; i < count; i++) {
                        state[selectedRows[i]][selectedCols[i]] = 2;
                    }
                    successfulMatches++;
                    count = 0;
                    if (isFinished()) {
                        phase = GAME_FINISHED;
                        if (customMode) calculateCustomResult(&customConfig, successfulMatches, matchErrors, cheatCount, preciseElapsedSeconds, &customResult);
                        *seconds = elapsedSeconds;
                        return 1;
                    }
                } else {
                    matchErrors++;
                    hideTime = now + 1.0;
                    phase = GAME_WAIT;
                }
            }
            return 0;
        }
    }
    return 0;
}







int getGameMistakes(void)
{
    return matchErrors;
}







int getGameCheats(void)
{
    return cheatCount;
}







void drawGame(Font font)
{
    int r, c;
    Rectangle bounds;
    Color fill;
    const char *label;
    GameLayout layout = getGameLayout();
    DrawTextEx(font, (customMode ? "自定义挑战" : currentLevel == 1 ? "初级" : currentLevel == 2 ? "中级" : "高级"),
               (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 10, 8}, 18, 1, DARKGRAY);
    DrawTextEx(font, TextFormat(customMode ? "亮牌：%d 次" : "作弊：%d 次", cheatCount),
               (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 130, 10}, 16, 1, DARKGRAY);
    if (phase == GAME_PREVIEW || phase == GAME_REVEAL) {
        label = phase == GAME_PREVIEW ? "记忆倒计时" : "亮牌倒计时";
    } else {
        if (customMode && phase == GAME_WAIT) {
            label = TextFormat("用时：%d 秒   配对错误：%d 次   锁定：%.1f 秒",
                               elapsedSeconds, matchErrors, hideTime - GetTime());
        } else {
            label = TextFormat("用时：%d 秒   配对错误：%d 次", elapsedSeconds, matchErrors);
        }
    }
    DrawTextEx(font, label, (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 10, 32}, 16, 1, DARKGRAY);
    if (phase == GAME_PREVIEW || phase == GAME_REVEAL) {

        float left = -getUiCamera().offset.x / getUiCamera().zoom + 10;
        Rectangle track = {left + 170, 34, layout.revealButton.x - left - 182, 10};
        Rectangle progress = track;
        float ratio = getPreviewProgress();

        Color barColor = ratio > 0.5f ? (Color){ 90, 170, 110, 255 }
                       : ratio > 0.2f ? (Color){ 225, 170, 60, 255 }
                                     : (Color){ 205, 80, 80, 255 };
        progress.width *= ratio;
        DrawRectangleRec(track, (Color){218, 226, 236, 255});
        DrawRectangleRec(progress, barColor);
    }
    if (customMode) DrawTextEx(font, TextFormat("已配对 %d/%d  基础积分 %d  亮牌剩余 %.1f%%", successfulMatches, rows*cols/2,
        successfulMatches*100-matchErrors*40, 100*pow(.97, cheatCount)),
        (Vector2){-getUiCamera().offset.x/getUiCamera().zoom+10, 58}, 16, 1, WHITE);
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







static void initBoard(int level)
{
    int pool[CARD_POOL_SIZE];
    int i, j, temp, kinds;
    int r, c, number = 0;
    rows = level == 3 ? 6 : 4;
    cols = level == 1 ? 4 : level == 2 ? 5 : 6;
    groupSize = level == 3 ? 3 : 2;
    if (customMode) { rows = customConfig.rows; cols = customConfig.cols; groupSize = 2; }
    kinds = rows * cols / groupSize;
    for (i = 0; i < CARD_POOL_SIZE; i++) pool[i] = i;

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







static void shuffleBoard(void)
{
    int i, j, temp;
    for (i = rows * cols - 1; i > 0; i--) {
        j = rand() % (i + 1);

        temp = cards[i / cols][i % cols];
        cards[i / cols][i % cols] = cards[j / cols][j % cols];
        cards[j / cols][j % cols] = temp;
    }
}







static int checkMatch(void)
{
    int i;
    int first = cards[selectedRows[0]][selectedCols[0]];
    for (i = 1; i < count; i++) {
        if (cards[selectedRows[i]][selectedCols[i]] != first) return 0;
    }
    return 1;
}







static int isFinished(void)
{
    int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if (state[r][c] != 2) return 0;
        }
    }
    return 1;
}







static float getPreviewProgress(void)
{
    float progress;
    if (phase == GAME_REVEAL) {
        progress = (float)((revealEnd - GetTime()) / 4.0);
    } else {
        progress = (float)((previewEnd - GetTime()) / previewSeconds);
    }
    return progress;
}







static GameLayout getGameLayout(void)
{
    Camera2D camera = getUiCamera();
    GameLayout layout;

    float width = GetScreenWidth() / camera.zoom;
    float height = GetScreenHeight() / camera.zoom;
    float margin = 10;
    float boardTop = customMode ? 86.0f : 62.0f;
    float availableHeight = height - boardTop - margin;

    layout.gap = availableHeight / 90;
    if (layout.gap < 4) layout.gap = 4;
    if (layout.gap > 10) layout.gap = 10;
    layout.left = -camera.offset.x / camera.zoom + margin;
    layout.top = boardTop;
    layout.cardWidth = (width - margin * 2 - (cols - 1) * layout.gap) / cols;
    layout.cardHeight = (availableHeight - (rows - 1) * layout.gap) / rows;
    layout.backButton = (Rectangle){layout.left + width - margin * 2 - 90, 10, 90, 36};
    layout.revealButton = (Rectangle){layout.backButton.x - 100, 10, 90, 36};

    if (layout.cardWidth < layout.cardHeight) {
        layout.cardHeight = layout.cardWidth;
    } else {
        layout.cardWidth = layout.cardHeight;
    }

    layout.left += (width - margin * 2 -
                   (cols * layout.cardWidth + (cols - 1) * layout.gap)) / 2;
    layout.top += (availableHeight -
                  (rows * layout.cardHeight + (rows - 1) * layout.gap)) / 2;
    return layout;
}







static Rectangle cardBounds(GameLayout layout, int row, int col)
{
    return (Rectangle){layout.left + col * (layout.cardWidth + layout.gap),
                       layout.top + row * (layout.cardHeight + layout.gap),
                       layout.cardWidth, layout.cardHeight};
}







static void drawCardPicture(Texture2D picture, Rectangle bounds)
{
    float padding = (bounds.width < bounds.height ? bounds.width : bounds.height) * 0.08f;
    float sx = (bounds.width - padding * 2) / picture.width;
    float sy = (bounds.height - padding * 2) / picture.height;
    float scale = sx < sy ? sx : sy;
    Rectangle dest = {bounds.x + (bounds.width - picture.width * scale) / 2,
                      bounds.y + (bounds.height - picture.height * scale) / 2,
                      picture.width * scale, picture.height * scale};
    DrawTexturePro(picture, (Rectangle){0, 0, (float)picture.width, (float)picture.height},
                   dest, (Vector2){0, 0}, 0, WHITE);
}
