/*
 * Gomoku GUI version using SDL2 and SDL_ttf, 调用逻辑接口实现功能，GUI 仅负责界面和用户交互
 * 逻辑部分在 gomoku_logic.h/.c 中实现，GUI 调用其接口。
 *
 * 逻辑接口（示例，需在 gomoku_logic.h 中声明并在 gomoku_logic.c 中实现）：
 *   void gomoku_init();                                // 初始化棋盘和状态
 *   bool gomoku_make_move(int row, int col);           // 尝试在 (row,col) 落子，返回成功与否
 *   bool gomoku_undo_moves();                          // 撤销最近两步，返回是否成功
 *   bool gomoku_save(const char *filename);            // 保存棋局到文件
 *   bool gomoku_load(const char *filename);            // 从文件加载棋局
 *   bool gomoku_check_win(int row, int col);           // 检查最近落子是否胜利
 *   void gomoku_switch_player();                       // 切换当前玩家
 *   char gomoku_current_player();                      // 获取当前玩家 'X' 或 'O'
 *   void gomoku_detect_threats();                      // 内部维护威胁点标记
 *   const int (*gomoku_get_threat_marks())[SIZE];      // 获取威胁标记数组指针
 *   void gomoku_mark_win(int row, int col);            // 内部标记胜利连线
 *   const int (*gomoku_get_win_marks())[SIZE];         // 获取胜利标记数组指针
 *   int gomoku_move_count();                           // 当前步数
 *   void gomoku_get_move(int index, int *row, int *col);// 获取第 index 步的落子位置
 *   int gomoku_board_cell(int row, int col);           // 获取棋盘某格状态： 'X','O' 或 ' '
 *   // 可选触摸等逻辑接口
 *   void gomoku_touch_opponent();                      // 逻辑内部处理摸头标志
 *   bool gomoku_was_touched();                         // 逻辑层查询是否需要显示摸头提示
 *   void gomoku_clear_touch();                         // 清除摸头提示状态
 *   // 复盘相关：根据 move history，外部可按需调用 gomoku_init & gomoku_make_move
 *
 * GUI 部分包含：
 *   - 初始化 SDL2/TTF
 *   - 绘制棋盘、棋子、高亮（通过调用逻辑接口获取标记）
 *   - 事件处理：鼠标点击调用 gomoku_make_move，再调用 gomoku_check_win、gomoku_switch_player
 *   - 键盘快捷键 U: 调用 gomoku_undo_moves + gomoku_switch_player
 *                  S: gomoku_save
 *                  L: gomoku_load
 *                  R: 进入复盘模式，GUI 通过多次 gomoku_init+gomoku_make_move 重现
 *                  T: 调用 gomoku_touch_opponent，显示提示
 *   - 底部信息显示当前玩家 gomoku_current_player()
 *   - 临时消息系统：在 GUI 层自行管理
 *
 * 您可以在 gomoku_logic.c 中移植和清理现有控制台程序逻辑，将所有与界面输入/输出相关的部分替换为以上接口。
 * GUI 只需调用接口并根据返回值更新界面。
 * 下面示例 GUI 代码：
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "gomoku_logic.h"  // 声明上述接口，需要自行实现

#define SIZE 15
#define WINDOW_SIZE 600
#define INFO_HEIGHT 80
#define WINDOW_HEIGHT (WINDOW_SIZE + INFO_HEIGHT)
#define CELL_SIZE (WINDOW_SIZE / SIZE)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

// 临时消息系统
char messageBuffer[256] = "";
Uint32 messageStart = 0;
int messageDuration = 0;
void showMessage(const char* msg, int duration) {
    strncpy(messageBuffer, msg, sizeof(messageBuffer) - 1);
    messageBuffer[sizeof(messageBuffer) - 1] = '\0';
    messageStart = SDL_GetTicks();
    messageDuration = duration * 1000;
}

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError()); return false;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError()); return false;
    }
    window = SDL_CreateWindow("五子棋", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_SIZE, WINDOW_HEIGHT, 0);
    if (!window) { fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError()); return false; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError()); return false; }
    font = TTF_OpenFont("arial.ttf", 18);
    if (!font) { fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError()); return false; }
    return true;
}

void drawBoardGUI(bool inReviewMode, int reviewStep) {
    SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);
    SDL_RenderClear(renderer);
    // 逻辑层自动更新威胁和胜利标记
    gomoku_detect_threats();
    // 绘制网格和棋子
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int x = j * CELL_SIZE;
            int y = i * CELL_SIZE;
            SDL_Rect cellRect = { x, y, CELL_SIZE, CELL_SIZE };
            // 胜利连线：绿色底
            const int (*winMarks)[SIZE] = gomoku_get_win_marks();
            if (winMarks[i][j]) {
                SDL_SetRenderDrawColor(renderer, 144, 238, 144, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }
            // 威胁点：蓝色底，仅对局时显示
            const int (*threatMarks)[SIZE] = gomoku_get_threat_marks();
            if (!inReviewMode && threatMarks[i][j]) {
                SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }
            // 网格线
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &cellRect);
            // 绘制棋子
            char ch = gomoku_board_cell(i, j);
            if (ch == 'X' || ch == 'O') {
                SDL_Rect pieceRect = { x + 4, y + 4, CELL_SIZE - 8, CELL_SIZE - 8 };
                if (ch == 'X') SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &pieceRect);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &pieceRect);
            }
        }
    }
    // 上一步高亮：黄色边框
    int moveCount = gomoku_move_count();
    if (!inReviewMode && moveCount > 0) {
        int lr, lc;
        gomoku_get_move(moveCount - 1, &lr, &lc);
        SDL_Rect lastRect = { lc * CELL_SIZE, lr * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int k = 0; k < 3; k++) SDL_RenderDrawRect(renderer, &lastRect);
    }
    // 信息区背景
    SDL_Rect infoRect = { 0, WINDOW_SIZE, WINDOW_SIZE, INFO_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255); SDL_RenderFillRect(renderer, &infoRect);
    // 文本：当前玩家与快捷提示
    char info[128];
    snprintf(info, sizeof(info), "当前玩家: %c   快捷: U撤销, S保存, L加载, R复盘, T摸头", gomoku_current_player());
    SDL_Color textColor = { 0,0,0,255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, info, textColor);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = { 5, WINDOW_SIZE + 5, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_FreeSurface(surf); SDL_DestroyTexture(tex);
    // 消息显示
    if (messageBuffer[0]) {
        Uint32 now = SDL_GetTicks();
        if (now - messageStart < (Uint32)messageDuration) {
            SDL_Surface* msurf = TTF_RenderText_Blended(font, messageBuffer, textColor);
            SDL_Texture* mtex = SDL_CreateTextureFromSurface(renderer, msurf);
            SDL_Rect mdst = { 5, WINDOW_SIZE + 30, msurf->w, msurf->h };
            SDL_RenderCopy(renderer, mtex, NULL, &mdst);
            SDL_FreeSurface(msurf); SDL_DestroyTexture(mtex);
        }
        else {
            messageBuffer[0] = '\0';
        }
    }
    SDL_RenderPresent(renderer);
}

void runGUI() {
    bool running = true;
    bool inReviewMode = false;
    int reviewStep = 0;
    // 保存状态用于退出复盘
    // 复盘时：外部通过 gomoku_init + gomoku_make_move 重现步数
    char savedBoardState[SIZE][SIZE];
    int savedMoveCount;
    char savedPlayer;

    drawBoardGUI(false, 0);
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            if (inReviewMode) {
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_LEFT) {
                        if (reviewStep > 0) reviewStep--;
                        // 重现 reviewStep 步
                        gomoku_init();
                        for (int i = 0; i < reviewStep; i++) {
                            int r, c; gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        if (reviewStep > 0) gomoku_mark_win(gomoku_get_move_index_row(reviewStep - 1), gomoku_get_move_index_col(reviewStep - 1));
                    }
                    else if (e.key.keysym.sym == SDLK_RIGHT) {
                        if (reviewStep < savedMoveCount) reviewStep++;
                        gomoku_init();
                        for (int i = 0; i < reviewStep; i++) {
                            int r, c; gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        if (reviewStep > 0) gomoku_mark_win(gomoku_get_move_index_row(reviewStep - 1), gomoku_get_move_index_col(reviewStep - 1));
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        // 恢复对局
                        gomoku_init();
                        for (int i = 0; i < savedMoveCount; i++) {
                            int r, c; gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        // 恢复当前玩家
                        // 逻辑层 current player 已在 gomoku_make_move 切换后设定
                        inReviewMode = false;
                    }
                }
            }
            else {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int x = e.button.x, y = e.button.y;
                    if (y < WINDOW_SIZE) {
                        int c = x / CELL_SIZE;
                        int r = y / CELL_SIZE;
                        if (gomoku_make_move(r, c)) {
                            if (gomoku_check_win(r, c)) {
                                showMessage((gomoku_current_player() == 'X' ? "玩家 X 获胜!" : "玩家 O 获胜!"), 5);
                            }
                            else {
                                gomoku_switch_player();
                            }
                        }
                    }
                }
                else if (e.type == SDL_KEYDOWN) {
                    switch (e.key.keysym.sym) {
                    case SDLK_u:
                        if (gomoku_undo_moves()) {
                            gomoku_switch_player();
                        }
                        break;
                    case SDLK_s:
                        if (gomoku_save(SAVE_FILE)) showMessage("已保存", 2);
                        else showMessage("保存失败", 2);
                        break;
                    case SDLK_l:
                        if (gomoku_load(SAVE_FILE)) showMessage("已加载", 2);
                        else showMessage("加载失败", 2);
                        break;
                    case SDLK_t:
                        gomoku_touch_opponent();
                        showMessage("你摸了摸对手的头~", 2);
                        break;
                    case SDLK_r:
                        if (gomoku_move_count() > 0) {
                            inReviewMode = true;
                            reviewStep = 0;
                            // 保存当前状态: 复制 move history
                            savedMoveCount = gomoku_move_count();
                            for (int i = 0; i < savedMoveCount; i++) {
                                int rr, cc; gomoku_get_move(i, &rr, &cc);
                                savedBoardState[rr][cc] = gomoku_board_cell(rr, cc);
                            }
                            // 进入复盘时，先初始化逻辑
                            gomoku_init();
                        }
                        break;
                    default: break;
                    }
                }
            }
        }
        drawBoardGUI(inReviewMode, reviewStep);
        SDL_Delay(16);
    }
}

int main(int argc, char* argv[]) {
    if (!initSDL()) return -1;
    gomoku_init();
    runGUI();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit();
    return 0;
}
