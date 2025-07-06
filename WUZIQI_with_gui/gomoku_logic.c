#define _CRT_SECURE_NO_WARNINGS
#include "gomoku_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GOMOKU_SIZE 15     /* 如已在头文件定义，可删除 */
#define MAX_DEPTH 2        /* 搜索深度，可根据性能调整 */

/* ---------- 棋盘数据 ---------- */
static char board_internal[GOMOKU_SIZE][GOMOKU_SIZE];
static int  moveHistory_internal[GOMOKU_SIZE * GOMOKU_SIZE][2];
static int  moveCount_internal;
static char currentPlayer_internal;
static int  winMarks_internal[GOMOKU_SIZE][GOMOKU_SIZE];
static int  threatMarks_internal[GOMOKU_SIZE][GOMOKU_SIZE];
static bool touchedFlag_internal;
static char touchedPlayer_internal;
#define IN_BOARD(r,c) ((r) >= 0 && (r) < GOMOKU_SIZE && \
                       (c) >= 0 && (c) < GOMOKU_SIZE)

/* 统计连续同色子长度与活口数 */
static void count_segment(int r, int c, int dr, int dc,
    char me, int* len, int* openEnds)
{
    *len = 1;
    *openEnds = 0;

    int rr = r + dr, cc = c + dc;
    while (IN_BOARD(rr, cc) && board_internal[rr][cc] == me) {
        (*len)++;  rr += dr;  cc += dc;
    }
    if (IN_BOARD(rr, cc) && board_internal[rr][cc] == ' ') (*openEnds)++;
    rr = r - dr;  cc = c - dc;
    while (IN_BOARD(rr, cc) && board_internal[rr][cc] == me) {
        (*len)++;  rr -= dr;  cc -= dc;
    }
    if (IN_BOARD(rr, cc) && board_internal[rr][cc] == ' ') (*openEnds)++;
}

/* 基础接口 */
static bool is_valid_move_internal(int r, int c) {
    return IN_BOARD(r, c) && board_internal[r][c] == ' ';
}

void gomoku_init(void) {
    for (int i = 0; i < GOMOKU_SIZE; i++)
        for (int j = 0; j < GOMOKU_SIZE; j++) {
            board_internal[i][j] = ' ';
            winMarks_internal[i][j] = 0;
            threatMarks_internal[i][j] = 0;
        }
    moveCount_internal = 0;
    currentPlayer_internal = 'X';
    touchedFlag_internal = false;
    touchedPlayer_internal = '\0';
}

bool gomoku_make_move(int row, int col) {
    if (!is_valid_move_internal(row, col)) return false;
    board_internal[row][col] = currentPlayer_internal;
    moveHistory_internal[moveCount_internal][0] = row;
    moveHistory_internal[moveCount_internal][1] = col;
    moveCount_internal++;
    gomoku_mark_win(row, col);
    return true;
}

bool gomoku_undo_moves(void) {
    if (moveCount_internal < 2) return false;
    for (int k = 0; k < 2; ++k) {
        moveCount_internal--;
        int r = moveHistory_internal[moveCount_internal][0];
        int c = moveHistory_internal[moveCount_internal][1];
        board_internal[r][c] = ' ';
    }
    memset(winMarks_internal, 0, sizeof(winMarks_internal));
    return true;
}

// 文件保存 / 载入 
bool gomoku_save(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return false;
    if (fprintf(fp, "%d\n", moveCount_internal) < 0) { fclose(fp); return false; }
    for (int i = 0; i < moveCount_internal; ++i) {
        if (fprintf(fp, "%d %d\n",
            moveHistory_internal[i][0], moveHistory_internal[i][1]) < 0) {
            fclose(fp); return false;
        }
    }
    fclose(fp);
    return true;
}

bool gomoku_load(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return false;
    gomoku_init();
    int cnt;
    if (fscanf(fp, "%d", &cnt) != 1 || cnt < 0 || cnt > GOMOKU_SIZE * GOMOKU_SIZE) {
        fclose(fp); return false;
    }
    for (int i = 0; i < cnt; ++i) {
        int r, c;
        if (fscanf(fp, "%d %d", &r, &c) != 2 || !IN_BOARD(r, c)) { fclose(fp); return false; }
        board_internal[r][c] = (i % 2 == 0) ? 'X' : 'O';
        moveHistory_internal[i][0] = r;
        moveHistory_internal[i][1] = c;
    }
    moveCount_internal = cnt;
    currentPlayer_internal = (cnt % 2 == 0) ? 'X' : 'O';
    if (cnt) {
        int lr = moveHistory_internal[cnt - 1][0];
        int lc = moveHistory_internal[cnt - 1][1];
        gomoku_mark_win(lr, lc);
    }
    fclose(fp);
    return true;
}

//胜负判定 & 标记 
void gomoku_mark_win(int row, int col) {
    memset(winMarks_internal, 0, sizeof(winMarks_internal));
    const int dirs[4][2] = { {0,1},{1,0},{1,1},{1,-1} };
    for (int d = 0; d < 4; ++d) {
        int dr = dirs[d][0], dc = dirs[d][1];
        int count = 1;
        /* 正方向 */
        int r = row + dr, c = col + dc;
        while (IN_BOARD(r, c) && board_internal[r][c] == currentPlayer_internal) { count++; r += dr; c += dc; }
        /* 反方向 */
        r = row - dr; c = col - dc;
        while (IN_BOARD(r, c) && board_internal[r][c] == currentPlayer_internal) { count++; r -= dr; c -= dc; }

        if (count >= 5) {
            winMarks_internal[row][col] = 1;
            for (int k = 1; k < 5; ++k) {
                int rr = row + dr * k, cc = col + dc * k;
                if (!IN_BOARD(rr, cc) || board_internal[rr][cc] != currentPlayer_internal) break;
                winMarks_internal[rr][cc] = 1;
            }
            for (int k = 1; k < 5; ++k) {
                int rr = row - dr * k, cc = col - dc * k;
                if (!IN_BOARD(rr, cc) || board_internal[rr][cc] != currentPlayer_internal) break;
                winMarks_internal[rr][cc] = 1;
            }
        }
    }
}

bool gomoku_check_win(int row, int col) {
    gomoku_mark_win(row, col);
    for (int i = 0; i < GOMOKU_SIZE; ++i)
        for (int j = 0; j < GOMOKU_SIZE; ++j)
            if (winMarks_internal[i][j]) return true;
    return false;
}

/* ---------- 玩家切换 ---------- */
void gomoku_switch_player(void) { currentPlayer_internal = (currentPlayer_internal == 'X') ? 'O' : 'X'; }
char gomoku_current_player(void) { return currentPlayer_internal; }

/* ---------- 威胁探测（保留原实现） ---------- */
void gomoku_detect_threats(void) {
    memset(threatMarks_internal, 0, sizeof(threatMarks_internal));
    char opponent = (currentPlayer_internal == 'X') ? 'O' : 'X';
    const int dirs[4][2] = { {0,1},{1,0},{1,1},{1,-1} };

    for (int i = 0; i < GOMOKU_SIZE; ++i) {
        for (int j = 0; j < GOMOKU_SIZE; ++j) {
            if (board_internal[i][j] != ' ') continue;
            board_internal[i][j] = opponent;

            int isThreat = 0;
            for (int d = 0; d < 4 && !isThreat; ++d) {
                int dr = dirs[d][0], dc = dirs[d][1], lenPos = 0, lenNeg = 0;

                for (int k = 1; k < 5; ++k) {
                    int r = i + dr * k, c = j + dc * k;
                    if (!IN_BOARD(r, c) || board_internal[r][c] != opponent) break; lenPos++;
                }
                for (int k = 1; k < 5; ++k) {
                    int r = i - dr * k, c = j - dc * k;
                    if (!IN_BOARD(r, c) || board_internal[r][c] != opponent) break; lenNeg++;
                }

                int cnt = 1 + lenPos + lenNeg;
                if (cnt >= 5) isThreat = 1;
                else if (cnt == 4) {
                    int r1 = i + dr * (lenPos + 1), c1 = j + dc * (lenPos + 1);
                    int r2 = i - dr * (lenNeg + 1), c2 = j - dc * (lenNeg + 1);
                    int open1 = IN_BOARD(r1, c1) && board_internal[r1][c1] == ' ';
                    int open2 = IN_BOARD(r2, c2) && board_internal[r2][c2] == ' ';
                    if (open1 && open2) isThreat = 1;
                }
            }
            threatMarks_internal[i][j] = isThreat;
            board_internal[i][j] = ' ';
        }
    }
}
const int (*gomoku_get_threat_marks(void))[GOMOKU_SIZE] { return threatMarks_internal; }
const int (*gomoku_get_win_marks(void))[GOMOKU_SIZE] { return winMarks_internal; }
int  gomoku_move_count(void) { return moveCount_internal; }
void gomoku_get_move(int idx, int* r, int* c) {
    if (idx < 0 || idx >= moveCount_internal) return;
    if (r) *r = moveHistory_internal[idx][0];
    if (c) *c = moveHistory_internal[idx][1];
}
char gomoku_board_cell(int r, int c) {
    return IN_BOARD(r, c) ? board_internal[r][c] : ' ';
}

/* ---------- 摸头 ---------- */
void gomoku_touch_opponent(void) {
    touchedFlag_internal = true;
    touchedPlayer_internal = (currentPlayer_internal == 'X') ? 'O' : 'X';
}
bool gomoku_was_touched(void) {
    return touchedFlag_internal && currentPlayer_internal == touchedPlayer_internal;
}
void gomoku_clear_touch(void) {
    touchedFlag_internal = false; touchedPlayer_internal = '\0';
}


static int evaluate_board(char aiPlayer)
{
    const char opp = (aiPlayer == 'X') ? 'O' : 'X';
    const int dirs[4][2] = { {0,1},{1,0},{1,1},{1,-1} };
    int score = 0;

    for (int r = 0; r < GOMOKU_SIZE; ++r) {
        for (int c = 0; c < GOMOKU_SIZE; ++c) {
            char p = board_internal[r][c];
            if (p == ' ') continue;

            for (int d = 0; d < 4; ++d) {
                int pr = r - dirs[d][0], pc = c - dirs[d][1];
                if (IN_BOARD(pr, pc) && board_internal[pr][pc] == p) continue;

                int len = 0, openEnds = 0;
                count_segment(r, c, dirs[d][0], dirs[d][1], p, &len, &openEnds);

                int segScore = 0;
                if (len >= 5)  segScore = 100000;
                else if (len == 4 && openEnds == 2)    segScore = 10000;   
                else if (len == 4 && openEnds == 1)    segScore = 1000;    
                else if (len == 3 && openEnds == 2)    segScore = 100;
                else if (len == 3 && openEnds == 1)    segScore = 30;
                else if (len == 2 && openEnds == 2)    segScore = 10;
                else if (len == 2 && openEnds == 1)    segScore = 3;

                if (p == aiPlayer) score += segScore;
                else             score -= segScore;
            }
        }
    }
    return score;
}


static int minimax(int depth, bool maximizing, char aiPlayer)
{
    if (depth == 0 || moveCount_internal >= GOMOKU_SIZE * GOMOKU_SIZE)
        return evaluate_board(aiPlayer);

    int best = maximizing ? -1000000 : 1000000;
    char me = maximizing ? aiPlayer : ((aiPlayer == 'X') ? 'O' : 'X');

    for (int r = 0; r < GOMOKU_SIZE; ++r) {
        for (int c = 0; c < GOMOKU_SIZE; ++c) {
            if (board_internal[r][c] != ' ') continue;

            board_internal[r][c] = me;
            moveHistory_internal[moveCount_internal][0] = r;
            moveHistory_internal[moveCount_internal][1] = c;
            moveCount_internal++;

            int s = minimax(depth - 1, !maximizing, aiPlayer);

            board_internal[r][c] = ' ';
            moveCount_internal--;

            if (maximizing) { if (s > best) best = s; }
            else { if (s < best) best = s; }
        }
    }
    return best;
}

void gomoku_get_ai_move(int* bestRow, int* bestCol)
{
    if (moveCount_internal == 0) {
        *bestRow = GOMOKU_SIZE / 2;
        *bestCol = GOMOKU_SIZE / 2;
        return;
    }

    if (moveCount_internal == 1) {
        int pr = moveHistory_internal[0][0];
        int pc = moveHistory_internal[0][1];
        const int delta[8][2] = {
            {-1,  0}, {1,  0}, {0, -1}, {0,  1},
            {-1, -1}, {-1, 1}, {1, -1}, {1,  1}
        };

        int cand[8][2]; int cnt = 0;
        for (int k = 0; k < 8; ++k) {
            int r = pr + delta[k][0];
            int c = pc + delta[k][1];
            if (IN_BOARD(r, c) && board_internal[r][c] == ' ') {
                cand[cnt][0] = r;
                cand[cnt][1] = c;
                cnt++;
            }
        }

        if (cnt > 0) {
            int idx = rand() % cnt;
            *bestRow = cand[idx][0];
            *bestCol = cand[idx][1];
        }
        else {
            *bestRow = GOMOKU_SIZE / 2;
            *bestCol = GOMOKU_SIZE / 2;
        }
        return;
    }

    int bestScore = -1000000;
    int candCnt = 0, cand[GOMOKU_SIZE * GOMOKU_SIZE][2];
    char aiPlayer = currentPlayer_internal;

    for (int r = 0; r < GOMOKU_SIZE; ++r) {
        for (int c = 0; c < GOMOKU_SIZE; ++c) {
            if (board_internal[r][c] != ' ') continue;

            board_internal[r][c] = aiPlayer;
            moveHistory_internal[moveCount_internal][0] = r;
            moveHistory_internal[moveCount_internal][1] = c;
            moveCount_internal++;

            int s = minimax(MAX_DEPTH - 1, false, aiPlayer);

            board_internal[r][c] = ' ';
            moveCount_internal--;

            if (s > bestScore) {
                bestScore = s;
                candCnt = 0;
                cand[candCnt][0] = r; cand[candCnt][1] = c; candCnt++;
            }
            else if (s == bestScore) {
                cand[candCnt][0] = r; cand[candCnt][1] = c; candCnt++;
            }
        }
    }

    if (candCnt) {
        int idx = rand() % candCnt;
        *bestRow = cand[idx][0];
        *bestCol = cand[idx][1];
    }
    else {
        *bestRow = GOMOKU_SIZE / 2;
        *bestCol = GOMOKU_SIZE / 2;
    }
}
