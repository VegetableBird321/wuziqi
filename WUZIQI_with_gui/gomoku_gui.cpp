#define _CRT_SECURE_NO_WARNINGS 
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "gomoku_logic.h"

#define SIZE 15
#define WINDOW_SIZE 600
#define INFO_HEIGHT 80
#define WINDOW_HEIGHT (WINDOW_SIZE + INFO_HEIGHT)
#define CELL_SIZE (WINDOW_SIZE / SIZE)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

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
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        return false;
    }
    window = SDL_CreateWindow("Gomoku", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        return false;
    }
    font = TTF_OpenFont("msyh.ttc", 18);
    if (!font) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        return false;
    }
    return true;
}

void drawBoardGUI(bool inReviewMode, int reviewStep) {
    SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);
    SDL_RenderClear(renderer);
    gomoku_detect_threats();

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int x = j * CELL_SIZE;
            int y = i * CELL_SIZE;
            SDL_Rect cellRect = { x, y, CELL_SIZE, CELL_SIZE };

            const int (*winMarks)[SIZE] = gomoku_get_win_marks();
            if (winMarks && winMarks[i][j]) {
                SDL_SetRenderDrawColor(renderer, 144, 238, 144, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }

            const int (*threatMarks)[SIZE] = gomoku_get_threat_marks();
            if (!inReviewMode && threatMarks && threatMarks[i][j]) {
                SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &cellRect);

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

    int moveCount = gomoku_move_count();
    if (!inReviewMode && moveCount > 0) {
        int lr, lc;
        gomoku_get_move(moveCount - 1, &lr, &lc);
        SDL_Rect lastRect = { lc * CELL_SIZE, lr * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int k = 0; k < 3; k++) SDL_RenderDrawRect(renderer, &lastRect);
    }

    SDL_Rect infoRect = { 0, WINDOW_SIZE, WINDOW_SIZE, INFO_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255);
    SDL_RenderFillRect(renderer, &infoRect);

    char info[128];
    snprintf(info, sizeof(info), "Turn: %c   Hotkeys: U=Undo S=Save L=Load R=Replay T=Hint", gomoku_current_player());
    SDL_Color textColor = { 0, 0, 0, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, info, textColor);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = { 5, WINDOW_SIZE + 5, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);

    if (messageBuffer[0]) {
        Uint32 now = SDL_GetTicks();
        if (now - messageStart < (Uint32)messageDuration) {
            SDL_Surface* msurf = TTF_RenderText_Blended(font, messageBuffer, textColor);
            SDL_Texture* mtex = SDL_CreateTextureFromSurface(renderer, msurf);
            SDL_Rect mdst = { 5, WINDOW_SIZE + 30, msurf->w, msurf->h };
            SDL_RenderCopy(renderer, mtex, NULL, &mdst);
            SDL_FreeSurface(msurf);
            SDL_DestroyTexture(mtex);
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
    int savedMoveCount = 0;
    int win = 0;

    drawBoardGUI(false, 0);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
                break;
            }

            if (inReviewMode) {
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_LEFT) {
                        if (reviewStep > 0) reviewStep--;
                        gomoku_init();
                        for (int i = 0; i < reviewStep; i++) {
                            int r, c;
                            gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        if (reviewStep > 0) gomoku_mark_win(0, 0);
                    }
                    else if (e.key.keysym.sym == SDLK_RIGHT) {
                        if (reviewStep < savedMoveCount) reviewStep++;
                        gomoku_init();
                        for (int i = 0; i < reviewStep; i++) {
                            int r, c;
                            gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        if (reviewStep > 0) gomoku_mark_win(0, 0);
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        gomoku_init();
                        for (int i = 0; i < savedMoveCount; i++) {
                            int r, c;
                            gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        inReviewMode = false;
                    }
                }
            }
            else if (win == 1) {
                switch (e.key.keysym.sym) {
                case SDLK_u:
                    if (gomoku_undo_moves()) {
                        gomoku_switch_player(); win = 0;break;
                    }
                    break;
                case SDLK_l:
                    if (gomoku_load("save.txt")) showMessage("Game loaded", 2);
                    else showMessage("Load failed", 2);
                    break;
                case SDLK_t:
                    gomoku_touch_opponent();
                    showMessage("You patted your opponent's head!", 2);
                    break;
                case SDLK_r:
                    savedMoveCount = gomoku_move_count();
                    if (savedMoveCount > 0) {
                        inReviewMode = true;
                        reviewStep = 0;
                        gomoku_init();
                    }
                    break;
                case SDLK_e:
                    gomoku_init();
                    win = 0;
                    break;
                default:
                    break;
                }
            }
            else   {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int x = e.button.x;
                    int y = e.button.y;
                    if (y < WINDOW_SIZE) {
                        int col = x / CELL_SIZE;
                        int row = y / CELL_SIZE;
                        if (gomoku_make_move(row, col)) {
                            if (gomoku_check_win(row, col)) {
                                showMessage(gomoku_current_player() == 'X' ? "Player X wins!\npress r to replay,press e to restart" : "Player O wins!\npress r to replay,press e to restart", 5);
                                win = 1;
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
                        if (gomoku_undo_moves())
                        break;
                    case SDLK_s:
                        if (gomoku_save("save.txt")) showMessage("Game saved", 2);
                        else showMessage("Save failed", 2);
                        break;
                    case SDLK_l:
                        if (gomoku_load("save.txt")) showMessage("Game loaded", 2);
                        else showMessage("Load failed", 2);
                        break;
                    case SDLK_t:
                        gomoku_touch_opponent();
                        showMessage("You patted your opponent's head!", 2);
                        break;
                    case SDLK_r:
                        savedMoveCount = gomoku_move_count();
                        if (savedMoveCount > 0) {
                            inReviewMode = true;
                            reviewStep = 0;
                            gomoku_init();
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        drawBoardGUI(inReviewMode, reviewStep);
        SDL_Delay(16);
    }
}

int main(int argc, char* argv[]) {
    system("chcp 65001 > nul");  // Ensure UTF-8 console output on Windows
    SetConsoleOutputCP(CP_UTF8);
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    printf("Working directory: %s\n", buffer);

    if (!initSDL()) return -1;

    gomoku_init();
    runGUI();

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

