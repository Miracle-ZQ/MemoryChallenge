#ifndef UI_H
#define UI_H

#include "raylib.h"

/*
 * 功能：窗口创建后加载界面中文字体，缺少组内文件时尝试 Windows 黑体
 * 输入：font 是接收字体的变量地址
 * 输出：成功时将字体写入 *font，失败时清理本次已加载的资源
 * 返回值：1 成功，0 失败
 */
int loadUiResources(Font *font);

/*
 * 功能：关闭窗口前释放界面字体
 * 输入：font 为加载成功的字体
 * 输出：释放字体资源
 * 返回值：无
 */
void unloadUiResources(Font font);

/*
 * 功能：判断本帧是否点击按钮
 * 输入：bounds 是按钮矩形区域
 * 输出：无，不绘制按钮
 * 返回值：1 本帧左键在按钮区域内按下，0 没有点击
 */
int isButtonClicked(Rectangle bounds);

/*
 * 功能：绘制统一样式的按钮
 * 输入：bounds 为区域，text 为 UTF-8 文字，font 为字体
 * 输出：矩形按钮、边框、文字和悬停效果，不触发任何操作
 * 返回值：无
 */
void drawButton(Rectangle bounds, const char text[], Font font);

/*
 * 功能：计算原 800x600 布局到当前窗口的等比例放大和居中位置
 * 输入：无，读取当前窗口尺寸
 * 输出：无，不改变鼠标或绘制状态
 * 返回值：供主画面和输入框裁剪共同使用的二维相机
 */
Camera2D getUiCamera(void);

/*
 * 功能：加载三张页面背景图（登录 / 游戏 / 排行榜）
 * 输入：无，从 assets/bg/ 读取
 * 输出：纹理加载到模块内部
 * 返回值：1 成功
 */
int loadBackgrounds(void);

/*
 * 功能：把指定背景铺满整个画面
 * 输入：index 0=登录 1=游戏 2=排行榜
 * 输出：拉伸到 800x600 绘制
 * 返回值：无
 */
void drawBackground(int index);

/*
 * 功能：关闭窗口前释放背景图
 * 输入：无
 * 输出：释放纹理
 * 返回值：无
 */
void unloadBackgrounds(void);

#endif
