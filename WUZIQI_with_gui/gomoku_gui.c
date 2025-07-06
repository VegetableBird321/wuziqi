#define _CRT_SECURE_NO_WARNINGS

#include <SDL.h>
#include <SDL_ttf.h>
#undef main
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "gomoku_logic.h" 

// 颜色
static const SDL_Color COLOR_BLACK = { 0,0,0,255 };

#define BOARD_SIZE     15
#define BOARD_PIXELS   640
#define INFO_HEIGHT    100
#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  (BOARD_PIXELS + INFO_HEIGHT)
#define CELL_SIZE      (BOARD_PIXELS / BOARD_SIZE)
#define MAX_MOVES      (BOARD_SIZE * BOARD_SIZE)

typedef enum { STATE_MENU, STATE_AI_SELECT, STATE_PLAY, STATE_REVIEW } AppState;

typedef struct {
    SDL_Rect rect;
    char     label[32];
    void   (*onClick)(void);
} Button;

// 全局
SDL_Window* window;
SDL_Renderer* renderer;
TTF_Font* font;
AppState      appState = STATE_MENU;
bool          aiEnabled = false;
bool          aiGoesFirst = false;
int           reviewStep = 0;
int           savedMoveCount = 0;
int           winFlag = 0;
char          messageBuffer[256] = "";
Uint32        messageStart = 0;
int           messageDuration = 0;
int           playerWin = 0;

Button menuButtons[4];
Button aiSelectButtons[2];
Button gameButtons[5];
Button winButtons[3];
Button reviewButtons[4];
int    reviewMoves[MAX_MOVES][2];

// 前向声明
void startGame(void);
void aiBattle(void);
void aiStartAI(void);
void aiStartPlayer(void);
void exitGame(void);
void undoMove(void);
void saveGame(void);
void loadGamePlay(void);
void hintMove(void);
void replayGame(void);
void restartGame(void);
void setupMenuButtons(void);
void setupAISelectButtons(void);
void setupGameButtons(void);
void setupWinButtons(void);
void setupReviewButtons(void);
bool handlePlayEvent(SDL_Event* e);
void drawBoard(bool reviewMode);
void applyReviewSteps(void);
void reviewPrev(void);
void reviewNext(void);
void reviewRestart(void);
void reviewExit(void);
void showMessage(const char* msg, int duration);
void drawButton(const Button* btn);

// 显示短消息
void showMessage(const char* msg, int duration) {
    strncpy(messageBuffer, msg, sizeof(messageBuffer) - 1);
    messageBuffer[sizeof(messageBuffer) - 1] = '\0';
    messageStart = SDL_GetTicks();
    messageDuration = duration * 1000;
}

// 画按钮
void drawButton(const Button* btn) {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &btn->rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &btn->rect);
    SDL_Surface* surf = TTF_RenderText_Blended(font, btn->label, COLOR_BLACK);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = {
        btn->rect.x + (btn->rect.w - surf->w) / 2,
        btn->rect.y + (btn->rect.h - surf->h) / 2,
        surf->w, surf->h
    };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

// ---------- 回调 ----------

void startGame(void) {
    gomoku_init();
    appState = STATE_PLAY;
    winFlag = 0;
    aiEnabled = false;
}

void aiBattle(void) {
    appState = STATE_AI_SELECT;
    showMessage("Select who goes first: AI or Player", 2);
}

void aiStartAI(void) {
    startGame();
    aiEnabled = true;
    aiGoesFirst = true;
    int r, c;
    gomoku_get_ai_move(&r, &c);
    gomoku_make_move(r, c);
    gomoku_switch_player();
}

void aiStartPlayer(void) {
    startGame();
    aiEnabled = true;
    aiGoesFirst = false;
}

void exitGame(void) {
    SDL_Event ev; ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

void undoMove(void) {
    if (playerWin == 1)showMessage("Lets just rebattle", 1);
    else {
        
        if (aiEnabled && winFlag) gomoku_switch_player();
        if (gomoku_undo_moves()) {
            winFlag = 0;
            showMessage("Undid last two moves", 1);
        }
    }
}

void saveGame(void) {
    if (gomoku_save("save.txt")) showMessage("Save successful", 2);
    else                         showMessage("Save failed", 2);
}

void loadGamePlay(void) {
    if (gomoku_load("save.txt")) {
        appState = STATE_PLAY;
        winFlag = 0;
        showMessage("Load successful", 2);
    }
    else {
        showMessage("Load failed", 2);
    }
}

void hintMove(void) {
    if (aiEnabled) showMessage("You can't affect my strategy by just doing this", 2);
    else {
        gomoku_touch_opponent();
        showMessage("You rua your opponent", 2);
    }
}

void replayGame(void) {
    // 重新填充 reviewMoves
    savedMoveCount = gomoku_move_count();
    reviewStep = 0;
    if (savedMoveCount > 0 && savedMoveCount <= MAX_MOVES) {
        for (int i = 0; i < savedMoveCount; ++i)
            gomoku_get_move(i, &reviewMoves[i][0], &reviewMoves[i][1]);
        appState = STATE_REVIEW;
        winFlag = 0;
        applyReviewSteps();
        showMessage("Enter review mode", 2);
    }
    else {
        showMessage("No moves to review", 2);
    }
}

void restartGame(void) {
    // 完全重置对局状态
    gomoku_init();
    playerWin = 0;
    winFlag = 0;
    aiEnabled = false;
    aiGoesFirst = false;
    reviewStep = 0;
    savedMoveCount = 0;
    appState = STATE_MENU;
    showMessage("Game restarted", 2);
}

// ---------- 按钮布局 ----------

void setupMenuButtons(void) {
    const char* labels[4] = { "Start","Load","AI Battle","Exit" };
    void (*cbs[4])(void) = { startGame, loadGamePlay, aiBattle, exitGame };
    int w = 180, h = 50, x0 = (WINDOW_WIDTH - w) / 2;
    for (int i = 0; i < 4; ++i) {
        menuButtons[i].rect.x = x0;
        menuButtons[i].rect.y = 150 + i * (h + 20);
        menuButtons[i].rect.w = w;
        menuButtons[i].rect.h = h;
        strncpy(menuButtons[i].label, labels[i], 31);
        menuButtons[i].label[31] = '\0';
        menuButtons[i].onClick = cbs[i];
    }
}

void setupAISelectButtons(void) {
    const char* labels[2] = { "AI First","Player First" };
    void (*cbs[2])(void) = { aiStartAI, aiStartPlayer };
    int w = 200, h = 60, x0 = (WINDOW_WIDTH - w) / 2;
    for (int i = 0; i < 2; ++i) {
        aiSelectButtons[i].rect.x = x0;
        aiSelectButtons[i].rect.y = 150 + i * (h + 20);
        aiSelectButtons[i].rect.w = w;
        aiSelectButtons[i].rect.h = h;
        strncpy(aiSelectButtons[i].label, labels[i], 31);
        aiSelectButtons[i].label[31] = '\0';
        aiSelectButtons[i].onClick = cbs[i];
    }
}

void setupGameButtons(void) {
    const char* labels[5] = { "Undo","Save","Load","Touch","Exit" };
    void (*cbs[5])(void) = { undoMove, saveGame, loadGamePlay, hintMove, exitGame };
    for (int i = 0; i < 5; ++i) {
        gameButtons[i].rect.x = 10 + i * 100;
        gameButtons[i].rect.y = BOARD_PIXELS + 10;
        gameButtons[i].rect.w = 90;
        gameButtons[i].rect.h = 35;
        strncpy(gameButtons[i].label, labels[i], 31);
        gameButtons[i].label[31] = '\0';
        gameButtons[i].onClick = cbs[i];
    }
}

void setupWinButtons(void) {
    const char* labels[3] = { "Undo","Review","Restart" };
    void (*cbs[3])(void) = { undoMove, replayGame, restartGame };
    for (int i = 0; i < 3; ++i) {
        winButtons[i].rect.x = 10 + i * 100;
        winButtons[i].rect.y = BOARD_PIXELS + 10;
        winButtons[i].rect.w = 90;
        winButtons[i].rect.h = 35;
        strncpy(winButtons[i].label, labels[i], 31);
        winButtons[i].label[31] = '\0';
        winButtons[i].onClick = cbs[i];
    }
}

void setupReviewButtons(void) {
    const char* labels[4] = { "Prev","Next","Restart","Exit" };
    void (*cbs[4])(void) = { reviewPrev, reviewNext, reviewRestart, reviewExit };
    for (int i = 0; i < 4; ++i) {
        reviewButtons[i].rect.x = 10 + i * 100;
        reviewButtons[i].rect.y = BOARD_PIXELS + 10;
        reviewButtons[i].rect.w = 90;
        reviewButtons[i].rect.h = 35;
        strncpy(reviewButtons[i].label, labels[i], 31);
        reviewButtons[i].label[31] = '\0';
        reviewButtons[i].onClick = cbs[i];
    }
}

// ---------- 回放逻辑 ----------

void applyReviewSteps(void) {
    gomoku_init();
    for (int i = 0; i < reviewStep; ++i) {
        gomoku_make_move(reviewMoves[i][0], reviewMoves[i][1]);
        gomoku_switch_player();
    }
}

void reviewPrev(void) {
    if (reviewStep > 0) {
        --reviewStep;
        applyReviewSteps();
        showMessage("Review: previous", 1);
    }
    else showMessage("Already first", 1);
}

void reviewNext(void) {
    if (reviewStep < savedMoveCount) {
        ++reviewStep;
        applyReviewSteps();
        showMessage("Review: next", 1);
    }
    else showMessage("Already last", 1);
}

void reviewRestart(void) {
    reviewStep = 0;
    applyReviewSteps();
    showMessage("Review reset", 1);
}

void reviewExit(void) {
    // 退出回放，回到主菜单
    reviewStep = 0;
    savedMoveCount = 0;
    appState = STATE_MENU;
    showMessage("Exit review", 1);
}

// ---------- 事件处理 ----------
bool handlePlayEvent(SDL_Event* e) {
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        SDL_Point pt = { e->button.x, e->button.y };

        // 菜单态
        if (appState == STATE_MENU) {
            for (int i = 0; i < 4; ++i) {
                if (SDL_PointInRect(&pt, &menuButtons[i].rect))
                    menuButtons[i].onClick();
            }
        }
        // 战斗态
        else if (appState == STATE_PLAY) {
            // 点击棋盘区域下子
            if (!winFlag && e->button.y < BOARD_PIXELS) {
                int r = e->button.y / CELL_SIZE;
                int c = e->button.x / CELL_SIZE;
                if (gomoku_make_move(r, c)) {
                    if (gomoku_check_win(r, c)) {
                        winFlag = 1;
                        playerWin = 1;
                        gomoku_switch_player();
                        if (aiEnabled) {
                            char winner = gomoku_current_player();
                            bool isAIWinner = !(winner == (aiGoesFirst ? 'X' : 'O'));

                            if (isAIWinner) {
                                if (aiGoesFirst)
                                    showMessage("I only had to think two moves ahead to beat you.", 3);  // AI 是黑棋赢了
                                else
                                    showMessage("Looks like having the first move didn’t help you much.", 3);  // AI 是白棋赢了
                            }
                            else {
                                if (aiGoesFirst)
                                    showMessage("I’ll be back!", 3);  // 玩家是白棋赢了
                                else
                                    showMessage("Good game. Let me go first next time.", 3);  // 玩家是黑棋赢了
                            }
                        }

                        else {
                            showMessage("You win!", 3);
                        }
                    }
                    else {
                        gomoku_switch_player();
                    }
                }
            }
            // 点击按钮区域
            else {
                if (winFlag) {
                    for (int i = 0; i < 3; ++i) {
                        if (SDL_PointInRect(&pt, &winButtons[i].rect))
                            winButtons[i].onClick();
                    }
                }
                else {
                    for (int i = 0; i < 5; ++i) {
                        if (SDL_PointInRect(&pt, &gameButtons[i].rect))
                            gameButtons[i].onClick();
                    }
                }
            }
        }
        // 回放态
        else if (appState == STATE_REVIEW) {
            for (int i = 0; i < 4; ++i) {
                if (SDL_PointInRect(&pt, &reviewButtons[i].rect))
                    reviewButtons[i].onClick();
            }
        }
        // AI 先手选择态
        else if (appState == STATE_AI_SELECT) {
            for (int i = 0; i < 2; ++i) {
                if (SDL_PointInRect(&pt, &aiSelectButtons[i].rect))
                    aiSelectButtons[i].onClick();
            }
        }
    }
    else if (e->type == SDL_KEYDOWN && appState == STATE_REVIEW) {
        switch (e->key.keysym.sym) {
        case SDLK_LEFT:  reviewPrev();    break;
        case SDLK_RIGHT: reviewNext();    break;
        case SDLK_r:     reviewRestart(); break;
        case SDLK_e:     reviewExit();    break;
        }
    }
    return false;
}

// ---------- 绘制 ----------

void drawBoard(bool reviewMode) {
    // 背景
    SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);
    SDL_RenderClear(renderer);

    gomoku_detect_threats();
    const int (*winM)[BOARD_SIZE] = gomoku_get_win_marks();
    const int (*threatM)[BOARD_SIZE] = gomoku_get_threat_marks();

    // 棋格与棋子
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            SDL_Rect cell = { j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE };
            if (winM && winM[i][j]) {
                SDL_SetRenderDrawColor(renderer, 144, 238, 144, 255);
                SDL_RenderFillRect(renderer, &cell);
            }
            else if (!reviewMode && threatM && threatM[i][j]) {
                SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                SDL_RenderFillRect(renderer, &cell);
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &cell);

            char ch = gomoku_board_cell(i, j);
            if (ch == 'X' || ch == 'O') {
                SDL_Rect p = { cell.x + 4,cell.y + 4, CELL_SIZE - 8, CELL_SIZE - 8 };
                if (ch == 'X') SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                else        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &p);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &p);
            }
        }
    }

    // 高亮最后一步
    int mvCnt = gomoku_move_count();
    if (!reviewMode && mvCnt > 0) {
        int lr, lc;
        gomoku_get_move(mvCnt - 1, &lr, &lc);
        SDL_Rect last = { lc * CELL_SIZE, lr * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int k = 0; k < 3; ++k) SDL_RenderDrawRect(renderer, &last);
    }

    // 信息区
    SDL_Rect infoRect = { 0, BOARD_PIXELS, WINDOW_WIDTH, INFO_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255);
    SDL_RenderFillRect(renderer, &infoRect);

    // 当前玩家
    char turnBuf[32];
    snprintf(turnBuf, sizeof(turnBuf), "Turn: %c", gomoku_current_player());
    SDL_Surface* surf = TTF_RenderText_Blended(font, turnBuf, COLOR_BLACK);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect turnRect = { 5, BOARD_PIXELS + 5, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, NULL, &turnRect);
    SDL_FreeSurface(surf); SDL_DestroyTexture(tex);

    // 按钮
    if (appState == STATE_MENU) {
        for (int i = 0; i < 4; ++i) drawButton(&menuButtons[i]);
    }
    else if (appState == STATE_AI_SELECT) {
        for (int i = 0; i < 2; ++i) drawButton(&aiSelectButtons[i]);
    }
    else if (appState == STATE_REVIEW) {
        for (int i = 0; i < 4; ++i) drawButton(&reviewButtons[i]);
    }
    else { // PLAY
        if (winFlag) {
            for (int i = 0; i < 3; ++i) drawButton(&winButtons[i]);
        }
        else {
            for (int i = 0; i < 5; ++i) drawButton(&gameButtons[i]);
        }
    }

    // 短消息
    if (messageBuffer[0]) {
        Uint32 now = SDL_GetTicks();
        if (now - messageStart < (Uint32)messageDuration) {
            SDL_Surface* mS = TTF_RenderText_Blended(font, messageBuffer, COLOR_BLACK);
            SDL_Texture* mT = SDL_CreateTextureFromSurface(renderer, mS);
            int msgY = BOARD_PIXELS + INFO_HEIGHT - (mS->h + 5);
            SDL_Rect msgRect = { 5, msgY, mS->w, mS->h };
            SDL_RenderCopy(renderer, mT, NULL, &msgRect);
            SDL_FreeSurface(mS); SDL_DestroyTexture(mT);
        }
    }

    SDL_RenderPresent(renderer);
}

// ---------- Main ----------

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Gomoku", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("msyh.ttc", 18);

    setupMenuButtons();
    setupAISelectButtons();
    setupGameButtons();
    setupWinButtons();
    setupReviewButtons();

    bool running = true;
    SDL_Event e;

    while (running) {
        if (appState == STATE_MENU || appState == STATE_AI_SELECT) {
            drawBoard(false);
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) running = false;
                else handlePlayEvent(&e);
            }
        }
        else {
            while (SDL_PollEvent(&e)) {
                if (gomoku_was_touched()) {
                    showMessage("Your opponent just rua d your head",2); 
                    gomoku_clear_touch();         
                }
                if (e.type == SDL_QUIT) running = false;
                else handlePlayEvent(&e);
            }
            // AI 回合
            if (appState == STATE_PLAY && aiEnabled && !winFlag) {
                char turn = gomoku_current_player();
                char aiTurn = aiGoesFirst ? 'X' : 'O';
                if (turn == aiTurn) {
                    int ar, ac;
                    gomoku_get_ai_move(&ar, &ac);
                    if (gomoku_make_move(ar, ac)) {
                        if (gomoku_check_win(ar, ac)) {
                            winFlag = 1;
                            if (aiGoesFirst)      showMessage("I only had to think two moves ahead to beat you.", 3);
                            else                  showMessage("Looks like having the first move didn’t help you much.", 3);
                        }
                        else {
                            gomoku_switch_player();
                        }
                    }
                }
            }
            drawBoard(appState == STATE_REVIEW);
            SDL_Delay(16);
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
