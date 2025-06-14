#define _CRT_SECURE_NO_WARNINGS

#include <SDL.h>
#include <SDL_ttf.h>
#undef main
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "gomoku_logic.h"  // 棋局逻辑接口

// 窗口和棋盘相关常量
#define BOARD_SIZE 15
#define BOARD_PIXELS 640
#define INFO_HEIGHT 80
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT (BOARD_PIXELS + INFO_HEIGHT)
#define CELL_SIZE (BOARD_PIXELS / BOARD_SIZE)
#define MAX_MOVES (BOARD_SIZE * BOARD_SIZE)

typedef enum { STATE_MENU, STATE_PLAY, STATE_REVIEW } AppState;  // 应用状态：菜单、游戏中、回放模式

typedef struct {
    SDL_Rect rect;
    char label[32];
    void (*onClick)(void);
} Button;  // 简单按钮结构

// 全局变量
SDL_Window* window;
SDL_Renderer* renderer;
TTF_Font* font;
AppState appState;
int reviewStep;         // 当前回放步数
int savedMoveCount;     // 保存的历史步数
int winFlag;
char messageBuffer[256];
Uint32 messageStart;
int messageDuration;

Button menuButtons[4];
Button gameButtons[5];
Button winButtons[3];
Button reviewButtons[4];  // Prev, Next, Restart, Exit Review

// 存储回放用的历史棋步
int reviewMoves[MAX_MOVES][2];

// 函数原型
void startGame(void);
void loadGame(void);
void aiBattle(void);
void exitGame(void);
void undoMove(void);
void saveGame(void);
void loadGamePlay(void);
void hintMove(void);
void replayGame(void);
void restartGame(void);

// 回放相关原型
void setupReviewButtons(void);
void reviewPrev(void);
void reviewNext(void);
void reviewRestart(void);
void reviewExit(void);
void applyReviewSteps(void);

// 在底部信息区显示短暂消息
void showMessage(const char* msg, int duration) {
    strncpy_s(messageBuffer, sizeof(messageBuffer), msg, _TRUNCATE);
    messageStart = SDL_GetTicks();
    messageDuration = duration * 1000;
}

// 绘制按钮：背景、边框、文本居中
void drawButton(const Button* btn) {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &btn->rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btn->rect);
    SDL_Color color = { 0, 0, 0, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, btn->label, color);
    if (!surf) {
        SDL_Log("TTF_RenderText_Blended error: %s", TTF_GetError());
        return;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst;
    dst.w = surf->w;
    dst.h = surf->h;
    dst.x = btn->rect.x + (btn->rect.w - dst.w) / 2;
    dst.y = btn->rect.y + (btn->rect.h - dst.h) / 2;
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

// ----------------- 回调函数 -----------------
// 开始新游戏：初始化棋局，切换到游戏状态
void startGame(void) {
    gomoku_init();
    appState = STATE_PLAY;
    winFlag = 0;
    SDL_Log("Started game, state PLAY");
}

// 从文件加载游戏并进入游戏
void loadGame(void) {
    if (gomoku_load("save.txt") != 0) {
        showMessage("Game loaded", 2);
        appState = STATE_PLAY;
        winFlag = 0;
        SDL_Log("Loaded game, state PLAY");
    }
    else {
        showMessage("Load failed", 2);
        SDL_Log("Load failed");
    }
}

// AI对战（未实现）
void aiBattle(void) {
    showMessage("AI Battle not implemented", 2);
}

// 退出应用
void exitGame(void) {
    SDL_Event ev;
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

// 撤销一步：调用逻辑层并切换玩家
void undoMove(void) {
    if (gomoku_undo_moves()) {
        gomoku_switch_player();
        winFlag = 0;
        SDL_Log("Undo move, switched player");
    }
}

// 保存游戏到文件
void saveGame(void) {
    if (gomoku_save("save.txt") != 0) {
        showMessage("Game saved", 2);
        SDL_Log("Game saved to save.txt");
    }
    else {
        showMessage("Save failed", 2);
        SDL_Log("Save failed");
    }
}

// 从文件加载并进入游戏
void loadGamePlay(void) {
    if (gomoku_load("save.txt") != 0) {
        showMessage("Game loaded", 2);
        appState = STATE_PLAY;
        winFlag = 0;
        SDL_Log("Loaded game for play");
    }
    else {
        showMessage("Load failed", 2);
        SDL_Log("Load failed in play");
    }
}

// 提示功能：逻辑层标记威胁并显示消息
void hintMove(void) {
    gomoku_detect_threats();
    showMessage("Hint displayed", 2);
}

// 进入回放模式：读取历史步数，保存到 reviewMoves，进入 STATE_REVIEW
void replayGame(void) {
    savedMoveCount = gomoku_move_count();
    SDL_Log("Entering review: savedMoveCount=%d", savedMoveCount);
    if (savedMoveCount > 0 && savedMoveCount <= MAX_MOVES) {
        for (int i = 0; i < savedMoveCount; ++i) {
            int r, c;
            gomoku_get_move(i, &r, &c);
            reviewMoves[i][0] = r;
            reviewMoves[i][1] = c;
            SDL_Log("Stored move %d: (%d,%d)", i, r, c);
        }
        appState = STATE_REVIEW;
        reviewStep = 0;
        winFlag = 0;
        applyReviewSteps();
        showMessage("Review mode: step 0", 2);
    }
    else {
        showMessage("No moves to replay", 2);
        SDL_Log("No moves to replay or too many moves");
    }
}

// 重新开始当前对局（仅在游戏中）
void restartGame(void) {
    gomoku_init();
    winFlag = 0;
    SDL_Log("Game restarted");
}

// ----------------- 回放相关函数 -----------------
// 初始化回放按钮，包含 Prev, Next, Restart, Exit Review
void setupReviewButtons(void) {
    const char* labels[4] = { "Prev", "Next", "Restart", "Exit" };
    void (*cbs[4])(void) = { reviewPrev, reviewNext, reviewRestart, reviewExit };
    int bw = 90, bh = 35;
    for (int i = 0; i < 4; ++i) {
        reviewButtons[i].rect.x = 10 + i * (bw + 10);
        reviewButtons[i].rect.y = BOARD_PIXELS + 20;
        reviewButtons[i].rect.w = bw;
        reviewButtons[i].rect.h = bh;
        strncpy_s(reviewButtons[i].label, sizeof(reviewButtons[i].label), labels[i], _TRUNCATE);
        reviewButtons[i].onClick = cbs[i];
        SDL_Log("Setup review button: %s at (%d,%d)", labels[i], reviewButtons[i].rect.x, reviewButtons[i].rect.y);
    }
}

// 根据 reviewStep 重放前 reviewStep 步棋，黑白交替
void applyReviewSteps(void) {
    gomoku_init();  // 重置棋盘
    SDL_Log("Applying review steps: reviewStep=%d", reviewStep);
    for (int i = 0; i < reviewStep; ++i) {
        int r = reviewMoves[i][0];
        int c = reviewMoves[i][1];
        if (!gomoku_make_move(r, c)) {
            SDL_Log("Failed to apply stored move %d: (%d,%d)", i, r, c);
            break;
        }
        gomoku_switch_player();  // 交替落子
    }
}

// 回放上一步
void reviewPrev(void) {
    SDL_Log("Clicked Prev: current reviewStep=%d", reviewStep);
    if (reviewStep > 0) {
        reviewStep--;
        applyReviewSteps();
        char buf[64];
        snprintf(buf, sizeof(buf), "Review: step %d", reviewStep);
        showMessage(buf, 2);
    }
    else {
        showMessage("Already at first step", 2);
        SDL_Log("Already at first review step");
    }
}

// 回放下一步
void reviewNext(void) {
    SDL_Log("Clicked Next: current reviewStep=%d, savedMoveCount=%d", reviewStep, savedMoveCount);
    if (reviewStep < savedMoveCount) {
        reviewStep++;
        applyReviewSteps();
        char buf[64];
        snprintf(buf, sizeof(buf), "Review: step %d", reviewStep);
        showMessage(buf, 2);
    }
    else {
        showMessage("Already at last step", 2);
        SDL_Log("Already at last review step");
    }
}

// 回放重置到第0步
void reviewRestart(void) {
    SDL_Log("Clicked Restart in review");
    reviewStep = 0;
    applyReviewSteps();
    showMessage("Review restarted", 2);
}

// 退出回放，返回到菜单
void reviewExit(void) {
    SDL_Log("Clicked Exit Review");
    appState = STATE_MENU;
    showMessage("Exited review mode", 2);
}

// ----------------- 按钮设置 -----------------
void setupMenuButtons(void) {
    const char* labels[4] = { "Start Game", "Load Game", "AI Battle", "Exit" };
    void (*cbs[4])(void) = { startGame, loadGame, aiBattle, exitGame };
    int w = 180, h = 50;
    int x0 = (WINDOW_WIDTH - w) / 2;
    for (int i = 0; i < 4; ++i) {
        menuButtons[i].rect.x = x0;
        menuButtons[i].rect.y = 150 + i * (h + 20);
        menuButtons[i].rect.w = w;
        menuButtons[i].rect.h = h;
        strncpy_s(menuButtons[i].label, sizeof(menuButtons[i].label), labels[i], _TRUNCATE);
        menuButtons[i].onClick = cbs[i];
    }
}

void setupGameButtons(void) {
    const char* labels[5] = { "Undo", "Save", "Load", "Hint", "Exit" };
    void (*cbs[5])(void) = { undoMove, saveGame, loadGamePlay, hintMove, exitGame };
    int bw = 90, bh = 35;
    for (int i = 0; i < 5; ++i) {
        gameButtons[i].rect.x = 10 + i * (bw + 10);
        gameButtons[i].rect.y = BOARD_PIXELS + 20;
        gameButtons[i].rect.w = bw;
        gameButtons[i].rect.h = bh;
        strncpy_s(gameButtons[i].label, sizeof(gameButtons[i].label), labels[i], _TRUNCATE);
        gameButtons[i].onClick = cbs[i];
    }
}

void setupWinButtons(void) {
    const char* labels[3] = { "Undo", "Replay", "Restart" };
    void (*cbs[3])(void) = { undoMove, replayGame, restartGame };
    int bw = 90, bh = 35;
    for (int i = 0; i < 3; ++i) {
        winButtons[i].rect.x = 10 + i * (bw + 10);
        winButtons[i].rect.y = BOARD_PIXELS + 20;
        winButtons[i].rect.w = bw;
        winButtons[i].rect.h = bh;
        strncpy_s(winButtons[i].label, sizeof(winButtons[i].label), labels[i], _TRUNCATE);
        winButtons[i].onClick = cbs[i];
    }
}

// ----------------- 渲染函数 -----------------
// 显示主菜单
void showMenu(void) {
    SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);
    SDL_RenderClear(renderer);
    for (int i = 0; i < 4; ++i) {
        drawButton(&menuButtons[i]);
    }
    SDL_RenderPresent(renderer);
}

// 绘制棋盘和UI
void drawBoard(bool reviewMode) {
    // 背景色
    SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);
    SDL_RenderClear(renderer);

    // 逻辑层检测威胁、高亮胜利
    gomoku_detect_threats();
    const int (*winM)[BOARD_SIZE] = gomoku_get_win_marks();
    const int (*thM)[BOARD_SIZE] = gomoku_get_threat_marks();

    // 绘制格子和棋子
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            SDL_Rect cell = { j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE };
            if (winM && winM[i][j]) {
                SDL_SetRenderDrawColor(renderer, 144, 238, 144, 255);
                SDL_RenderFillRect(renderer, &cell);
            }
            else if (!reviewMode && thM && thM[i][j]) {
                SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                SDL_RenderFillRect(renderer, &cell);
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &cell);
            char ch = gomoku_board_cell(i, j);
            if (ch == 'X' || ch == 'O') {
                SDL_Rect p = { cell.x + 4, cell.y + 4, CELL_SIZE - 8, CELL_SIZE - 8 };
                if (ch == 'X') SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &p);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &p);
            }
        }
    }

    // 高亮最后一步
    int moves = gomoku_move_count();
    if (!reviewMode && moves > 0) {
        int lr, lc;
        gomoku_get_move(moves - 1, &lr, &lc);
        SDL_Rect last = { lc * CELL_SIZE, lr * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int i = 0; i < 3; ++i) {
            SDL_RenderDrawRect(renderer, &last);
        }
    }

    // 信息区背景
    SDL_Rect info = { 0, BOARD_PIXELS, WINDOW_WIDTH, INFO_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255);
    SDL_RenderFillRect(renderer, &info);
    // 显示当前玩家
    char buf[128];
    snprintf(buf, sizeof(buf), "Turn: %c", gomoku_current_player());
    SDL_Color tc = { 0,0,0,255 };
    SDL_Surface* bs = TTF_RenderText_Blended(font, buf, tc);
    if (bs) {
        SDL_Texture* bt = SDL_CreateTextureFromSurface(renderer, bs);
        SDL_Rect bd = { 5, BOARD_PIXELS + 5, bs->w, bs->h };
        SDL_RenderCopy(renderer, bt, NULL, &bd);
        SDL_FreeSurface(bs);
        SDL_DestroyTexture(bt);
    }

    // 绘制按钮
    if (appState == STATE_REVIEW) {
        for (int i = 0; i < 4; ++i) drawButton(&reviewButtons[i]);
    }
    else if (winFlag) {
        for (int i = 0; i < 3; ++i) drawButton(&winButtons[i]);
    }
    else {
        for (int i = 0; i < 5; ++i) drawButton(&gameButtons[i]);
    }

    // 渲染短消息
    if (messageBuffer[0] != '\0') {
        Uint32 now = SDL_GetTicks();
        if (now - messageStart < (Uint32)messageDuration) {
            SDL_Surface* msurf = TTF_RenderText_Blended(font, messageBuffer, tc);
            if (msurf) {
                SDL_Texture* mtex = SDL_CreateTextureFromSurface(renderer, msurf);
                SDL_Rect mrect = { 5, BOARD_PIXELS + 30, msurf->w, msurf->h };
                SDL_RenderCopy(renderer, mtex, NULL, &mrect);
                SDL_FreeSurface(msurf);
                SDL_DestroyTexture(mtex);
            }
        }
        else {
            messageBuffer[0] = '\0';
        }
    }

    SDL_RenderPresent(renderer);
}

// 事件处理：根据状态处理鼠标和键盘事件
void handlePlayEvent(SDL_Event* e) {
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        int mx = e->button.x;
        int my = e->button.y;
        SDL_Point pt = { mx, my };
        if (appState == STATE_PLAY) {
            // 游戏中点击棋盘落子或点击按钮
            if (!winFlag && my < BOARD_PIXELS) {
                int row = my / CELL_SIZE;
                int col = mx / CELL_SIZE;
                if (gomoku_make_move(row, col)) {
                    if (gomoku_check_win(row, col)) {
                        winFlag = 1;
                        showMessage("Game Over", 3);
                        SDL_Log("Game Over detected");
                    }
                    else {
                        gomoku_switch_player();
                    }
                }
            }
            else {
                // 点击游戏按钮或胜利后的按钮
                if (winFlag) {
                    for (int i = 0; i < 3; ++i) {
                        if (SDL_PointInRect(&pt, &winButtons[i].rect)) {
                            winButtons[i].onClick();
                        }
                    }
                }
                else {
                    for (int i = 0; i < 5; ++i) {
                        if (SDL_PointInRect(&pt, &gameButtons[i].rect)) {
                            gameButtons[i].onClick();
                        }
                    }
                }
            }
        }
        else if (appState == STATE_REVIEW) {
            // 回放模式下点击按钮
            SDL_Log("Mouse click at (%d,%d) in REVIEW", mx, my);
            for (int i = 0; i < 4; ++i) {
                if (SDL_PointInRect(&pt, &reviewButtons[i].rect)) {
                    reviewButtons[i].onClick();
                }
            }
        }
    }
    else if (e->type == SDL_KEYDOWN) {
        if (appState == STATE_REVIEW) {
            // 键盘也支持回放控制: 左/右/重启/退出
            if (e->key.keysym.sym == SDLK_LEFT) {
                reviewPrev();
            }
            else if (e->key.keysym.sym == SDLK_RIGHT) {
                reviewNext();
            }
            else if (e->key.keysym.sym == SDLK_r) {
                reviewRestart();
            }
            else if (e->key.keysym.sym == SDLK_e) {
                reviewExit();
            }
        }
    }
    // SDL_QUIT 在主循环处理
}

int main(int argc, char* argv[]) {
    // 初始化SDL和TTF
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Gomoku", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("msyh.ttc", 18);
    if (!font) SDL_Log("Failed to open font: %s", TTF_GetError());

    // 初始化状态
    appState = STATE_MENU;
    reviewStep = 0;
    savedMoveCount = 0;
    winFlag = 0;
    messageBuffer[0] = '\0';

    // 设置按钮
    setupMenuButtons();
    setupGameButtons();
    setupWinButtons();
    setupReviewButtons(); 

    bool running = true;
    SDL_Event e;
    while (running) {
        if (appState == STATE_MENU) {
            // 显示菜单并处理点击
            showMenu();
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    SDL_Point pt = { e.button.x, e.button.y };
                    for (int i = 0; i < 4; ++i) {
                        if (SDL_PointInRect(&pt, &menuButtons[i].rect)) {
                            menuButtons[i].onClick();
                        }
                    }
                }
                else if (e.type == SDL_QUIT) {
                    running = false;
                }
            }
        }
        else {
            // 游戏或回放状态: 处理事件并绘制
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    running = false;
                }
                handlePlayEvent(&e);
            }
            drawBoard(appState == STATE_REVIEW);
            SDL_Delay(16);
        }
    }

    // 清理
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
