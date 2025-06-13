#pragma once
#ifndef GOMOKU_LOGIC_H
#define GOMOKU_LOGIC_H
#define _CRT_SECURE_NO_WARNINGS 
#include <stdbool.h>

/// 五子棋棋盘尺寸
#define GOMOKU_SIZE 15

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * 初始化/重置游戏状态。
	 * 对应原 console 程序中的 initBoard()。
	 */
	void gomoku_init(void);

	/**
	 * 落子接口。
	 * row, col: 0 <= row,col < GOMOKU_SIZE
	 * 返回 true 表示落子成功（该格为空且在范围内），false 表示非法位置。
	 * 对应原 console 中 makeMove() 的核心检查与记录功能，但不切换玩家。
	 */
	bool gomoku_make_move(int row, int col);

	/**
	 * 撤销最近两步。
	 * 如果 move_count < 2 则返回 false，不做任何修改；否则删除最近两步并返回 true。
	 * 对应原 console 中 undoMoves() 的功能，但不在此切换玩家，GUI 可在调用后调用 gomoku_switch_player() 切换回来。
	 */
	bool gomoku_undo_moves(void);

	/**
	 * 保存当前对局到文件 filename。
	 * 返回 true 表示成功，false 表示打开或写入失败。
	 * 对应原 console saveGame()。
	 */
	bool gomoku_save(const char* filename);

	/**
	 * 从文件 filename 加载对局。
	 * 返回 true 表示成功加载并恢复状态，false 表示打开失败或格式错误。
	 * 对应原 console loadGame()，内部会先重置再按历史落子。
	 */
	bool gomoku_load(const char* filename);

	/**
	 * 检查以 row,col 为最近一手落子后是否胜利（五连）。
	 * 返回 true 表示胜利，false 表示未胜利。
	 * 同时会更新内部胜利标记数组（winMarks），以便 GUI 调用 gomoku_get_win_marks() 高亮。
	 * 对应原 console 中 checkWin() 的逻辑。
	 */
	bool gomoku_check_win(int row, int col);

	/**
	 * 切换当前玩家：'X' <-> 'O'。
	 * 对应原 console 中 switchPlayer() 中玩家切换部分，但不触发 GUI 提示逻辑。
	 */
	void gomoku_switch_player(void);

	/**
	 * 获取当前玩家，返回 'X' 或 'O'。
	 * GUI 绘制提示“当前玩家: %c”时使用。
	 */
	char gomoku_current_player(void);

	/**
	 * 检测当前局面下的威胁点（即如果对手在该空位落子会形成连五或活四）。
	 * 内部更新二维数组 threatMarks。
	 * 对应原 console detectThreats()。
	 */
	void gomoku_detect_threats(void);

	/**
	 * 获取威胁标记数组指针，返回类型是 const int (*)[GOMOKU_SIZE]。
	 * GUI 可在调用 gomoku_detect_threats() 后，使用此接口读取标记以高亮威胁点:
	 *   const int (*marks)[GOMOKU_SIZE] = gomoku_get_threat_marks();
	 *   if (marks[r][c]) { ... }
	 */
	const int (*gomoku_get_threat_marks(void))[GOMOKU_SIZE];

	/**
	 * 标记以 row,col 为中心的胜利连线（若有）。
	 * 更新内部 winMarks 数组，但不返回值。
	 * 通常在落子后或 load 最后一步后调用，或 GUI 需要手动标记时调用。
	 * 对应原 console markWin()。
	 */
	void gomoku_mark_win(int row, int col);

	/**
	 * 获取胜利标记数组指针，返回类型是 const int (*)[GOMOKU_SIZE]。
	 * GUI 可根据此接口高亮胜利连线:
	 *   const int (*w)[GOMOKU_SIZE] = gomoku_get_win_marks();
	 *   if (w[r][c]) { ... }
	 */
	const int (*gomoku_get_win_marks(void))[GOMOKU_SIZE];

	/**
	 * 获取当前已落子步数（历史长度）。
	 * GUI 可用于判断是否可撤销(move_count>=2)、复盘长度、上一步位置等。
	 */
	int gomoku_move_count(void);

	/**
	 * 获取第 index (0 <= index < gomoku_move_count()) 步的落子位置，通过输出参数 row,col 返回。
	 * 对应原 moveHistory 访问：moveHistory[index][0], moveHistory[index][1]。
	 * 如果 index 越界，函数不修改 *row, *col（调用方应检查边界）。
	 */
	void gomoku_get_move(int index, int* row, int* col);

	/**
	 * 获取棋盘指定格位状态，返回 'X'、'O' 或 ' ' (空格)。
	 * GUI 绘制棋子时使用。
	 */
	char gomoku_board_cell(int row, int col);

	/**
	 * 摸头提示：调用后，内部记录下一个玩家切换到被摸玩家时，有提示。
	 * 对应原 console touchOpponent()。
	 */
	void gomoku_touch_opponent(void);

	/**
	 * 检查当前是否应该显示摸头提示：即内部标记 touchedFlag 且当前玩家与 touchedPlayer 相同。
	 * 返回 true 表示需要显示，GUI 在检测到后应调用 gomoku_clear_touch() 清除标记，避免重复。
	 */
	bool gomoku_was_touched(void);

	/**
	 * 清除摸头提示标记。通常在 GUI 显示完一次提示后调用。
	 */
	void gomoku_clear_touch(void);

#ifdef __cplusplus
}
#endif

#endif // GOMOKU_LOGIC_H
