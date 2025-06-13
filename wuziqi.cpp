#include <stdio.h>
#include <stdlib.h>

#define SIZE 15  

char board[SIZE][SIZE];

char currentPlayer = 'X';

void initBoard() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = ' ';
}

void printBoard() {
    printf("   ");
    for (int i = 0; i < SIZE; i++) {
        printf("%2d", i);
    }
    printf("\n");

    for (int i = 0; i < SIZE; i++) {
        printf("%2d ", i); 
        for (int j = 0; j < SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

int isValidMove(int row, int col) {
    if (row < 0 || row >= SIZE || col < 0 || col >= SIZE)
        return 0;
    if (board[row][col] != ' ')
        return 0;
    return 1;
}

int makeMove(int row, int col) {
    if (!isValidMove(row, col)) {
        printf("无效的位置，请重试。\n");
        return 0;
    }
    board[row][col] = currentPlayer;
    return 1;
}

void switchPlayer() {
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
}

int checkWin(int row, int col) {
    int directions[4][2] = {
        {0, 1}, {1, 0}, {1, 1}, {1, -1} };

    for (int d = 0; d < 4; d++) {
        int count = 1;
        int dr = directions[d][0];
        int dc = directions[d][1];

        for (int i = 1; i < 5; i++) {
            int r = row + i * dr;
            int c = col + i * dc;
            if (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == currentPlayer)
                count++;
            else
                break;
        }

        for (int i = 1; i < 5; i++) {
            int r = row - i * dr;
            int c = col - i * dc;
            if (r >= 0 && r < SIZE && c >= 0 && c < SIZE && board[r][c] == currentPlayer)
                count++;
            else
                break;
        }

        if (count >= 5)
            return 1;
    }

    return 0;
}

int main() {
    int row, col;
    initBoard();

    while (1) {
        printBoard();
        printf("当前玩家 %c，请输入落子位置 (行 列): ", currentPlayer);
        scanf("%d %d", &row, &col);

        if (makeMove(row, col)) {
            if (checkWin(row, col)) {
                printBoard();
                printf("玩家 %c 获胜！\n", currentPlayer);
                break;
            }
            switchPlayer();
        }
    }

    return 0;
}

