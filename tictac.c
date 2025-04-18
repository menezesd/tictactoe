#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_SIZE 600
#define CELL_SIZE (WINDOW_SIZE / 3)
#define FULL 0777

typedef unsigned int bitfield;

typedef enum { X, O, NONE = -1 } side;

side next_player(side p) { return (p + 1) % 2; }
char token(side p) { return (p == X) ? 'X' : (p == O) ? 'O' : '?'; }

/* Determine if there are three in a row.  Given an input n
   representing a bitmask for the squares occupied by a player, test
   if any of the patterns in wins is matched.  */
bool is_win(bitfield board) {
    static const bitfield wins[] = {0007, 0070, 0700, 0111, 0222, 0444, 0421, 0124};
    for (size_t i = 0; i < sizeof(wins)/sizeof(int); i++)
        if ((board & wins[i]) == wins[i]) return true;
    return false;
}

/* evaluation scores */
typedef int score;
enum { WIN = 1, LOSS = -1, DRAW = 0 };

/* determine best move using recursive search */
/*  parameters:
 *      me:         bitfield indicating squares I occupy
 *      opp:        bitfield indicating squares opponent occupies
 *      achievable: score of best variation found so far
 *      cutoff:     if we can find a move better than this our opponent
 *                  will avoid this variation so we can stop searching
 *      best:       output parameter used to store index of square 
 *                  indicating the move with the best score
 *      return value: score of position, from perspective of player to move
*/
score alpha_beta(bitfield me, bitfield opp, score achievable, score cutoff, int *best) {
    int bestmove = 0;
    if ((me | opp) == FULL) return DRAW;

    for (int i = 0; i < 9; i++) {
        if ((me | opp) & (1 << i)) continue;
        bitfield tmp = me | (1 << i);
        score cur = is_win(tmp) ? WIN : -alpha_beta(opp, tmp, -cutoff, -achievable, NULL);
        
        if (cur > achievable) {
            achievable = cur;
            bestmove = i;
        }
        if (achievable >= cutoff) break;
    }
    if (best) *best = bestmove;
    return achievable;
}

void draw_x(SDL_Renderer *ren, int cell) {
    int x = (cell % 3) * CELL_SIZE;
    int y = (cell / 3) * CELL_SIZE;
    int pad = CELL_SIZE / 4;
    SDL_RenderDrawLine(ren, x+pad, y+pad, x+CELL_SIZE-pad, y+CELL_SIZE-pad);
    SDL_RenderDrawLine(ren, x+CELL_SIZE-pad, y+pad, x+pad, y+CELL_SIZE-pad);
}

void draw_o(SDL_Renderer *ren, int cell) {
    int cx = (cell % 3) * CELL_SIZE + CELL_SIZE/2;
    int cy = (cell / 3) * CELL_SIZE + CELL_SIZE/2;
    int radius = CELL_SIZE/3;
    
    SDL_Point points[360];
    for (int i = 0; i < 360; i++) {
        double angle = i * M_PI / 180;
        points[i].x = cx + radius * cos(angle);
        points[i].y = cy + radius * sin(angle);
    }
    SDL_RenderDrawLines(ren, points, 360);
}

void render_board(SDL_Renderer *ren, bitfield *players, TTF_Font *font, const char *message) {
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    for (int i = 1; i < 3; i++) {
        SDL_RenderDrawLine(ren, i*CELL_SIZE, 0, i*CELL_SIZE, WINDOW_SIZE);
        SDL_RenderDrawLine(ren, 0, i*CELL_SIZE, WINDOW_SIZE, i*CELL_SIZE);
    }

    for (int i = 0; i < 9; i++) {
        if (players[X] & (1 << i)) draw_x(ren, i);
        if (players[O] & (1 << i)) draw_o(ren, i);
    }

    if (message) {
        SDL_Color color = {255, 0, 0, 255};
        SDL_Surface *surf = TTF_RenderText_Solid(font, message, color);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_Rect rect = {WINDOW_SIZE/4, WINDOW_SIZE/2 - 30, WINDOW_SIZE/2, 60};
        SDL_RenderCopy(ren, tex, NULL, &rect);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    }

    SDL_RenderPresent(ren);
}

side select_ai(SDL_Renderer *ren, TTF_Font *font) {
    const char *options[] = {"Play X (AI O)", "Play O (AI X)", "Two Player"};
    SDL_Rect rects[3];
    SDL_Texture *textures[3];

    for (int i = 0; i < 3; i++) {
        SDL_Surface *surf = TTF_RenderText_Solid(font, options[i], (SDL_Color){0,0,0,255});
        textures[i] = SDL_CreateTextureFromSurface(ren, surf);
        rects[i] = (SDL_Rect){WINDOW_SIZE/4, 100 + i*100, WINDOW_SIZE/2, 50};
        SDL_FreeSurface(surf);
    }

    while (1) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x, y = e.button.y;
                for (int i = 0; i < 3; i++) {
                    if (x >= rects[i].x && x <= rects[i].x + rects[i].w &&
                        y >= rects[i].y && y <= rects[i].y + rects[i].h) {
                        for (int j = 0; j < 3; j++) SDL_DestroyTexture(textures[j]);
                        return (side[]){O, X, NONE}[i];
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);
        for (int i = 0; i < 3; i++)
            SDL_RenderCopy(ren, textures[i], NULL, &rects[i]);
        SDL_RenderPresent(ren);
        SDL_Delay(100);
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("Tic Tac Toe", SDL_WINDOWPOS_CENTERED, 
                                      SDL_WINDOWPOS_CENTERED, WINDOW_SIZE, WINDOW_SIZE, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("arial.ttf", 24);

    bitfield players[2] = {0};
    side cur_player = X;
    side ai = select_ai(ren, font);
    bool over = false;
    const char *message = NULL;

    while (!over) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) over = true;
            
            if (!over && cur_player != ai && e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x / CELL_SIZE;
                int y = e.button.y / CELL_SIZE;
                int move = y * 3 + x;

                if (move >= 0 && move < 9 && !(players[X] & (1 << move))) {
                    players[cur_player] |= (1 << move);

                    if (is_win(players[cur_player])) {
                        message = cur_player == X ? "X Wins!" : "O Wins!";
                        over = true;
                    } else if ((players[X] | players[O]) == FULL) {
                        message = "Draw!";
                        over = true;
                    }
                    cur_player = next_player(cur_player);
                }
            }
        }

        if (!over && cur_player == ai) {
            int move;
            alpha_beta(players[ai], players[next_player(ai)], -INT_MAX, INT_MAX, &move);
            players[ai] |= (1 << move);

            if (is_win(players[ai])) {
                message = "AI Wins!";
                over = true;
            } else if ((players[X] | players[O]) == FULL) {
                message = "Draw!";
                over = true;
            }
            cur_player = next_player(cur_player);
        }

        render_board(ren, players, font, over ? message : NULL);
        SDL_Delay(100);
    }

    SDL_Delay(2000);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
