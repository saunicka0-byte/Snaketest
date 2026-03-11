#ifndef SNAKE_COMMON_H
#define SNAKE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WIDTH     30
#define HEIGHT    20
#define MAX_SNAKE (WIDTH * HEIGHT)
#define FRAME_MS  150

typedef struct { int x, y; } Point;

#define KEY_UP    256
#define KEY_DOWN  257
#define KEY_LEFT  258
#define KEY_RIGHT 259

static Point snake[MAX_SNAKE];
static int   length   = 3;
static Point food;
static Point dir      = {1, 0};
static int   gameOver = 0;

void platform_init(void);
void platform_cleanup(void);
void move_cursor(int x, int y);
int  read_key(void);

static int snake_at(int x, int y) {
    for (int i = 0; i < length; i++)
        if (snake[i].x == x && snake[i].y == y) return 1;
        return 0;
}

static void spawn_food(void) {
    int n = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!snake_at(x, y)) {
                n++;
                if (rand() % n == 0) food = (Point){x, y};
            }
        }
    }
    if (!n) gameOver = 1;
}

static void game_init(void) {
    length   = 3;
    dir      = (Point){1, 0};
    gameOver = 0;
    snake[0] = (Point){ WIDTH/2,     HEIGHT/2 };
    snake[1] = (Point){ WIDTH/2 - 1, HEIGHT/2 };
    snake[2] = (Point){ WIDTH/2 - 2, HEIGHT/2 };
    srand((unsigned)time(NULL));
    spawn_food();
}

static void apply_direction(int c) {
    switch (c) {
        case KEY_UP:   case 'w': case 'W':
            if (dir.y == 0) { dir.x =  0; dir.y = -1; } break;
        case KEY_DOWN: case 's': case 'S':
            if (dir.y == 0) { dir.x =  0; dir.y =  1; } break;
        case KEY_LEFT: case 'a': case 'A':
            if (dir.x == 0) { dir.x = -1; dir.y =  0; } break;
        case KEY_RIGHT:case 'd': case 'D':
            if (dir.x == 0) { dir.x =  1; dir.y =  0; } break;
    }
}

static void update(void) {
    Point head = { snake[0].x + dir.x, snake[0].y + dir.y };

    if (head.x < 0 || head.x >= WIDTH || head.y < 0 || head.y >= HEIGHT
        || snake_at(head.x, head.y)) {
        gameOver = 1; return;
        }

        int ate = (head.x == food.x && head.y == food.y);
    if (!ate) length--;
        memmove(&snake[1], &snake[0], (size_t)length * sizeof(Point));
    snake[0] = head;
    length++;

    if (ate) spawn_food();
}

static void draw_board(int show_gameover) {
    move_cursor(0, 0);

    for (int x = 0; x < WIDTH + 2; x++) putchar('#');
    putchar('\n');

    for (int y = 0; y < HEIGHT; y++) {
        putchar('#');
        for (int x = 0; x < WIDTH; x++) {
            char c = ' ';
            if (x == food.x && y == food.y) {
                c = '*';
            } else {
                for (int i = 0; i < length; i++) {
                    if (snake[i].x == x && snake[i].y == y) {
                        c = (i == 0) ? 'O' : 'o'; break;
                    }
                }
            }
            putchar(c);
        }
        putchar('#');
        putchar('\n');
    }

    for (int x = 0; x < WIDTH + 2; x++) putchar('#');
    putchar('\n');

    if (show_gameover) {
        printf("  ** GAME OVER **  Score: %d   I=Restart  Q=Quit\n", length - 3);
    } else {
        printf("Score: %-4d | WASD/Arrows=move  I=restart  Q=quit   \n", length - 3);
    }
    fflush(stdout);
}

static void handle_input(void) {
    int c = read_key();
    if (!c) return;
    if ((c | 0x20) == 'q') { gameOver = -1; return; }
    apply_direction(c);
}

static void handle_input_gameover(void) {
    int c = read_key();
    if (!c) return;
    switch (c | 0x20) {
        case 'i': game_init();   break;
        case 'q': gameOver = -1; break;
    }
}

static void run_game(void) {
    platform_init();
    game_init();

    for (;;) {
        if (gameOver == 0) {
            handle_input();
            if (gameOver == 0) update();
            draw_board(0);
        } else if (gameOver == 1) {
            draw_board(1);
            handle_input_gameover();
        } else {
            break;
        }

        #ifdef _WIN32
        Sleep(FRAME_MS);
        #else
        usleep(FRAME_MS * 1000);
        #endif
    }

    move_cursor(0, HEIGHT + 3);
    printf("\nThanks for playing!  Final score: %d\n", length - 3);
}

#endif
