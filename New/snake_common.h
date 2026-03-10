#ifndef SNAKE_COMMON_H
#define SNAKE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WIDTH      30
#define HEIGHT     20
#define MAX_SNAKE  (WIDTH * HEIGHT)
#define FRAME_MS   150


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


#define BUF_W  (WIDTH  + 2)
#define BUF_H  (HEIGHT + 2)
static char cur [BUF_H][BUF_W];
static char prev[BUF_H][BUF_W];

void platform_init(void);
void platform_cleanup(void);
void move_cursor(int x, int y);
int  read_key(void);

static void init_buffer(void) {
    memset(cur,  ' ', sizeof cur);
    memset(prev, '\0', sizeof prev);

    for (int x = 0; x < BUF_W; x++) {
        cur[0][x]       = '#';
        cur[BUF_H-1][x] = '#';
    }
    for (int y = 0; y < BUF_H; y++) {
        cur[y][0]       = '#';
        cur[y][BUF_W-1] = '#';
    }
}

static void flush_buffer(void) {
    for (int y = 0; y < BUF_H; y++) {
        for (int x = 0; x < BUF_W; x++) {
            if (cur[y][x] != prev[y][x]) {
                move_cursor(x, y);
                putchar(cur[y][x]);
                prev[y][x] = cur[y][x];
            }
        }
    }
    fflush(stdout);
}

static void clear_interior(void) {
    for (int y = 1; y <= HEIGHT; y++)
        memset(&cur[y][1], ' ', WIDTH);
}

static char grid[HEIGHT][WIDTH];

static void spawn_food(void) {
    static Point empty[MAX_SNAKE];
    int count = 0;
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            if (!grid[y][x])
                empty[count++] = (Point){x, y};

    if (!count) { gameOver = 1; return; }
    food = empty[rand() % count];
    grid[food.y][food.x] = 2;
}

static void game_init(void) {
    length   = 3;
    dir      = (Point){1, 0};
    gameOver = 0;

    memset(grid, 0, sizeof grid);

    snake[0] = (Point){ WIDTH / 2,     HEIGHT / 2 };
    snake[1] = (Point){ WIDTH / 2 - 1, HEIGHT / 2 };
    snake[2] = (Point){ WIDTH / 2 - 2, HEIGHT / 2 };
    for (int i = 0; i < length; i++)
        grid[snake[i].y][snake[i].x] = 1;

    init_buffer();
    spawn_food();
}

static void apply_direction(int c) {
    switch (c) {
        case KEY_UP:    case 'w': case 'W':
            if (dir.y ==  0) { dir.x =  0; dir.y = -1; } break;
        case KEY_DOWN:  case 's': case 'S':
            if (dir.y ==  0) { dir.x =  0; dir.y =  1; } break;
        case KEY_LEFT:  case 'a': case 'A':
            if (dir.x ==  0) { dir.x = -1; dir.y =  0; } break;
        case KEY_RIGHT: case 'd': case 'D':
            if (dir.x ==  0) { dir.x =  1; dir.y =  0; } break;
    }
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
        case 'i': game_init();       break;
        case 'q': gameOver = -1;     break;
    }
}

static void update(void) {
    Point head = { snake[0].x + dir.x, snake[0].y + dir.y };

    if (head.x < 0 || head.x >= WIDTH || head.y < 0 || head.y >= HEIGHT) {
        gameOver = 1; return;
    }
    if (grid[head.y][head.x] == 1) {
        gameOver = 1; return;
    }

    int ate = (head.x == food.x && head.y == food.y);

    if (!ate) {
        Point tail = snake[length - 1];
        grid[tail.y][tail.x] = 0;
    }

    memmove(&snake[1], &snake[0], (size_t)length * sizeof(Point));
    snake[0] = head;
    grid[head.y][head.x] = 1;

    if (ate) {
        length++;
        spawn_food();
    }
}

static void draw(void) {
    clear_interior();
    cur[food.y + 1][food.x + 1] = '*';
    for (int i = 0; i < length; i++)
        cur[snake[i].y + 1][snake[i].x + 1] = (i == 0) ? 'O' : 'o';

    flush_buffer();

    move_cursor(0, BUF_H);
    printf("Score: %-4d | WASD/Arrows=move  I=restart  Q=quit   ", length - 3);
    fflush(stdout);
}

static void draw_gameover(void) {
    const char *line1 = "  GAME  OVER!  ";
    const char *line2 = "I=Restart  Q=Quit";
    int len1 = (int)strlen(line1);
    int len2 = (int)strlen(line2);
    int cx   = BUF_W / 2;
    int cy   = BUF_H / 2;

    for (int i = 0; i < len1; i++) cur[cy - 1][cx - len1/2 + i] = line1[i];
    for (int i = 0; i < len2; i++) cur[cy    ][cx - len2/2 + i] = line2[i];

    flush_buffer();

    move_cursor(0, BUF_H);
    printf("Score: %-4d | WASD/Arrows=move  I=restart  Q=quit   ", length - 3);
    fflush(stdout);
}

static void run_game(void) {
    srand((unsigned)time(NULL));
    platform_init();
    game_init();

    for (;;) {
        if (gameOver == 0) {
            handle_input();
            if (gameOver != 0) continue;
            update();
            draw();
        } else if (gameOver == 1) {
            draw_gameover();
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

    move_cursor(0, BUF_H + 1);
    printf("\nThanks for playing!  Final score: %d\n", length - 3);
}

#endif
