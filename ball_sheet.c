#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 800
#define HEIGHT 600
#define DOT_RADIUS 2
#define GRID_ROWS 30
#define GRID_COLS 40
#define SPRING_LENGTH 15
#define SPRING_CONSTANT 0.2
#define DAMPING 0.9
#define RESTORING_FORCE 0.01

typedef struct {
    float x, y;
    float vx, vy;
    float original_x, original_y;
    bool fixed;
} Dot;

Dot dots[GRID_ROWS][GRID_COLS];
bool dragging = false;
int drag_row = -1, drag_col = -1;

void initialize_dots() {
    float startX = (WIDTH - (GRID_COLS - 1) * SPRING_LENGTH) / 2;
    float startY = (HEIGHT - (GRID_ROWS - 1) * SPRING_LENGTH) / 2;

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            dots[row][col].x = dots[row][col].original_x = startX + col * SPRING_LENGTH;
            dots[row][col].y = dots[row][col].original_y = startY + row * SPRING_LENGTH;
            dots[row][col].vx = 0;
            dots[row][col].vy = 0;
            dots[row][col].fixed = false;
        }
    }
    // Fix the top-left and top-right dots for stability
    dots[0][0].fixed = true;
    dots[0][GRID_COLS - 1].fixed = true;
}

void apply_spring(Dot *a, Dot *b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float distance = sqrtf(dx * dx + dy * dy);
    float force = (distance - SPRING_LENGTH) * SPRING_CONSTANT;
    float fx = force * (dx / distance);
    float fy = force * (dy / distance);

    if (!a->fixed) {
        a->vx += fx;
        a->vy += fy;
    }
    if (!b->fixed) {
        b->vx -= fx;
        b->vy -= fy;
    }
}

void apply_restoring_force(Dot *dot) {
    if (!dot->fixed) {
        float dx = dot->original_x - dot->x;
        float dy = dot->original_y - dot->y;
        dot->vx += dx * RESTORING_FORCE;
        dot->vy += dy * RESTORING_FORCE;
    }
}

void update_dots() {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            if (!dots[row][col].fixed) {
                dots[row][col].vx *= DAMPING;
                dots[row][col].vy *= DAMPING;
                dots[row][col].x += dots[row][col].vx;
                dots[row][col].y += dots[row][col].vy;
                apply_restoring_force(&dots[row][col]);
            }
        }
    }

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            if (row > 0) apply_spring(&dots[row][col], &dots[row - 1][col]);
            if (row < GRID_ROWS - 1) apply_spring(&dots[row][col], &dots[row + 1][col]);
            if (col > 0) apply_spring(&dots[row][col], &dots[row][col - 1]);
            if (col < GRID_COLS - 1) apply_spring(&dots[row][col], &dots[row][col + 1]);
        }
    }
}

void render_dots(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            SDL_Rect rect = {
                (int)(dots[row][col].x - DOT_RADIUS),
                (int)(dots[row][col].y - DOT_RADIUS),
                DOT_RADIUS * 2,
                DOT_RADIUS * 2
            };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void handle_mouse_event(SDL_Event *event) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        for (int row = 0; row < GRID_ROWS; row++) {
            for (int col = 0; col < GRID_COLS; col++) {
                float dx = x - dots[row][col].x;
                float dy = y - dots[row][col].y;
                if (sqrtf(dx * dx + dy * dy) < DOT_RADIUS) {
                    dragging = true;
                    drag_row = row;
                    drag_col = col;
                    dots[row][col].fixed = true;
                    return;
                }
            }
        }
    } else if (event->type == SDL_MOUSEBUTTONUP) {
        if (dragging) {
            dots[drag_row][drag_col].fixed = false;
            dragging = false;
        }
    } else if (event->type == SDL_MOUSEMOTION) {
        if (dragging) {
            dots[drag_row][drag_col].x = x;
            dots[drag_row][drag_col].y = y;
        }
    }
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Dot Sheet", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    initialize_dots();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                handle_mouse_event(&event);
            }
        }

        update_dots();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render_dots(renderer);
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}