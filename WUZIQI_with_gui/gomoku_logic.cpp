#include "gomoku_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _CRT_SECURE_NO_WARNINGS 
static char board_internal[GOMOKU_SIZE][GOMOKU_SIZE];
static int moveHistory_internal[GOMOKU_SIZE * GOMOKU_SIZE][2];
static int moveCount_internal;
static char currentPlayer_internal;
static int winMarks_internal[GOMOKU_SIZE][GOMOKU_SIZE];
static int threatMarks_internal[GOMOKU_SIZE][GOMOKU_SIZE];
static bool touchedFlag_internal;
static char touchedPlayer_internal;


static bool is_valid_move_internal(int r, int c) {
    return (r >= 0 && r < GOMOKU_SIZE && c >= 0 && c < GOMOKU_SIZE && board_internal[r][c] == ' ');
}


void gomoku_init(void) {
    for (int i = 0; i < GOMOKU_SIZE; i++) {
        for (int j = 0; j < GOMOKU_SIZE; j++) {
            board_internal[i][j] = ' ';
            winMarks_internal[i][j] = 0;
            threatMarks_internal[i][j] = 0;
        }
    }
    moveCount_internal = 0;
    currentPlayer_internal = 'X';
    touchedFlag_internal = false;
    touchedPlayer_internal = '\0';
}


bool gomoku_make_move(int row, int col) {
    if (!is_valid_move_internal(row, col)) {
        return false;
    }
    board_internal[row][col] = currentPlayer_internal;
    moveHistory_internal[moveCount_internal][0] = row;
    moveHistory_internal[moveCount_internal][1] = col;
    moveCount_internal++;
    gomoku_mark_win(row, col);
    return true;
}


bool gomoku_undo_moves(void) {
    if (moveCount_internal < 2) {
        return false;
    }
    for (int i = 0; i < 2; i++) {
        moveCount_internal--;
        int r = moveHistory_internal[moveCount_internal][0];
        int c = moveHistory_internal[moveCount_internal][1];
        board_internal[r][c] = ' ';
    }
    for (int i = 0; i < GOMOKU_SIZE; i++)
        for (int j = 0; j < GOMOKU_SIZE; j++)
            winMarks_internal[i][j] = 0;
    return true;
}

bool gomoku_save(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return false;
    if (fprintf(fp, "%d\n", moveCount_internal) < 0) {
        fclose(fp);
        return false;
    }
    for (int i = 0; i < moveCount_internal; i++) {
        int r = moveHistory_internal[i][0];
        int c = moveHistory_internal[i][1];
        if (fprintf(fp, "%d %d\n", r, c) < 0) {
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}

bool gomoku_load(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return false;
    gomoku_init();
    int cnt = 0;
    if (fscanf(fp, "%d", &cnt) != 1) {
        fclose(fp);
        return false;
    }
    if (cnt < 0 || cnt > GOMOKU_SIZE * GOMOKU_SIZE) {
        fclose(fp);
        return false;
    }
    for (int i = 0; i < cnt; i++) {
        int r, c;
        if (fscanf(fp, "%d %d", &r, &c) != 2) {
            fclose(fp);
            return false;
        }
        if (r < 0 || r >= GOMOKU_SIZE || c < 0 || c >= GOMOKU_SIZE) {
            fclose(fp);
            return false;
        }
        board_internal[r][c] = (i % 2 == 0) ? 'X' : 'O';
        moveHistory_internal[i][0] = r;
        moveHistory_internal[i][1] = c;
    }
    moveCount_internal = cnt;
    currentPlayer_internal = (moveCount_internal % 2 == 0) ? 'X' : 'O';
    if (moveCount_internal > 0) {
        int lr = moveHistory_internal[moveCount_internal - 1][0];
        int lc = moveHistory_internal[moveCount_internal - 1][1];
        gomoku_mark_win(lr, lc);
    }
    fclose(fp);
    return true;
}


void gomoku_mark_win(int row, int col) {
    for (int i = 0; i < GOMOKU_SIZE; i++)
        for (int j = 0; j < GOMOKU_SIZE; j++)
            winMarks_internal[i][j] = 0;
    int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };
    for (int d = 0; d < 4; d++) {
        int dr = dirs[d][0], dc = dirs[d][1];
        int count = 1;
        int r = row + dr, c = col + dc;
        while (r >= 0 && r < GOMOKU_SIZE && c >= 0 && c < GOMOKU_SIZE && board_internal[r][c] == currentPlayer_internal) {
            count++;
            r += dr; c += dc;
        }
        // 反向统计
        r = row - dr; c = col - dc;
        while (r >= 0 && r < GOMOKU_SIZE && c >= 0 && c < GOMOKU_SIZE && board_internal[r][c] == currentPlayer_internal) {
            count++;
            r -= dr; c -= dc;
        }
        if (count >= 5) {
            winMarks_internal[row][col] = 1;
            for (int k = 1; k < 5; k++) {
                int rr = row + dr * k, cc = col + dc * k;
                if (rr < 0 || rr >= GOMOKU_SIZE || cc < 0 || cc >= GOMOKU_SIZE) break;
                if (board_internal[rr][cc] == currentPlayer_internal) winMarks_internal[rr][cc] = 1;
                else break;
            }
            for (int k = 1; k < 5; k++) {
                int rr = row - dr * k, cc = col - dc * k;
                if (rr < 0 || rr >= GOMOKU_SIZE || cc < 0 || cc >= GOMOKU_SIZE) break;
                if (board_internal[rr][cc] == currentPlayer_internal) winMarks_internal[rr][cc] = 1;
                else break;
            }
        }
    }
}


bool gomoku_check_win(int row, int col) {
    gomoku_mark_win(row, col);
    for (int i = 0; i < GOMOKU_SIZE; i++) {
        for (int j = 0; j < GOMOKU_SIZE; j++) {
            if (winMarks_internal[i][j]) return true;
        }
    }
    return false;
}


void gomoku_switch_player(void) {
    currentPlayer_internal = (currentPlayer_internal == 'X') ? 'O' : 'X';
}


char gomoku_current_player(void) {
    return currentPlayer_internal;
}


void gomoku_detect_threats(void) {
    for (int i = 0; i < GOMOKU_SIZE; i++)
        for (int j = 0; j < GOMOKU_SIZE; j++)
            threatMarks_internal[i][j] = 0;
    char opponent = (currentPlayer_internal == 'X') ? 'O' : 'X';
    int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };
    for (int i = 0; i < GOMOKU_SIZE; i++) {
        for (int j = 0; j < GOMOKU_SIZE; j++) {
            if (board_internal[i][j] != ' ') {
                threatMarks_internal[i][j] = 0;
                continue;
            }
            board_internal[i][j] = opponent;
            int isThreat = 0;
            for (int d = 0; d < 4; d++) {
                int dr = dirs[d][0], dc = dirs[d][1];
                int lenPos = 0, lenNeg = 0;
                for (int k = 1; k < 5; k++) {
                    int r = i + dr * k, c = j + dc * k;
                    if (r < 0 || r >= GOMOKU_SIZE || c < 0 || c >= GOMOKU_SIZE || board_internal[r][c] != opponent) break;
                    lenPos++;
                }
                for (int k = 1; k < 5; k++) {
                    int r = i - dr * k, c = j - dc * k;
                    if (r < 0 || r >= GOMOKU_SIZE || c < 0 || c >= GOMOKU_SIZE || board_internal[r][c] != opponent) break;
                    lenNeg++;
                }
                int count = 1 + lenPos + lenNeg;
                if (count >= 5) {
                    isThreat = 1;
                    break;
                }
                else if (count == 4) {
                    int end1_r = i + dr * (lenPos + 1), end1_c = j + dc * (lenPos + 1);
                    int end2_r = i - dr * (lenNeg + 1), end2_c = j - dc * (lenNeg + 1);
                    int open1 = (end1_r >= 0 && end1_r < GOMOKU_SIZE && end1_c >= 0 && end1_c < GOMOKU_SIZE && board_internal[end1_r][end1_c] == ' ');
                    int open2 = (end2_r >= 0 && end2_r < GOMOKU_SIZE && end2_c >= 0 && end2_c < GOMOKU_SIZE && board_internal[end2_r][end2_c] == ' ');
                    if (open1 && open2) {
                        isThreat = 1;
                        break;
                    }
                }
            }
            threatMarks_internal[i][j] = isThreat;
            board_internal[i][j] = ' ';
        }
    }
}

const int (*gomoku_get_threat_marks(void))[GOMOKU_SIZE] {
    return threatMarks_internal;
}

const int (*gomoku_get_win_marks(void))[GOMOKU_SIZE] {
    return winMarks_internal;
}


int gomoku_move_count(void) {
    return moveCount_internal;
}


void gomoku_get_move(int index, int* row, int* col) {
    if (index < 0 || index >= moveCount_internal) {
        return;
    }
    if (row) *row = moveHistory_internal[index][0];
    if (col) *col = moveHistory_internal[index][1];
}


char gomoku_board_cell(int row, int col) {
    if (row < 0 || row >= GOMOKU_SIZE || col < 0 || col >= GOMOKU_SIZE) return ' ';
    return board_internal[row][col];
}


void gomoku_touch_opponent(void) {
    touchedFlag_internal = true;
    touchedPlayer_internal = (currentPlayer_internal == 'X') ? 'O' : 'X';
}


bool gomoku_was_touched(void) {
    return (touchedFlag_internal && currentPlayer_internal == touchedPlayer_internal);
}


void gomoku_clear_touch(void) {
    touchedFlag_internal = false;
    touchedPlayer_internal = '\0';
}
