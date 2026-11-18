#ifndef GAME_H
#define GAME_H

#include "raylib.h"

/*
 * 功能：加载 12 种正面和卡背，窗口创建后调用一次
 * 输入：无，读取 assets/cards 中 back.png 和 00.png 到 11.png
 * 输出：保存图片；完全没有卡牌资源时采用字符卡牌
 * 返回值：1 可正常游戏，0 图片不完整或加载失败，失败时释放部分资源
 */
int loadGameResources(void);

/*
 * 功能：开始新的一局，重置棋盘、时间和选牌记录
 * 输入：level 为 1 初级、2 中级、3 高级
 * 输出：生成并打乱棋盘，进入亮牌阶段
 * 返回值：无
 */
void startGame(int level);

/*
 * 功能：处理本帧的选牌、盖牌和通关，不等待输入或暂停窗口
 * 输入：seconds 是接收用时的地址，鼠标由 raylib 读取
 * 输出：更新棋盘；只有通关时才写入 *seconds，整秒用时从开局盖牌后计算
 * 返回值：0 继续，1 通关，2 退出或尚未开始
 */
int updateGame(int *seconds);

/*
 * 功能：查询本局配对错误次数，供结算页面显示
 * 输入：无
 * 输出：无，不修改计数
 * 返回值：本局完整选牌后配对失败的次数，新一局从 0 开始
 */
int getGameMistakes(void);

/*
 * 功能：查询本局手动亮牌的使用次数
 * 输入：无
 * 输出：无，不修改计数；开局自动亮牌不计入
 * 返回值：本局作弊次数，新一局从 0 开始
 */
int getGameCheats(void);

/*
 * 功能：绘制棋盘、阶段提示、用时和返回按钮
 * 输入：font 为主函数提供的中文字体
 * 输出：当前画面，不改变棋盘、阶段或计时
 * 返回值：无
 */
void drawGame(Font font);

/*
 * 功能：关闭窗口前释放游戏图片，也用于清理加载失败的部分资源
 * 输入：无
 * 输出：释放并清零纹理，恢复字符卡牌模式
 * 返回值：无
 */
void unloadGameResources(void);

#endif
