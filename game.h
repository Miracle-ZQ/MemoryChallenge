#ifndef GAME_H
#define GAME_H

#include "raylib.h"

/*
 * 功能：加载 36 种正面和卡背，窗口创建后调用一次
 * 输入：无，读取 assets/cards 中 back.png 和 00.png 到 35.png
 * 输出：保存卡牌正面和卡背纹理
 * 返回值：无
 */
void loadGameResources(void);

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
 * 返回值：0 继续，1 通关，2 退出本局
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



// 自定义模式的开局配置，行列决定牌数，每种图案固定两张
typedef struct {
    int rows;  // 棋盘行数，范围为 2～6
    int cols;  // 棋盘列数，范围为 2～6，与行数相乘必须为偶数
    int previewSeconds;  // 开局亮牌秒数，范围为 1～30
    double k;  // 耗时曲线系数
} CustomGameConfig;

// 自定义通关成绩，保存统计、倍率和最终积分，便于结算页逐项展示
typedef struct {
    int G;  // 本局总配对组数，等于总牌数除以 2
    int R;  // 已正确配对的组数，通关时等于 G
    int W;  // 配对错误次数，每次选满两张且图案不同计一次
    int H;  // 主动亮牌次数，不包含开局自动亮牌
    double S;  // 基础积分 (100R - 10W) / G
    double t;  // 正式用时，单位为秒，不包含开局展示
    double B_plus;  // 棋盘倍率 B，由总配对组数决定
    double T;
    double T0;
    double Tref;
    double K;
    double F_H;
    double B_minus;  // B-
    int finalScore;
} CustomGameResult;

// 根据配置与当前统计计算积分明细，不修改游戏状态
void calculateCustomResult(const CustomGameConfig *cfg, int correct,
    int wrong, int reveals, double elapsed, CustomGameResult *out);

/*
 * 功能：检查自定义棋盘的总牌数是否为偶数
 * 输入：cfg 为待检查的配置地址
 * 输出：无，不修改传入配置
 * 返回值：1 总牌数为偶数，0 总牌数为奇数
 */
int isEven(const CustomGameConfig *cfg);

/*
 * 功能：按自定义配置开始两两配对的新局
 * 输入：cfg 为已校验的棋盘行列数、亮牌秒数与耗时曲线系数
 * 输出：生成并打乱棋盘，清零计数和时间，进入开局亮牌阶段
 * 返回值：无
 */
void startCustomGame(const CustomGameConfig *cfg);

/*
 * 功能：读取自定义模式通关时保存的结算结果
 * 输入：out 为接收成绩的结构体地址，仅在自定义局通关后调用
 * 输出：写入 *out，不重新计算或修改成绩
 * 返回值：无
 */
void getCustomGameResult(CustomGameResult *out);

/*
 * 功能：查询当前局是否采用自定义规则
 * 输入：无
 * 输出：无，不修改游戏状态
 * 返回值：1 自定义挑战，0 普通模式
 */
int isCustomGame(void);

/*
 * 功能：释放卡牌正面和背面纹理，在关闭窗口前调用一次
 * 输入：无，使用本模块已加载的纹理
 * 输出：释放卡牌正面和背面纹理
 * 返回值：无
 */
void unloadGameResources(void);

#endif
