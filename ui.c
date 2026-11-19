#define _CRT_SECURE_NO_WARNINGS

#include "ui.h"

#define UI_FONT_PATH        "assets/fonts/chinese.ttf"
#define UI_SYSTEM_FONT_PATH "C:/Windows/Fonts/simhei.ttf"
#define UI_FONT_SIZE        192
#define UI_CODEPOINT_MAX    1024

static const char UI_GLYPHS[] =
    "人普说"
    "公版"
    "下与且临之也乱互交从他任会传但住体何例供依侧候值偏停允充共其内冻函则刚创判别到制刻剪动区半即历原去参双发受变右各合同向启员四固地址坐块域填处复多奇它实容宽寸寻将小尺尽层居屏左布帧常幕平并序库应底建张待循必快态性总恢截所手才打找抽拒持指按据接控摆收放效整断方旧明显景暗更期机束条果标栏根格框案止此段比水没洗浅深清滑满滤然照状环理生由画界留白的盖目直相矩短禁离种移程立算管累红纳纵纹线终经结绘给绝统编缘缩翻而背致色获表裁览角触认让许证询调起越距跳轮较辨边运还这透逐途遍邻部都释钮长阶际陆除随需非面页顶项预颜黄默鼠"
    "挑战棋盘行列展示基础积分倍率锁定仅两每乘耗扣偶须资源图片检查修正组"
    "先切知忆倒计时当前用户完成配对亮作弊暂不可用中本局使用了亮牌不计入排行榜"
    "三上两个为于仍以位使修包及只号后含和字少尚强或换改有检法空符类置能英被要设该账足达量限"
    "记忆力大考验排行榜初级中级高级返回名次用户名用时秒暂无成绩文件读取失败"
    "写入测试成绩查看退出开始游戏分数恭喜通过再来一局主菜单登录注册密码"
    "点击选择难度卡牌配对准成功剩余时间连击错误提示保存读取加载关闭窗口"
    "重新记录刷新进入前五自定义模式关卡继续取消确定是否玩家当前最高最低"
    "排名第本局得分已保存未保存请输入不存在安全等级图形化演示先"
    "！，。：（）、？；"
    " 0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    ".:-+/()%#\"$&'*;<=>?@[\\]^_`{|}~!";

int loadUiResources(Font *font)
{
    static int codepoints[UI_CODEPOINT_MAX];
    const char *path = UI_FONT_PATH;
    Font loaded;
    int *all;
    int total = 0;
    int count = 0;
    int i;
    int j;

    all = LoadCodepoints(UI_GLYPHS, &total);
    if (all == NULL) return 0;

    for (i = 0; i < total && count < UI_CODEPOINT_MAX; i++)
    {
        int dup = 0;

        for (j = 0; j < count; j++)
        {
            if (codepoints[j] == all[i])
            {
                dup = 1;
                break;
            }
        }

        if (!dup) codepoints[count++] = all[i];
    }

    UnloadCodepoints(all);

    loaded = LoadFontEx(path, UI_FONT_SIZE, codepoints, count);

    SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);

    *font = loaded;
    return 1;
}

void unloadUiResources(Font font)
{
    if (font.texture.id != 0) UnloadFont(font);
}

int isButtonClicked(Rectangle bounds)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 0;

    return CheckCollisionPointRec(GetMousePosition(), bounds) ? 1 : 0;
}

void drawButton(Rectangle bounds, const char text[], Font font)
{
    Vector2 size;
    Vector2 pos;
    int hover;
    Color fill;
    Color border;

    hover = CheckCollisionPointRec(GetMousePosition(), bounds);

    fill = hover ? (Color){ 222, 236, 252, 255 } : (Color){ 246, 248, 252, 255 };
    border = hover ? (Color){ 70, 130, 190, 255 } : (Color){ 150, 165, 185, 255 };

    DrawRectangleRec(bounds, fill);
    DrawRectangleLinesEx(bounds, hover ? 2.0f : 1.0f, border);

    size = MeasureTextEx(font, text, 20.0f, 1.0f);
    pos.x = bounds.x + (bounds.width - size.x) / 2.0f;
    pos.y = bounds.y + (bounds.height - size.y) / 2.0;

    DrawTextEx(font, text, pos, 20.0f, 1.0f, (Color){ 30, 40, 55, 255 });
}

Camera2D getUiCamera(void)
{
    Camera2D camera = {0};
    float scaleX = GetScreenWidth() / 800.0f;
    float scaleY = GetScreenHeight() / 600.0f;

    camera.zoom = scaleX < scaleY ? scaleX : scaleY;

    camera.offset.x = (GetScreenWidth() - 800 * camera.zoom) / 2;
    camera.offset.y = (GetScreenHeight() - 600 * camera.zoom) / 2;

    return camera;
}

#define BG_COUNT 3

static const char *BG_PATH[BG_COUNT] = {
    "assets/bg/account.png",
    "assets/bg/game.png",
    "assets/bg/rank.png"
};
static Texture2D g_bg[BG_COUNT];

int loadBackgrounds(void)
{
    int i;

    for (i = 0; i < BG_COUNT; i++)
        g_bg[i] = LoadTexture(BG_PATH[i]);

    return 1;
}

void drawBackground(int index)
{
    Rectangle src = { 0, 0, (float)g_bg[index].width, (float)g_bg[index].height };
    Rectangle dst = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };

    DrawTexturePro(g_bg[index], src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

void unloadBackgrounds(void)
{
    int i;

    for (i = 0; i < BG_COUNT; i++)
        UnloadTexture(g_bg[i]);
}
