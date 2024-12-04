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
#define CLICK_RADIUS 10

SDL_Color backgroundColor = {0, 0, 0, 255};
SDL_Color dotColor = {203, 170, 203, 255};

typedef struct {
    float x, y, vx, vy, original_x, original_y;
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
            dots[row][col] = (Dot){startX + col * SPRING_LENGTH, startY + row * SPRING_LENGTH, 0, 0, startX + col * SPRING_LENGTH, startY + row * SPRING_LENGTH, false};
        }
    }
    dots[0][0].fixed = dots[0][GRID_COLS - 1].fixed = true;
}

void apply_spring(Dot *a, Dot *b) {
    float dx = b->x - a->x, dy = b->y - a->y;
    float distance = sqrtf(dx * dx + dy * dy);
    float force = (distance - SPRING_LENGTH) * SPRING_CONSTANT;
    float fx = force * (dx / distance), fy = force * (dy / distance);

    if (!a->fixed) { a->vx += fx; a->vy += fy; }
    if (!b->fixed) { b->vx -= fx; b->vy -= fy; }
}

void apply_restoring_force(Dot *dot) {
    if (!dot->fixed) {
        float dx = dot->original_x - dot->x, dy = dot->original_y - dot->y;
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
    SDL_SetRenderDrawColor(renderer, dotColor.r, dotColor.g, dotColor.b, dotColor.a);
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            for (int w = 0; w < DOT_RADIUS * 2; w++) {
                for (int h = 0; h < DOT_RADIUS * 2; h++) {
                    int dx = DOT_RADIUS - w, dy = DOT_RADIUS - h;
                    if ((dx * dx + dy * dy) <= (DOT_RADIUS * DOT_RADIUS)) {
                        SDL_RenderDrawPoint(renderer, dots[row][col].x + dx, dots[row][col].y + dy);
                    }
                }
            }
        }
    }
}

void handle_mouse_event(SDL_Event *event) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        for (int row = 0; row < GRID_ROWS; row++) {
            for (int col = 0; col < GRID_COLS; col++) {
                float dx = x - dots[row][col].x, dy = y - dots[row][col].y;
                if (sqrtf(dx * dx + dy * dy) < CLICK_RADIUS) {
                    dragging = true;
                    drag_row = row;
                    drag_col = col;
                    dots[row][col].fixed = true;
                    return;
                }
            }
        }
    } else if (event->type == SDL_MOUSEBUTTONUP && dragging) {
        dots[drag_row][drag_col].fixed = false;
        dragging = false;
    } else if (event->type == SDL_MOUSEMOTION && dragging) {
        dots[drag_row][drag_col].x = x;
        dots[drag_row][drag_col].y = y;
    }
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window *window = SDL_CreateWindow("Dot Matrix Sheet", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) { SDL_Quit(); return 1; }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    initialize_dots();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            else handle_mouse_event(&event);
        }

        update_dots();

        SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderClear(renderer);
        render_dots(renderer);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}