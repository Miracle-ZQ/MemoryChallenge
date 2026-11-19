#ifndef RANK_H
#define RANK_H

#include "raylib.h"

/*
 * 功能：提交指定难度的通关成绩
 * 输入：name 为用户名，level 为 1、2、3，seconds 为非负整秒用时
 * 输出：需要更新时修改排行榜并保存到 ranks.txt
 * 返回值：1 更新且保存成功，0 未刷新个人最好成绩或未进入前五
 *        -1 参数或文件读写错误
 */
int saveScore(const char name[], int level, int seconds);

/*
 * 功能：记录当前玩家名，用于在榜单里高亮自己那一行
 * 输入：name 为用户名
 * 输出：保存用户名
 * 返回值：无
 */
void setCurrentPlayer(const char name[]);

/*
 * 功能：准备排行榜页面
 * 输入：无
 * 输出：默认选择初级，并读取自定义积分榜
 * 返回值：无
 */
void resetRankPage(void);

/*
 * 功能：处理初级、中级、高级、自定义四个选项和返回按钮
 * 输入：无，通过本帧鼠标操作
 * 输出：修改当前显示的难度
 * 返回值：0 留在排行榜，1 返回玩家菜单
 */
int updateRankPage(void);

/*
 * 功能：绘制四个排行榜选项和当前选项的榜单内容
 * 输入：font 为加载成功的中文字体
 * 输出：名次、用户名、用时，或暂无成绩、读取失败提示
 * 返回值：无
 */
void drawRankPage(Font font);


/* 自定义积分榜接口 v1，已在 rank.c 实现。
 * 由 main.c 的 PAGE_RANK 统一调度；顶部依次为初级、中级、高级、自定义。
 */
#define CUSTOM_RANK_TOP 5

/*
 * 功能：提交自定义最终积分，每人保留最高分，取前五名
 * 输入：name 为已登录用户名，finalScore 为通关最终积分
 * 输出：需要更新时保存到 custom_ranks.txt
 * 返回值：1 保存成功，0 未刷新或未入榜，-1 文件错误
 */
int saveCustomScore(const char name[], int finalScore);

/*
 * 功能：打开统一排行榜的自定义选项
 * 输入：无
 * 输出：读取用户名与最高分
 * 返回值：1 读取成功，-1 文件错误
 */
int resetCustomRankPage(void);

/* 功能：处理自定义选项的返回操作；由 updateRankPage 分派调用。
 * 输入：无；返回值：0 留在页面，1 返回玩家菜单。
 * 输出：不改变游戏配置或提交成绩。
 */
int updateCustomRankPage(void);

/* 功能：绘制自定义选项内的榜单内容；由 drawRankPage 分派调用。
 * 输入：共用字体；返回值：无。
 * 输出：前五名（名次/用户名/分数）、空榜或读取错误提示。
 * 约束：只绘制；不加载文件，不保存，不重复创建字体资源。
 */
void drawCustomRankPage(Font font);

#endif
