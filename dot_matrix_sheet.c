#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* Window config */
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "Dot Matrix Sheet"

/* Grid config */
#define GRID_ROWS 30
#define GRID_COLS 40
#define DOT_RADIUS 2

/* Physics constants */
#define SPRING_REST_LENGTH 15
#define SPRING_STIFFNESS 0.2
#define VELOCITY_DAMPING 0.9
#define RESTORING_FORCE_STRENGTH 0.01

/* Interaction constants */
#define CLICK_DETECTION_RADIUS 10
#define FRAME_DELAY_MS 16

/* Visual config */
#define BACKGROUND_COLOR_R 0
#define BACKGROUND_COLOR_G 0
#define BACKGROUND_COLOR_B 0
#define BACKGROUND_COLOR_A 255

#define DOT_COLOR_R 203
#define DOT_COLOR_G 170
#define DOT_COLOR_B 203
#define DOT_COLOR_A 255

/* Type Definitions */

/**
 * Represents a single dot in the grid with position, velocity, and state.
 */
typedef struct
{
    float x;          /* Current x position */
    float y;          /* Current y position */
    float vx;         /* Velocity in x direction */
    float vy;         /* Velocity in y direction */
    float original_x; /* Original x position (rest state) */
    float original_y; /* Original y position (rest state) */
    bool fixed;       /* Whether the dot is fixed in place */
} Dot;

/**
 * Manages the state of mouse dragging interaction.
 */
typedef struct
{
    bool is_dragging;
    int row;
    int col;
} DragState;

/* Global State */
static Dot g_dots[GRID_ROWS][GRID_COLS];
static DragState g_drag_state = {false, -1, -1};
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static bool g_running = true;

/* Function Prototypes */
static void initialize_grid(void);
static void apply_spring_force(Dot *dot_a, Dot *dot_b);
static void apply_restoring_force(Dot *dot);
static void update_physics(void);
static void render_grid(SDL_Renderer *renderer);
static void handle_mouse_event(const SDL_Event *event);
static bool find_dot_at_position(int mouse_x, int mouse_y, int *row, int *col);
static void draw_filled_circle(SDL_Renderer *renderer, int center_x, int center_y, int radius);
static void main_loop(void);

/**
 * Initializes the dot grid with evenly spaced dots centered in the window.
 * Sets the top-left and top-right corner dots as fixed anchor points.
 */
static void initialize_grid(void)
{
    const float start_x = (WINDOW_WIDTH - (GRID_COLS - 1) * SPRING_REST_LENGTH) / 2.0f;
    const float start_y = (WINDOW_HEIGHT - (GRID_ROWS - 1) * SPRING_REST_LENGTH) / 2.0f;

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            const float pos_x = start_x + col * SPRING_REST_LENGTH;
            const float pos_y = start_y + row * SPRING_REST_LENGTH;

            g_dots[row][col] = (Dot){
                .x = pos_x,
                .y = pos_y,
                .vx = 0.0f,
                .vy = 0.0f,
                .original_x = pos_x,
                .original_y = pos_y,
                .fixed = false};
        }
    }

    /* Fix the top corners as anchor points */
    g_dots[0][0].fixed = true;
    g_dots[0][GRID_COLS - 1].fixed = true;
}

/**
 * Applies spring force between two connected dots using Hooke's Law.
 * The force is proportional to the displacement from the rest length.
 *
 * @param dot_a First dot
 * @param dot_b Second dot connected to first dot
 */
static void apply_spring_force(Dot *dot_a, Dot *dot_b)
{
    const float dx = dot_b->x - dot_a->x;
    const float dy = dot_b->y - dot_a->y;
    const float distance = sqrtf(dx * dx + dy * dy);

    if (distance < 0.001f)
    {
        return; /* Avoid division by zero */
    }

    const float displacement = distance - SPRING_REST_LENGTH;
    const float force_magnitude = displacement * SPRING_STIFFNESS;
    const float fx = force_magnitude * (dx / distance);
    const float fy = force_magnitude * (dy / distance);

    if (!dot_a->fixed)
    {
        dot_a->vx += fx;
        dot_a->vy += fy;
    }

    if (!dot_b->fixed)
    {
        dot_b->vx -= fx;
        dot_b->vy -= fy;
    }
}

/**
 * Applies a gentle force that pulls the dot back toward its original position.
 * This helps the grid return to its rest state after being disturbed.
 *
 * @param dot The dot to apply restoring force to
 */
static void apply_restoring_force(Dot *dot)
{
    if (dot->fixed)
    {
        return;
    }

    const float dx = dot->original_x - dot->x;
    const float dy = dot->original_y - dot->y;

    dot->vx += dx * RESTORING_FORCE_STRENGTH;
    dot->vy += dy * RESTORING_FORCE_STRENGTH;
}

/**
 * Updates all dots' positions and velocities based on physics simulation.
 * First applies damping and integrates velocity, then applies spring and restoring forces.
 */
static void update_physics(void)
{
    /* Update positions and apply damping */
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            Dot *dot = &g_dots[row][col];

            if (!dot->fixed)
            {
                dot->vx *= VELOCITY_DAMPING;
                dot->vy *= VELOCITY_DAMPING;
                dot->x += dot->vx;
                dot->y += dot->vy;
                apply_restoring_force(dot);
            }
        }
    }

    /* Apply spring forces between connected dots */
    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            Dot *current = &g_dots[row][col];

            /* Connect to neighboring dots (up, down, left, right) */
            if (row > 0)
            {
                apply_spring_force(current, &g_dots[row - 1][col]);
            }
            if (row < GRID_ROWS - 1)
            {
                apply_spring_force(current, &g_dots[row + 1][col]);
            }
            if (col > 0)
            {
                apply_spring_force(current, &g_dots[row][col - 1]);
            }
            if (col < GRID_COLS - 1)
            {
                apply_spring_force(current, &g_dots[row][col + 1]);
            }
        }
    }
}

/**
 * Draws a filled circle using the midpoint circle algorithm.
 *
 * @param renderer SDL renderer to draw with
 * @param center_x X coordinate of circle center
 * @param center_y Y coordinate of circle center
 * @param radius Radius of the circle in pixels
 */
static void draw_filled_circle(SDL_Renderer *renderer, int center_x, int center_y, int radius)
{
    for (int w = 0; w < radius * 2; w++)
    {
        for (int h = 0; h < radius * 2; h++)
        {
            const int dx = radius - w;
            const int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius))
            {
                SDL_RenderDrawPoint(renderer, center_x + dx, center_y + dy);
            }
        }
    }
}

/**
 * Renders all dots in the grid as filled circles.
 *
 * @param renderer SDL renderer to draw with
 */
static void render_grid(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, DOT_COLOR_R, DOT_COLOR_G, DOT_COLOR_B, DOT_COLOR_A);

    for (int row = 0; row < GRID_ROWS; row++)
    {
        for (int col = 0; col < GRID_COLS; col++)
        {
            const Dot *dot = &g_dots[row][col];
            draw_filled_circle(renderer, (int)dot->x, (int)dot->y, DOT_RADIUS);
        }
    }
}

/**
 * Finds the dot closest to the given mouse position within the click radius.
 *
 * @param mouse_x Mouse X coordinate
 * @param mouse_y Mouse Y coordinate
 * @param row Output parameter for the row of the found dot
 * @param col Output parameter for the column of the found dot
 * @return true if a dot was found, false otherwise
 */
static bool find_dot_at_position(int mouse_x, int mouse_y, int *row, int *col)
{
    for (int r = 0; r < GRID_ROWS; r++)
    {
        for (int c = 0; c < GRID_COLS; c++)
        {
            const float dx = mouse_x - g_dots[r][c].x;
            const float dy = mouse_y - g_dots[r][c].y;
            const float distance = sqrtf(dx * dx + dy * dy);

            if (distance < CLICK_DETECTION_RADIUS)
            {
                *row = r;
                *col = c;
                return true;
            }
        }
    }
    return false;
}

/**
 * Handles mouse events for dragging dots.
 * Users can click and drag dots to move them, creating wave effects in the grid.
 *
 * @param event SDL event to process
 */
static void handle_mouse_event(const SDL_Event *event)
{
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    switch (event->type)
    {
    case SDL_MOUSEBUTTONDOWN:
    {
        int row, col;
        if (find_dot_at_position(mouse_x, mouse_y, &row, &col))
        {
            g_drag_state.is_dragging = true;
            g_drag_state.row = row;
            g_drag_state.col = col;
            g_dots[row][col].fixed = true;
        }
        break;
    }

    case SDL_MOUSEBUTTONUP:
        if (g_drag_state.is_dragging)
        {
            g_dots[g_drag_state.row][g_drag_state.col].fixed = false;
            g_drag_state.is_dragging = false;
        }
        break;

    case SDL_MOUSEMOTION:
        if (g_drag_state.is_dragging)
        {
            g_dots[g_drag_state.row][g_drag_state.col].x = mouse_x;
            g_dots[g_drag_state.row][g_drag_state.col].y = mouse_y;
        }
        break;

    default:
        break;
    }
}

/**
 * Main entry point for the Dot Matrix Sheet simulation.
 * Initializes SDL, creates the window and renderer, runs the main loop,
 * and cleans up resources on exit.
 *
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on error
 */

/**
 * Main loop iteration function.
 * Processes events, updates physics, and renders the frame.
 * This function is called repeatedly either by the native loop or by Emscripten.
 */
static void main_loop(void)
{
    SDL_Event event;

    /* Process all pending events */
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            g_running = false;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
        }
        else
        {
            handle_mouse_event(&event);
        }
    }

    /* Update physics simulation */
    update_physics();

    /* Render frame */
    SDL_SetRenderDrawColor(
        g_renderer,
        BACKGROUND_COLOR_R,
        BACKGROUND_COLOR_G,
        BACKGROUND_COLOR_B,
        BACKGROUND_COLOR_A);
    SDL_RenderClear(g_renderer);
    render_grid(g_renderer);
    SDL_RenderPresent(g_renderer);
}

int main(void)
{
    /* Initialize SDL video subsystem */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    /* Create window */
    g_window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (!g_window)
    {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    /* Create renderer with hardware acceleration */
    g_renderer = SDL_CreateRenderer(
        g_window,
        -1,
        SDL_RENDERER_ACCELERATED);

    if (!g_renderer)
    {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    /* Initialize the dot grid */
    initialize_grid();

#ifdef __EMSCRIPTEN__
    /* Use Emscripten's main loop for browser compatibility */
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    /* Native main event loop */
    while (g_running)
    {
        main_loop();
        SDL_Delay(FRAME_DELAY_MS);
    }
#endif

    /* Cleanup resources */
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return EXIT_SUCCESS;
}