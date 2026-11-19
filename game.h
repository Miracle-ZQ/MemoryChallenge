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
    double previewSeconds;  // 开局自动亮牌的持续时间，范围为 1～30 秒
} CustomGameConfig;

// 自定义通关成绩，保存统计、倍率和最终积分，便于结算页逐项展示
typedef struct {
    int groups;  // 本局总配对组数，等于总牌数除以 2
    int correct;  // 已正确配对的组数，通关时等于 groups
    int wrong;  // 配对错误次数，每次选满两张且图案不同计一次
    int reveals;  // 主动亮牌次数，不包含开局自动亮牌
    int baseScore;  // 基础积分，正确组数乘 100 减去错误次数乘 40
    double elapsedSeconds;  // 正式用时，单位为秒，不包含开局展示
    double boardMultiplier;  // 棋盘倍率 B，由总配对组数决定
    double previewMultiplier;  // 开局倍率 P，由开局展示时间决定
    double plusMultiplier;  // 正确奖励倍率，等于 B 乘 P
    double minusMultiplier;  // 错配扣分倍率，由 2-P 限制到规定范围得到
    double revealFactor;  // 主动亮牌后的剩余比例，等于 0.97 的 reveals 次方
    double timePenalty;  // 耗时扣分比例，以每组 5 秒为基准，最多为 0.30
    int finalScore;  // 最终非负整数积分，所有倍率应用后只四舍五入一次
} CustomGameResult;

/*
 * 功能：检查自定义棋盘和开局展示时间是否合法
 * 输入：cfg 为待检查的配置地址
 * 输出：无，不修改传入配置
 * 返回值：1 合法，0 配置为空、行列越界、牌数为奇数或展示时间不在 1～30 秒
 */
int validateCustomConfig(const CustomGameConfig *cfg);

/*
 * 功能：按自定义配置开始两两配对的新局
 * 输入：cfg 为棋盘行列数和开局展示秒数
 * 输出：生成并打乱棋盘，清零计数和时间，进入开局亮牌阶段
 * 返回值：1 启动成功，0 配置非法；失败时不改变旧局
 */
int startCustomGame(const CustomGameConfig *cfg);

/*
 * 功能：读取自定义模式通关时保存的结算结果
 * 输入：out 为接收成绩的结构体地址
 * 输出：成功时写入 *out，不重新计算或修改成绩
 * 返回值：1 读取成功，0 地址为空、不是自定义局或尚无有效通关结果
 */
int getCustomGameResult(CustomGameResult *out);

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
 * 输出：释放成功加载的纹理，跳过未加载的资源
 * 返回值：无
 */
void unloadGameResources(void);

#endif
