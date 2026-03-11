# Terminal Snake (C)

A simple Snake game written in C that runs in the terminal.
This project was created as a small test project for my GitHub account and to practice basic C programming.

## Features

* Real-time keyboard input
* Snake movement and growth
* Food spawning
* Collision detection (walls and self)
* Score tracking
* Simple terminal rendering
* It's Snake. What did you expect?
## Controls

| Key | Action        |
| --- | ------------- |
| W   | Move up       |
| S   | Move down     |
| A   | Move left     |
| D   | Move right    |
| Q   | Quit the game |
| I   | Restart game  |

## Requirements

* Windows, Linux or Unix-like system
* GCC compiler

## Compile (Linux, macOS)

```bash
gcc snake.c -o snake
```

## Run

```bash
./snake
```
## Compile (Windows)

1. Install MinGW-w64
2. ```bash
   86_64-w64-mingw32-gcc snake.c -O2 -o snake.exe
   ```
3. Run snake.exe

## How it Works

The game updates the snake's position in a loop, checks for collisions, and redraws the game board in the terminal.
Food is randomly generated, and each time the snake eats food, it grows longer and the score increases.

## Project Purpose

This repository is a small learning project used to:

* practice C programming
* understand terminal input/output
* experiment with real-time input handling
* test GitHub workflows

## Future Improvements

* Colored terminal graphics
* Increasing difficulty/speed
* High score saving
## Made with AI

This Project is mostly vibe-coded to learn, like said above.
## License

This project is free to use and modify.
