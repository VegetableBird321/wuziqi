/*
 * Gomoku GUI version using SDL2 and SDL_ttf, �����߼��ӿ�ʵ�ֹ��ܣ�GUI �����������û�����
 * �߼������� gomoku_logic.h/.c ��ʵ�֣�GUI ������ӿڡ�
 *
 * �߼��ӿڣ�ʾ�������� gomoku_logic.h ���������� gomoku_logic.c ��ʵ�֣���
 *   void gomoku_init();                                // ��ʼ�����̺�״̬
 *   bool gomoku_make_move(int row, int col);           // ������ (row,col) ���ӣ����سɹ����
 *   bool gomoku_undo_moves();                          // ������������������Ƿ�ɹ�
 *   bool gomoku_save(const char *filename);            // ������ֵ��ļ�
 *   bool gomoku_load(const char *filename);            // ���ļ��������
 *   bool gomoku_check_win(int row, int col);           // �����������Ƿ�ʤ��
 *   void gomoku_switch_player();                       // �л���ǰ���
 *   char gomoku_current_player();                      // ��ȡ��ǰ��� 'X' �� 'O'
 *   void gomoku_detect_threats();                      // �ڲ�ά����в����
 *   const int (*gomoku_get_threat_marks())[SIZE];      // ��ȡ��в�������ָ��
 *   void gomoku_mark_win(int row, int col);            // �ڲ����ʤ������
 *   const int (*gomoku_get_win_marks())[SIZE];         // ��ȡʤ���������ָ��
 *   int gomoku_move_count();                           // ��ǰ����
 *   void gomoku_get_move(int index, int *row, int *col);// ��ȡ�� index ��������λ��
 *   int gomoku_board_cell(int row, int col);           // ��ȡ����ĳ��״̬�� 'X','O' �� ' '
 *   // ��ѡ�������߼��ӿ�
 *   void gomoku_touch_opponent();                      // �߼��ڲ�������ͷ��־
 *   bool gomoku_was_touched();                         // �߼����ѯ�Ƿ���Ҫ��ʾ��ͷ��ʾ
 *   void gomoku_clear_touch();                         // �����ͷ��ʾ״̬
 *   // ������أ����� move history���ⲿ�ɰ������ gomoku_init & gomoku_make_move
 *
 * GUI ���ְ�����
 *   - ��ʼ�� SDL2/TTF
 *   - �������̡����ӡ�������ͨ�������߼��ӿڻ�ȡ��ǣ�
 *   - �¼�������������� gomoku_make_move���ٵ��� gomoku_check_win��gomoku_switch_player
 *   - ���̿�ݼ� U: ���� gomoku_undo_moves + gomoku_switch_player
 *                  S: gomoku_save
 *                  L: gomoku_load
 *                  R: ���븴��ģʽ��GUI ͨ����� gomoku_init+gomoku_make_move ����
 *                  T: ���� gomoku_touch_opponent����ʾ��ʾ
 *   - �ײ���Ϣ��ʾ��ǰ��� gomoku_current_player()
 *   - ��ʱ��Ϣϵͳ���� GUI �����й���
 *
 * �������� gomoku_logic.c ����ֲ���������п���̨�����߼������������������/�����صĲ����滻Ϊ���Ͻӿڡ�
 * GUI ֻ����ýӿڲ����ݷ���ֵ���½��档
 * ����ʾ�� GUI ���룺
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "gomoku_logic.h"  // ���������ӿڣ���Ҫ����ʵ��

#define SIZE 15
#define WINDOW_SIZE 600
#define INFO_HEIGHT 80
#define WINDOW_HEIGHT (WINDOW_SIZE + INFO_HEIGHT)
#define CELL_SIZE (WINDOW_SIZE / SIZE)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

// ��ʱ��Ϣϵͳ
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
    window = SDL_CreateWindow("������", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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
    // �߼����Զ�������в��ʤ�����
    gomoku_detect_threats();
    // �������������
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int x = j * CELL_SIZE;
            int y = i * CELL_SIZE;
            SDL_Rect cellRect = { x, y, CELL_SIZE, CELL_SIZE };
            // ʤ�����ߣ���ɫ��
            const int (*winMarks)[SIZE] = gomoku_get_win_marks();
            if (winMarks[i][j]) {
                SDL_SetRenderDrawColor(renderer, 144, 238, 144, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }
            // ��в�㣺��ɫ�ף����Ծ�ʱ��ʾ
            const int (*threatMarks)[SIZE] = gomoku_get_threat_marks();
            if (!inReviewMode && threatMarks[i][j]) {
                SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }
            // ������
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &cellRect);
            // ��������
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
    // ��һ����������ɫ�߿�
    int moveCount = gomoku_move_count();
    if (!inReviewMode && moveCount > 0) {
        int lr, lc;
        gomoku_get_move(moveCount - 1, &lr, &lc);
        SDL_Rect lastRect = { lc * CELL_SIZE, lr * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int k = 0; k < 3; k++) SDL_RenderDrawRect(renderer, &lastRect);
    }
    // ��Ϣ������
    SDL_Rect infoRect = { 0, WINDOW_SIZE, WINDOW_SIZE, INFO_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255); SDL_RenderFillRect(renderer, &infoRect);
    // �ı�����ǰ���������ʾ
    char info[128];
    snprintf(info, sizeof(info), "��ǰ���: %c   ���: U����, S����, L����, R����, T��ͷ", gomoku_current_player());
    SDL_Color textColor = { 0,0,0,255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, info, textColor);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = { 5, WINDOW_SIZE + 5, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_FreeSurface(surf); SDL_DestroyTexture(tex);
    // ��Ϣ��ʾ
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
    // ����״̬�����˳�����
    // ����ʱ���ⲿͨ�� gomoku_init + gomoku_make_move ���ֲ���
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
                        // ���� reviewStep ��
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
                        // �ָ��Ծ�
                        gomoku_init();
                        for (int i = 0; i < savedMoveCount; i++) {
                            int r, c; gomoku_get_move(i, &r, &c);
                            gomoku_make_move(r, c);
                        }
                        // �ָ���ǰ���
                        // �߼��� current player ���� gomoku_make_move �л����趨
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
                                showMessage((gomoku_current_player() == 'X' ? "��� X ��ʤ!" : "��� O ��ʤ!"), 5);
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
                        if (gomoku_save(SAVE_FILE)) showMessage("�ѱ���", 2);
                        else showMessage("����ʧ��", 2);
                        break;
                    case SDLK_l:
                        if (gomoku_load(SAVE_FILE)) showMessage("�Ѽ���", 2);
                        else showMessage("����ʧ��", 2);
                        break;
                    case SDLK_t:
                        gomoku_touch_opponent();
                        showMessage("�����������ֵ�ͷ~", 2);
                        break;
                    case SDLK_r:
                        if (gomoku_move_count() > 0) {
                            inReviewMode = true;
                            reviewStep = 0;
                            // ���浱ǰ״̬: ���� move history
                            savedMoveCount = gomoku_move_count();
                            for (int i = 0; i < savedMoveCount; i++) {
                                int rr, cc; gomoku_get_move(i, &rr, &cc);
                                savedBoardState[rr][cc] = gomoku_board_cell(rr, cc);
                            }
                            // ���븴��ʱ���ȳ�ʼ���߼�
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
