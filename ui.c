#define _CRT_SECURE_NO_WARNINGS

/*
 * ui.c 公共界面：中文字体加载 + 按钮点击判断 + 按钮绘制
 *
 * 字体为什么要特殊处理：
 *   raylib 默认字体只有 224 个 ASCII 字形，没有中文，
 *   必须用 LoadFontEx 按"码点（字符编号）"把要用的字一个个挑出来加载
 */

#include "ui.h"

#define UI_FONT_PATH        "assets/fonts/chinese.ttf"      // 组内提供的字体
#define UI_SYSTEM_FONT_PATH "C:/Windows/Fonts/simhei.ttf"   // 没有组内字体时用系统黑体
#define UI_FONT_SIZE        192      // 字体生成尺寸
#define UI_CODEPOINT_MAX    512      // 最多加载多少个字

// 界面里用到的全部文字
// 没写进这里的字画出来会是空白或方块，加了新提示记得往这里补字
static const char UI_GLYPHS[] =
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

/*
 * 功能：窗口创建后加载界面中文字体
 * 输入：font 是接收字体的变量地址
 * 输出：成功时把字体写入 *font
 * 返回值：1 成功，0 失败
 */
int loadUiResources(Font *font)
{
    static int codepoints[UI_CODEPOINT_MAX];   // 去重后的字符编号
    const char *path = UI_FONT_PATH;
    Font loaded;
    int *all;
    int total = 0;
    int count = 0;
    int i;
    int j;

    if (font == NULL) return 0;

    
    if (!FileExists(path)) path = UI_SYSTEM_FONT_PATH;//如果没有组内字体就用系统黑体
    if (!FileExists(path)) return 0;//如果还没有就放弃

    all = LoadCodepoints(UI_GLYPHS, &total);//raylib只能识别编号，所以要进行转换
    if (all == NULL) return 0;

    for (i = 0; i < total && count < UI_CODEPOINT_MAX; i++)//去重操作，已经有了的字体就不再加载
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

    UnloadCodepoints(all);      // LoadCodepoints 申请的内存要还回去

    if (count == 0) return 0;

    // 按码点加载字体：只有这样中文才画得出来
    loaded = LoadFontEx(path, UI_FONT_SIZE, codepoints, count);
    if (!IsFontValid(loaded)) return 0;

    // 双线性过滤：字放大缩小时边缘更平滑
    SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);

    *font = loaded;
    return 1;
}

/*
 * 功能：关闭窗口前释放界面字体
 * 输入：font 为加载成功的字体
 * 输出：释放字体资源
 * 返回值：无
 */
void unloadUiResources(Font font)//释放内存的函数
{
    if (font.texture.id != 0) UnloadFont(font);
}

/*
 * 功能：判断本帧是否点击按钮
 * 输入：bounds 是按钮矩形区域
 * 输出：无，不绘制按钮
 * 返回值：1 本帧左键在按钮区域内按下，0 没有点击
 */
int isButtonClicked(Rectangle bounds)
{
    // 用 Pressed 而不是 Down：按住不放时 Down 每帧都成立，会连续触发
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 0;

    return CheckCollisionPointRec(GetMousePosition(), bounds) ? 1 : 0;
}

/*
 * 功能：绘制统一样式的按钮
 * 输入：bounds 为区域，text 为 UTF-8 文字，font 为字体
 * 输出：矩形按钮、边框、文字和悬停效果，不触发任何操作
 * 返回值：无
 */
void drawButton(Rectangle bounds, const char text[], Font font)
{
    Vector2 size;
    Vector2 pos;
    int hover;
    Color fill;
    Color border;

    hover = CheckCollisionPointRec(GetMousePosition(), bounds);//CheckCollisionPointRec函数：检查点是否位于矩形内部

    fill = hover ? (Color){ 222, 236, 252, 255 } : (Color){ 246, 248, 252, 255 };//判断出在矩形内部的话进行悬停反馈
    border = hover ? (Color){ 70, 130, 190, 255 } : (Color){ 150, 165, 185, 255 };//边框

    // 先铺底色，再描边
    DrawRectangleRec(bounds, fill);
    DrawRectangleLinesEx(bounds, hover ? 2.0f : 1.0f, border);

    if (text == NULL || font.texture.id == 0) return;

    // 文字居中：先量出文字尺寸，再算偏移
    size = MeasureTextEx(font, text, 20.0f, 1.0f);
    pos.x = bounds.x + (bounds.width - size.x) / 2.0f;
    pos.y = bounds.y + (bounds.height - size.y) / 2.0f;

    DrawTextEx(font, text, pos, 20.0f, 1.0f, (Color){ 30, 40, 55, 255 });//DrawTextEx适合中文，能够指定字体
}

/*
 * 功能：计算界面整体缩放，让绘制和鼠标使用同一套坐标
 * 输入：无，读取当前窗口宽高
 * 输出：无
 * 返回值：等比例缩放且居中的相机
 */
Camera2D getUiCamera(void)//等比放大窗口的函数
{
    Camera2D camera = {0};
    float scaleX = GetScreenWidth() / 800.0f;
    float scaleY = GetScreenHeight() / 600.0f;

    camera.zoom = scaleX < scaleY ? scaleX : scaleY;//取较小的那个 保证两边都能放下
    camera.offset.x = (GetScreenWidth() - 800 * camera.zoom) / 2;//水平居中
    camera.offset.y = (GetScreenHeight() - 600 * camera.zoom) / 2;//垂直居中

    return camera;
}
