```c
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

#ifdef _WIN32

static HANDLE hOut, hIn;

static void platform_init(void) {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn  = GetStdHandle(STD_INPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(hOut, &ci);
    COORD buf = {BUF_W + 1, BUF_H + 3};
    SetConsoleScreenBufferSize(hOut, buf);
    SMALL_RECT win = {0, 0, BUF_W, BUF_H + 2};
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
    if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) return 0;
    switch (ir.Event.KeyEvent.wVirtualKeyCode) {
        case VK_UP:    return KEY_UP;
        case VK_DOWN:  return KEY_DOWN;
        case VK_LEFT:  return KEY_LEFT;
        case VK_RIGHT: return KEY_RIGHT;
        default:       return ir.Event.KeyEvent.uChar.AsciiChar;
    }
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
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;
    if (c != 0x1b) return (int)c;
    tv.tv_usec = 10000;
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0x1b;
    unsigned char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return 0x1b;
    if (seq[0] != '[') return 0x1b;
    tv.tv_usec = 10000;
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0x1b;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return 0x1b;
    switch (seq[1]) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return 0;
    }
}

#endif

static void init_buffer(void) {
    memset(cur,  ' ', sizeof cur);
    memset(prev, '\0', sizeof prev);
    for (int x = 0; x < BUF_W; x++) { cur[0][x] = '#'; cur[BUF_H-1][x] = '#'; }
    for (int y = 0; y < BUF_H; y++) { cur[y][0] = '#'; cur[y][BUF_W-1] = '#'; }
}

static void flush_buffer(void) {
    for (int y = 0; y < BUF_H; y++)
        for (int x = 0; x < BUF_W; x++)
            if (cur[y][x] != prev[y][x]) { move_cursor(x, y); putchar(cur[y][x]); prev[y][x] = cur[y][x]; }
    fflush(stdout);
}

static void clear_interior(void) { for (int y = 1; y <= HEIGHT; y++) memset(&cur[y][1], ' ', WIDTH); }

static char grid[HEIGHT][WIDTH];

static void spawn_food(void) {
    static Point empty[MAX_SNAKE];
    int count = 0;
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            if (!grid[y][x]) empty[count++] = (Point){x, y};
    if (!count) { gameOver = 1; return; }
    food = empty[rand() % count];
    grid[food.y][food.x] = 2;
}

static void game_init(void) {
    length = 3; dir = (Point){1,0}; gameOver=0;
    memset(grid,0,sizeof grid);
    snake[0]=(Point){WIDTH/2,HEIGHT/2}; snake[1]=(Point){WIDTH/2-1,HEIGHT/2}; snake[2]=(Point){WIDTH/2-2,HEIGHT/2};
    for(int i=0;i<length;i++) grid[snake[i].y][snake[i].x]=1;
    init_buffer(); spawn_food();
}

static void apply_direction(int c) {
    switch(c){
        case KEY_UP: case 'w': case 'W': if(dir.y==0){dir.x=0;dir.y=-1;} break;
        case KEY_DOWN: case 's': case 'S': if(dir.y==0){dir.x=0;dir.y=1;} break;
        case KEY_LEFT: case 'a': case 'A': if(dir.x==0){dir.x=-1;dir.y=0;} break;
        case KEY_RIGHT: case 'd': case 'D': if(dir.x==0){dir.x=1;dir.y=0;} break;
    }
}

static void handle_input(void) { int c=read_key(); if(!c) return; if((c|0x20)=='q'){gameOver=-1;return;} apply_direction(c); }

static void handle_input_gameover(void) { int c=read_key(); if(!c) return; switch(c|0x20){case 'i':game_init();break;case 'q':gameOver=-1;break;} }

static void update(void) {
    Point head={snake[0].x+dir.x,snake[0].y+dir.y};
    if(head.x<0||head.x>=WIDTH||head.y<0||head.y>=HEIGHT){gameOver=1;return;}
    if(grid[head.y][head.x]==1){gameOver=1;return;}
    int ate=(head.x==food.x&&head.y==food.y);
    if(!ate){Point tail=snake[length-1]; grid[tail.y][tail.x]=0;}
    memmove(&snake[1],&snake[0],(size_t)length*sizeof(Point));
    snake[0]=head; grid[head.y][head.x]=1;
    if(ate){length++;spawn_food();}
}

static void draw(void) {
    clear_interior();
    cur[food.y+1][food.x+1]='*';
    for(int i=0;i<length;i++) cur[snake[i].y+1][snake[i].x+1]=(i==0)?'O':'o';
    flush_buffer();
    move_cursor(0,BUF_H);
    printf("Score: %-4d | WASD/Arrows=move  I=restart  Q=quit   ",length-3);
    fflush(stdout);
}

static void draw_gameover(void) {
    const char *l1="  GAME  OVER!  "; const char *l2="I=Restart  Q=Quit";
    int len1=(int)strlen(l1), len2=(int)strlen(l2), cx=BUF_W/2, cy=BUF_H/2;
    for(int i=0;i<len1;i++) cur[cy-1][cx-len1/2+i]=l1[i];
    for(int i=0;i<len2;i++) cur[cy][cx-len2/2+i]=l2[i];
    flush_buffer();
    move_cursor(0,BUF_H);
    printf("Score: %-4d | WASD/Arrows=move  I=restart  Q=quit   ",length-3);
    fflush(stdout);
}

int main(void){
    srand((unsigned)time(NULL));
    platform_init(); game_init();
    for(;;){
        if(gameOver==0){handle_input(); if(gameOver!=0) continue; update(); draw(); SLEEP_MS(FRAME_MS);}
        else if(gameOver==1){draw_gameover(); handle_input_gameover(); SLEEP_MS(FRAME_MS);}
        else break;
    }
    move_cursor(0,BUF_H+1);
    printf("\nThanks for playing!  Final score: %d\n",length-3);
#ifdef _WIN32
    platform_cleanup(); printf("Press any key to exit...\n"); _getch();
#endif
    return 0;
}
```
