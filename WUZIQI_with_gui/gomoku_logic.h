#pragma once
#ifndef GOMOKU_LOGIC_H
#define GOMOKU_LOGIC_H
#define _CRT_SECURE_NO_WARNINGS 
#include <stdbool.h>

/// ���������̳ߴ�
#define GOMOKU_SIZE 15

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * ��ʼ��/������Ϸ״̬��
	 * ��Ӧԭ console �����е� initBoard()��
	 */
	void gomoku_init(void);

	/**
	 * ���ӽӿڡ�
	 * row, col: 0 <= row,col < GOMOKU_SIZE
	 * ���� true ��ʾ���ӳɹ����ø�Ϊ�����ڷ�Χ�ڣ���false ��ʾ�Ƿ�λ�á�
	 * ��Ӧԭ console �� makeMove() �ĺ��ļ�����¼���ܣ������л���ҡ�
	 */
	bool gomoku_make_move(int row, int col);

	/**
	 * �������������
	 * ��� move_count < 2 �򷵻� false�������κ��޸ģ�����ɾ��������������� true��
	 * ��Ӧԭ console �� undoMoves() �Ĺ��ܣ������ڴ��л���ң�GUI ���ڵ��ú���� gomoku_switch_player() �л�������
	 */
	bool gomoku_undo_moves(void);

	/**
	 * ���浱ǰ�Ծֵ��ļ� filename��
	 * ���� true ��ʾ�ɹ���false ��ʾ�򿪻�д��ʧ�ܡ�
	 * ��Ӧԭ console saveGame()��
	 */
	bool gomoku_save(const char* filename);

	/**
	 * ���ļ� filename ���ضԾ֡�
	 * ���� true ��ʾ�ɹ����ز��ָ�״̬��false ��ʾ��ʧ�ܻ��ʽ����
	 * ��Ӧԭ console loadGame()���ڲ����������ٰ���ʷ���ӡ�
	 */
	bool gomoku_load(const char* filename);

	/**
	 * ����� row,col Ϊ���һ�����Ӻ��Ƿ�ʤ������������
	 * ���� true ��ʾʤ����false ��ʾδʤ����
	 * ͬʱ������ڲ�ʤ��������飨winMarks�����Ա� GUI ���� gomoku_get_win_marks() ������
	 * ��Ӧԭ console �� checkWin() ���߼���
	 */
	bool gomoku_check_win(int row, int col);

	/**
	 * �л���ǰ��ң�'X' <-> 'O'��
	 * ��Ӧԭ console �� switchPlayer() ������л����֣��������� GUI ��ʾ�߼���
	 */
	void gomoku_switch_player(void);

	/**
	 * ��ȡ��ǰ��ң����� 'X' �� 'O'��
	 * GUI ������ʾ����ǰ���: %c��ʱʹ�á�
	 */
	char gomoku_current_player(void);

	/**
	 * ��⵱ǰ�����µ���в�㣨����������ڸÿ�λ���ӻ��γ��������ģ���
	 * �ڲ����¶�ά���� threatMarks��
	 * ��Ӧԭ console detectThreats()��
	 */
	void gomoku_detect_threats(void);

	/**
	 * ��ȡ��в�������ָ�룬���������� const int (*)[GOMOKU_SIZE]��
	 * GUI ���ڵ��� gomoku_detect_threats() ��ʹ�ô˽ӿڶ�ȡ����Ը�����в��:
	 *   const int (*marks)[GOMOKU_SIZE] = gomoku_get_threat_marks();
	 *   if (marks[r][c]) { ... }
	 */
	const int (*gomoku_get_threat_marks(void))[GOMOKU_SIZE];

	/**
	 * ����� row,col Ϊ���ĵ�ʤ�����ߣ����У���
	 * �����ڲ� winMarks ���飬��������ֵ��
	 * ͨ�������Ӻ�� load ���һ������ã��� GUI ��Ҫ�ֶ����ʱ���á�
	 * ��Ӧԭ console markWin()��
	 */
	void gomoku_mark_win(int row, int col);

	/**
	 * ��ȡʤ���������ָ�룬���������� const int (*)[GOMOKU_SIZE]��
	 * GUI �ɸ��ݴ˽ӿڸ���ʤ������:
	 *   const int (*w)[GOMOKU_SIZE] = gomoku_get_win_marks();
	 *   if (w[r][c]) { ... }
	 */
	const int (*gomoku_get_win_marks(void))[GOMOKU_SIZE];

	/**
	 * ��ȡ��ǰ�����Ӳ�������ʷ���ȣ���
	 * GUI �������ж��Ƿ�ɳ���(move_count>=2)�����̳��ȡ���һ��λ�õȡ�
	 */
	int gomoku_move_count(void);

	/**
	 * ��ȡ�� index (0 <= index < gomoku_move_count()) ��������λ�ã�ͨ��������� row,col ���ء�
	 * ��Ӧԭ moveHistory ���ʣ�moveHistory[index][0], moveHistory[index][1]��
	 * ��� index Խ�磬�������޸� *row, *col�����÷�Ӧ���߽磩��
	 */
	void gomoku_get_move(int index, int* row, int* col);

	/**
	 * ��ȡ����ָ����λ״̬������ 'X'��'O' �� ' ' (�ո�)��
	 * GUI ��������ʱʹ�á�
	 */
	char gomoku_board_cell(int row, int col);

	/**
	 * ��ͷ��ʾ�����ú��ڲ���¼��һ������л����������ʱ������ʾ��
	 * ��Ӧԭ console touchOpponent()��
	 */
	void gomoku_touch_opponent(void);

	/**
	 * ��鵱ǰ�Ƿ�Ӧ����ʾ��ͷ��ʾ�����ڲ���� touchedFlag �ҵ�ǰ����� touchedPlayer ��ͬ��
	 * ���� true ��ʾ��Ҫ��ʾ��GUI �ڼ�⵽��Ӧ���� gomoku_clear_touch() �����ǣ������ظ���
	 */
	bool gomoku_was_touched(void);

	/**
	 * �����ͷ��ʾ��ǡ�ͨ���� GUI ��ʾ��һ����ʾ����á�
	 */
	void gomoku_clear_touch(void);

#ifdef __cplusplus
}
#endif

#endif // GOMOKU_LOGIC_H
