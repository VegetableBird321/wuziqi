#define _CRT_SECURE_NO_WARNINGS  // 禁用 MSVC 关于安全函数的警告
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>  // 用于方向键和 _getch()

#define SIZE 15
#define MAX_MOVES (SIZE * SIZE)

char board[SIZE][SIZE];
char currentPlayer;
int moveHistory[MAX_MOVES][2];
int moveCount;
int touchedFlag;
char touchedPlayer;
int cursorRow, cursorCol;
int winMarks[SIZE][SIZE];
int threatMarks[SIZE][SIZE];
int inKeyboardMode = 0; 

void initBoard() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = ' ';
    moveCount = 0;
    currentPlayer = 'X';
    touchedFlag = 0;
    touchedPlayer = '\0';
    cursorRow = SIZE / 2;
    cursorCol = SIZE / 2;
    memset(winMarks, 0, sizeof(winMarks));
    memset(threatMarks, 0, sizeof(threatMarks));
    inKeyboardMode = 0;
}


void detectThreats() {
    memset(threatMarks, 0, sizeof(threatMarks));
    char opponent = (currentPlayer == 'X') ? 'O' : 'X';
    int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] != ' ') {
                threatMarks[i][j] = 0;
                continue;
            }
            // 临时放置对手棋子
            board[i][j] = opponent;
            int isThreat = 0;
            for (int d = 0; d < 4; d++) {
                int dr = dirs[d][0], dc = dirs[d][1];
                int lenPos = 0, lenNeg = 0;
                for (int k = 1; k < 5; k++) {
                    int r = i + dr * k, c = j + dc * k;
                    if (r < 0 || r >= SIZE || c < 0 || c >= SIZE || board[r][c] != opponent) break;
                    lenPos++;
                }
                for (int k = 1; k < 5; k++) {
                    int r = i - dr * k, c = j - dc * k;
                    if (r < 0 || r >= SIZE || c < 0 || c >= SIZE || board[r][c] != opponent) break;
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
                    int open1 = (end1_r >= 0 && end1_r < SIZE && end1_c >= 0 && end1_c < SIZE && board[end1_r][end1_c] == ' ');
                    int open2 = (end2_r >= 0 && end2_r < SIZE && end2_c >= 0 && end2_c < SIZE && board[end2_r][end2_c] == ' ');
                    if (open1 && open2) {
                        isThreat = 1;
                        break;
                    }
                }
            }
            threatMarks[i][j] = isThreat;
            board[i][j] = ' '; 
        }
    }
}

void markWin(int row, int col) {
    memset(winMarks, 0, sizeof(winMarks));
    int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };

    for (int d = 0; d < 4; d++) {
        int dr = dirs[d][0], dc = dirs[d][1];
        int count = 1;
        int r = row + dr, c = col + dc;
        while (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == currentPlayer) { count++; r += dr; c += dc; }
        r = row - dr; c = col - dc;
        while (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == currentPlayer) { count++; r -= dr; c -= dc; }
        if (count >= 5) {
            winMarks[row][col] = 1;
            for (int k = 1; k < 5; k++) {
                int rr = row + dr * k, cc = col + dc * k;
                if (rr < 0 || rr >= SIZE || cc < 0 || cc >= SIZE) break;
                if (board[rr][cc] == currentPlayer) winMarks[rr][cc] = 1;
            }
            for (int k = 1; k < 5; k++) {
                int rr = row - dr * k, cc = col - dc * k;
                if (rr < 0 || rr >= SIZE || cc < 0 || cc >= SIZE) break;
                if (board[rr][cc] == currentPlayer) winMarks[rr][cc] = 1;
            }
        }
    }
}

void printBoard() {
    detectThreats();
    printf("   "); for (int i = 0; i < SIZE; i++) printf("%2d", i); printf("\n");
    for (int i = 0; i < SIZE; i++) {
        printf("%2d ", i);
        for (int j = 0; j < SIZE; j++) {
            char ch = board[i][j];
            int isCursor = inKeyboardMode && (i == cursorRow && j == cursorCol);
            int isLast = (moveCount > 0 && moveHistory[moveCount - 1][0] == i && moveHistory[moveCount - 1][1] == j);
            int color = 0; 
            if (winMarks[i][j]) color = 2;
            else if (isLast) color = 1;
            else if (isCursor) color = 4;
            else if (threatMarks[i][j]) color = 3;
            switch (color) {
            case 1: printf("\x1b[33m %c \x1b[0m", ch); break;  // 黄色字
            case 2: printf("\x1b[42m %c \x1b[0m", ch); break;  // 绿色底
            case 3: printf("\x1b[44m %c \x1b[0m", ch); break;  // 蓝色底
            case 4: printf("\x1b[31m[%c]\x1b[0m", ch == ' ' ? ' ' : ch); break;  // 红
            default: printf(" %c ", ch);
            }
        }
        printf("\n");
    }
}

int isValidMove(int r, int c) {
    return r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == ' ';
}

int makeMove(int r, int c) {
    if (!isValidMove(r, c)) { printf("无效位置。\n"); return 0; }
    board[r][c] = currentPlayer;
    moveHistory[moveCount][0] = r;
    moveHistory[moveCount][1] = c;
    moveCount++;
    markWin(r, c);
    return 1;
}

void undoMoves() {
    if (moveCount < 2) { printf("步数不足，无法撤销。\n"); return; }
    for (int i = 0; i < 2; i++) {
        moveCount--;
        int r = moveHistory[moveCount][0], c = moveHistory[moveCount][1];
        board[r][c] = ' ';
    }
    memset(winMarks, 0, sizeof(winMarks));
    printf("已撤销最近的两步。\n");
}

void switchPlayer() {
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    if (touchedFlag && currentPlayer == touchedPlayer) {
        printf("对手摸了摸你的头~\n", currentPlayer);
        touchedFlag = 0;
    }
    if (moveCount > 0) {
        cursorRow = moveHistory[moveCount - 1][0];
        cursorCol = moveHistory[moveCount - 1][1];
    }
    else {
        cursorRow = SIZE / 2; cursorCol = SIZE / 2;
    }
}

int checkWin(int r, int c) {
    markWin(r, c);
    for (int i = 0; i < SIZE; i++) for (int j = 0; j < SIZE; j++) if (winMarks[i][j]) return 1;
    return 0;
}

void saveGame(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) { printf("无法保存文件。\n"); return; }
    fprintf(fp, "%d\n", moveCount);
    for (int i = 0; i < moveCount; i++) fprintf(fp, "%d %d\n", moveHistory[i][0], moveHistory[i][1]);
    fclose(fp);
    printf("已保存至 %s\n", filename);
}

void loadGame(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) { printf("无法打开文件。\n"); return; }
    initBoard();
    fscanf(fp, "%d", &moveCount);
    for (int i = 0; i < moveCount; i++) {
        int r, c; fscanf(fp, "%d %d", &r, &c);
        board[r][c] = (i % 2 == 0) ? 'X' : 'O';
        moveHistory[i][0] = r; moveHistory[i][1] = c;
    }
    currentPlayer = (moveCount % 2 == 0) ? 'X' : 'O';
    if (moveCount > 0) { markWin(moveHistory[moveCount - 1][0], moveHistory[moveCount - 1][1]); cursorRow = moveHistory[moveCount - 1][0]; cursorCol = moveHistory[moveCount - 1][1]; }
    fclose(fp);
    printf("已从 %s 加载，对弈继续...\n", filename);
}

void touchOpponent() {
    touchedFlag = 1; touchedPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    printf("你轻轻地摸了摸玩家 %c 的头。\n", touchedPlayer);
}

void printHelp() {
    printf("\n可用命令：\n");
    printf("  undo       - 悔棋\n");
    printf("  save       - 保存棋局\n");
    printf("  load       - 读取并继续\n");
    printf("  touch      - 摸摸头\n");
    printf("  m          - 方向键模式落子\n");
    printf("  exit       - 退出当前对局或程序\n");
    printf("  help       - 显示帮助菜单\n");
    printf("  [x y]      - 直接落子坐标 (如 7 7)\n\n");
}

void reviewGame() {
    if (moveCount == 0) { printf("暂无对战记录。\n"); return; }
    char savedBoard[SIZE][SIZE]; memcpy(savedBoard, board, sizeof(board));
    char savedPlayer = currentPlayer; int savedCount = moveCount;
    initBoard();
    printf("开始复盘：共 %d 步\n", savedCount); 
    getchar();
    for (int i = 0; i < savedCount; i++) {
        int r = moveHistory[i][0], c = moveHistory[i][1];
        char p = (i % 2 == 0) ? 'X' : 'O'; board[r][c] = p; printBoard();
        printf("第 %d 步 [%c](%d,%d)\n", i + 1, p, r, c);
        printf("按 Enter 继续..."); getchar();
    }
    memcpy(board, savedBoard, sizeof(board)); 
    moveCount = savedCount;
    currentPlayer = savedPlayer;
    markWin(moveHistory[moveCount - 1][0], moveHistory[moveCount - 1][1]);
    printf("复盘结束，回到对局\n");
}

void keyboardModeInput() {
    inKeyboardMode = 1;
    while (1) {
        system("cls"); printBoard(); printf("方向键移动，Enter落子，ESC退出模式\n");
        int ch = _getch();
        if (ch == 224) {
            ch = _getch();
            switch (ch) {
            case 72: if (cursorRow > 0) cursorRow--; break;
            case 80: if (cursorRow < SIZE - 1) cursorRow++; break;
            case 75: if (cursorCol > 0) cursorCol--; break;
            case 77: if (cursorCol < SIZE - 1) cursorCol++; break;
            }
        }
        else if (ch == 13) {
            if (makeMove(cursorRow, cursorCol)) {
                if (checkWin(cursorRow, cursorCol)) {
                    printBoard();
                    printf("玩家 %c 获胜！\n", currentPlayer);
                    inKeyboardMode = 0;
                    return;
                }
                switchPlayer();
            }
        }
        else if (ch == 27) {
            inKeyboardMode = 0;
            break;
        }
    }
}

void runGame() {
    char input[32]; int gameOver = 0;
    while (!gameOver) {
        printBoard(); printf("当前玩家 %c，请输入指令，help查看指令帮助: ", currentPlayer); scanf("%s", input);
        if (!strcmp(input, "undo")) undoMoves(); else if (!strcmp(input, "save")) saveGame("save.txt");
        else if (!strcmp(input, "load")) loadGame("save.txt"); else if (!strcmp(input, "touch")) touchOpponent();
        else if (!strcmp(input, "help")) printHelp(); else if (!strcmp(input, "m")) keyboardModeInput();
        else if (!strcmp(input, "exit")) { printf("退出本局。\n"); return; }
        else if (!strcmp(input, "review")) { printf("游戏未结束，无法复盘。\n"); }
        else { int r = atoi(input), c; scanf("%d", &c); if (makeMove(r, c)) { if (checkWin(r, c)) { printBoard(); printf("玩家 %c 获胜！\n", currentPlayer); gameOver = 1; } else switchPlayer(); } }
    }
    while (1) {
        printf("\n对局结束。输入 'review' 复盘或 'exit' 返回主菜单："); scanf("%s", input);
        if (!strcmp(input, "review")) reviewGame(); else if (!strcmp(input, "exit")) break;
    }
}

int main() {
    int choice; initBoard();
    while (1) {
        printf("\n===== 五子棋主菜单 =====\n"); printf("1. 玩家对战\n2. AI 对战（敬请期待）\n3. 加载对局\n4. 退出\n");
        printf("请选择(1-4)："); scanf("%d", &choice);
        switch (choice) {
        case 1: initBoard(); runGame(); break;
        case 2: printf("AI 对战功能敬请期待！\n"); break;
        case 3: loadGame("save.txt"); runGame(); break;
        case 4: exit(0);
        default: printf("无效选择。\n");
        }
    }
    return 0;
}
