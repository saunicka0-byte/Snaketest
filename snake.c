#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#define WIDTH 30
#define HEIGHT 20
#define MAX_SNAKE 200

int snakeX[MAX_SNAKE];
int snakeY[MAX_SNAKE];
int length = 3;
int foodX, foodY;
int dirX = 1;
int dirY = 0;
int gameOver = 0;

#ifdef _WIN32
HANDLE hConsole;
COORD cursorPos;

void initConsole() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Disable cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    // Set console buffer size to match game size
    COORD bufferSize = {WIDTH + 2, HEIGHT + 3};
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    
    // Set console window size
    SMALL_RECT windowSize = {0, 0, WIDTH + 1, HEIGHT + 1};
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
}

void setCursorPosition(int x, int y) {
    cursorPos.X = x;
    cursorPos.Y = y;
    SetConsoleCursorPosition(hConsole, cursorPos);
}

void clearConsole() {
    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', (WIDTH + 2) * (HEIGHT + 3), topLeft, &written);
    FillConsoleOutputAttribute(hConsole, FOREGROUND_WHITE, (WIDTH + 2) * (HEIGHT + 3), topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);
}

int kbhit_nonblocking() {
    INPUT_RECORD irBuffer;
    DWORD numEventsRead;
    
    if (!PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &irBuffer, 1, &numEventsRead)) {
        return 0;
    }
    
    if (numEventsRead && irBuffer.EventType == KEY_EVENT && irBuffer.Event.KeyEvent.bKeyDown) {
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &irBuffer, 1, &numEventsRead);
        return irBuffer.Event.KeyEvent.uChar.AsciiChar;
    }
    
    return 0;
}

#else
struct termios orig;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig);
    atexit(disableRawMode);
    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int kbhit_nonblocking() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0;
    
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

void initConsole() {
    enableRawMode();
}
#endif

void spawnFood() {
    foodX = rand() % WIDTH;
    foodY = rand() % HEIGHT;
}

void input() {
    int c = kbhit_nonblocking();
    if (c <= 0) return;
    
    if ((c == 'w' || c == 'W') && dirY == 0) { dirX = 0; dirY = -1; }
    if ((c == 's' || c == 'S') && dirY == 0) { dirX = 0; dirY = 1; }
    if ((c == 'a' || c == 'A') && dirX == 0) { dirX = -1; dirY = 0; }
    if ((c == 'd' || c == 'D') && dirX == 0) { dirX = 1; dirY = 0; }
    if (c == 'q' || c == 'Q') gameOver = 1;
}

void update() {
    for (int i = length; i > 0; i--) {
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
    }
    
    snakeX[0] += dirX;
    snakeY[0] += dirY;
    
    // Wall collision
    if (snakeX[0] < 0 || snakeX[0] >= WIDTH ||
        snakeY[0] < 0 || snakeY[0] >= HEIGHT) {
        gameOver = 1;
    }
    
    // Self collision
    for (int i = 1; i < length; i++) {
        if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
            gameOver = 1;
        }
    }
    
    // Food collision
    if (snakeX[0] == foodX && snakeY[0] == foodY) {
        length++;
        spawnFood();
    }
}

void draw() {
#ifdef _WIN32
    clearConsole();
    setCursorPosition(0, 0);
#else
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
    
    for (int y = -1; y <= HEIGHT; y++) {
        for (int x = -1; x <= WIDTH; x++) {
            if (x == -1 || x == WIDTH || y == -1 || y == HEIGHT) {
                printf("#");
                continue;
            }
            
            if (x == foodX && y == foodY) {
                printf("*");
                continue;
            }
            
            int printed = 0;
            for (int i = 0; i < length; i++) {
                if (snakeX[i] == x && snakeY[i] == y) {
                    printf(i == 0 ? "O" : "o");
                    printed = 1;
                    break;
                }
            }
            
            if (!printed) printf(" ");
        }
        printf("\n");
    }
    
    printf("Score: %d | Controls: WASD to move, Q to quit\n", length - 3);
    fflush(stdout);
}

int main() {
    srand(time(NULL));
    initConsole();
    
    snakeX[0] = WIDTH / 2;
    snakeY[0] = HEIGHT / 2;
    for (int i = 1; i < length; i++) {
        snakeX[i] = snakeX[0] - i;
        snakeY[i] = snakeY[0];
    }
    
    spawnFood();
    
    while (!gameOver) {
        input();
        update();
        draw();
        
#ifdef _WIN32
        Sleep(150);
#else
        usleep(150000);
#endif
    }
    
    printf("\nGame Over! Final score: %d\n", length - 3);
    
#ifdef _WIN32
    printf("Press any key to exit...\n");
    _getch();
#endif
    
    return 0;
}
