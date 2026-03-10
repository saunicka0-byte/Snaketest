#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <conio.h>
  #include <windows.h>
  #define SLEEP_MS(ms) Sleep(ms)
#else
  #include <unistd.h>
  #include <termios.h>
  #include <sys/select.h>
  #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

#define WIDTH      30
#define HEIGHT     20
#define MAX_SNAKE  (WIDTH * HEIGHT)
#define FRAME_MS   150

typedef struct { int x, y; } Point;

static Point  snake[MAX_SNAKE];
static int    length   = 3;
static Point  food;
static Point  dir      = {1, 0};
static int    gameOver = 0;

#define BUF_W  (WIDTH  + 2)
#define BUF_H  (HEIGHT + 2)
static char cur [BUF_H][BUF_W];
static char prev[BUF_H][BUF_W];

#ifdef _WIN32

static HANDLE hOut, hIn;

static void platform_init(void) {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn  = GetStdHandle(STD_INPUT_HANDLE);

    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(hOut, &ci);

    COORD buf = {BUF_W + 1, BUF_H + 3};
    SetConsoleScreenBufferSize(hOut, buf);
    SMALL_RECT win = {0, 0, BUF_W, BUF_H + 1};
    SetConsoleWindowInfo(hOut, TRUE, &win);
}

static void platform_cleanup(void) {
    CONSOLE_CURSOR_INFO ci = {1, TRUE};
    SetConsoleCursorInfo(hOut, &ci);
}

static void move_cursor(int x, int y) {
    COORD c = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hOut, c);
}

static int read_key(void) {
    INPUT_RECORD ir;
    DWORD n;
    if (!PeekConsoleInput(hIn, &ir, 1, &n) || !n) return 0;
    ReadConsoleInput(hIn, &ir, 1, &n);
    if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
        return ir.Event.KeyEvent.uChar.AsciiChar;
    return 0;
}

#else

static struct termios orig_term;

static void platform_cleanup(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
    printf("\033[?25h");
    fflush(stdout);
}

static void platform_init(void) {
    tcgetattr(STDIN_FILENO, &orig_term);
    atexit(platform_cleanup);

    struct termios raw = orig_term;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf("\033[?25l");
    fflush(stdout);
}

static void move_cursor(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

static int read_key(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0;
    unsigned char c;
    return (read(STDIN_FILENO, &c, 1) == 1) ? (int)c : 0;
}

#endif

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

static void handle_input(void) {
    int c = read_key();
    if (!c) return;

    switch (c | 0x20) {
        case 'w': if (dir.y ==  0) { dir.x =  0; dir.y = -1; } break;
        case 's': if (dir.y ==  0) { dir.x =  0; dir.y =  1; } break;
        case 'a': if (dir.x ==  0) { dir.x = -1; dir.y =  0; } break;
        case 'd': if (dir.x ==  0) { dir.x =  1; dir.y =  0; } break;
        case 'q': gameOver = 1; break;
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
    printf("Score: %-4d | WASD = move  Q = quit   ", length - 3);
    fflush(stdout);
}

int main(void) {
    srand((unsigned)time(NULL));
    platform_init();
    init_buffer();
    memset(grid, 0, sizeof grid);

    snake[0] = (Point){ WIDTH / 2,     HEIGHT / 2 };
    snake[1] = (Point){ WIDTH / 2 - 1, HEIGHT / 2 };
    snake[2] = (Point){ WIDTH / 2 - 2, HEIGHT / 2 };
    for (int i = 0; i < length; i++)
        grid[snake[i].y][snake[i].x] = 1;

    spawn_food();

    while (!gameOver) {
        handle_input();
        update();
        draw();
        SLEEP_MS(FRAME_MS);
    }

    move_cursor(0, BUF_H + 1);
    printf("\nGame Over!  Final score: %d\n", length - 3);

#ifdef _WIN32
    platform_cleanup();
    printf("Press any key to exit...\n");
    _getch();
#endif

    return 0;
}
