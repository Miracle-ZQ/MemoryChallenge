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
 * 输出：默认选择初级，并将自定义配置选择初始化为 4×4、预览 5000 毫秒
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
#define CUSTOM_RANK_FORMULA_VERSION 1
#define CUSTOM_RANK_TOP 5

typedef struct {
    int formulaVersion;  /* 当前为 CUSTOM_RANK_FORMULA_VERSION */
    int rows, cols;      /* 各为 2～6，总牌数为偶数；行列顺序参与分榜 */
    int previewMillis;   /* 1000～30000，按整数毫秒精确区分配置 */
} CustomRankConfig;

/*
 * 功能：提交已通关自定义局的最终积分，不重新计算游戏分数。
 * 输入：name 为 1～31 字节且不含空白的用户名；config 为配置键；
 *       finalScore 为非负整数；elapsedMillis 为非负 64 位整数毫秒。
 * 输出：独立更新 custom_ranks.txt；同配置每人仅留最好成绩、最多五名。
 * 返回：1 更新且落盘成功；0 未刷新或未进前五；-1 参数/读写/格式错误。
 * 约束：分数降序，同分用时升序，再同分保留先入榜者；失败保留原文件。
 */
int saveCustomScore(const char name[], const CustomRankConfig *config,
                    int finalScore, long long elapsedMillis);

/*
 * 功能：打开自定义积分榜，准备各玩家最高分的前五名。
 * 输入：config 为合法配置键，不能为 NULL；高亮玩家沿用 setCurrentPlayer。
 * 输出：选择统一排行榜页的“自定义”选项，汇总已有成绩或记录错误；不提交成绩。
 * 返回：1 页面准备成功（含空榜）；-1 参数/读取/格式错误。
 * 约束：读取失败仍可进入错误提示页并返回；绘制函数不读写文件。
 */
int resetCustomRankPage(const CustomRankConfig *config);

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
