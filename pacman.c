#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <time.h>

#define MAP_HEIGHT 31
#define MAP_WIDTH 28

#define PACMAN_MOVE_DELAY 4
#define POWER_UP_SPAWN_INTERVAL 15000
#define POWER_UP_DURATION 10000

#ifndef CONSOLE_FONT_HEIGHT
#define CONSOLE_FONT_HEIGHT 18
#endif

#define WALL '#'
#define EMPTY ' '
#define DOT '.'
#define BIG_DOT 'O'
#define PACMAN 'P'
#define POWER_UP '^'
#define SHELLS 240

char ghostTextures[4] = { 'B', 'P', 'I', 'C' };

int level = 1;
int shells = SHELLS;
int playing = 1;

// Pacman object
struct Pacman {
    int x;
    int y;
    int lives;
    char texture;
    char direction;
    char wantedDirection;
    int isChasing;
    int speed;
};

// Ghost object
struct Ghost {
    int id;
    int x;
    int y;
    int isChasing;
    char texture;
    int direction; // 0 = Up, 1 = Right, 2 = Down, 3 = Left
    int prevX;
    int prevY;
    int frozenGhost;
    int repelledGhosts;
    ULONGLONG frozenUntil;
};

// Power-up types
enum PowerUpType {
    HUNT_GHOSTS,
    FREEZE_GHOSTS,
    DOUBLE_SPEED,
    REPEL_GHOSTS,
    SHIELD,
    DOUBLE_SCORE,
    POWER_UP_COUNT
};

// Power-up object
struct PowerUp {
    int id;
    int x;
    int y;
    bool active;
    char underlyingTile;
};

// Game object
struct Game {
    struct Pacman pacman;
    struct Ghost ghosts[4]; // 0 - Blinky, 1 - Pinky, 2 - Inky, 3 - Clyde
    int score;
    struct PowerUp powerUps[POWER_UP_COUNT]; // Power-ups
    ULONGLONG powerUpExpires[POWER_UP_COUNT]; // Power-up expiration times
    ULONGLONG nextPowerUpSpawn; // Next power-up spawn time
    char map[MAP_HEIGHT][MAP_WIDTH]; // Map tiles
};

// These functions manage power-ups, saving/loading games, and console setup.
void applyPowerUp(struct Game* game, int type);
void saveGame(struct Game* game, const char* filename);
bool loadGame(struct Game* game, const char* filename);
void setCursor(int x, int y);
char getTileAt(struct Game* game, int x, int y);
int getGhostMoveDelay(void);

// Clears the Windows console before drawing a new screen.
void clearTerminal() {
    system("cls");
}

// Sets the console size, title, and window icon.
void setupConsole(int width, int height, int fontSize) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SMALL_RECT minRect = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hOut, TRUE, &minRect);

    COORD bufferSize = { (SHORT)width, (SHORT)height };
    SetConsoleScreenBufferSize(hOut, bufferSize);

    SMALL_RECT windowRect = { 0, 0, (SHORT)(width - 1), (SHORT)(height - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &windowRect);

    SetConsoleTitle("PaCman");
    HINSTANCE instance = GetModuleHandle(NULL);
    HWND window = GetConsoleWindow();

    HICON smallIcon = (HICON)LoadImage(
        instance, MAKEINTRESOURCE(101), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    );

    HICON bigIcon = (HICON)LoadImage(
        instance, MAKEINTRESOURCE(101), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR
    );

    if (window != NULL) {
        if (smallIcon != NULL) SendMessage(window, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
        if (bigIcon != NULL) SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
    }

    // Make the game text larger.
    CONSOLE_FONT_INFOEX font = { 0 };
    font.cbSize = sizeof(font);
    font.dwFontSize.Y = (SHORT)fontSize;
    font.FontWeight = FW_NORMAL;
    wcscpy(font.FaceName, L"Consolas");

    SetCurrentConsoleFontEx(hOut, FALSE, &font);
}

// Enables console colors by setting the virtual terminal processing mode.
void enableConsoleColors(void) {
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (GetConsoleMode(output, &mode)) {
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

// Moves the console cursor to a map position.
void setCursor(int x, int y) {
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// Shows or hides the console cursor.
void setCursorVisibility(bool visible) {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

// Displays the Pac-Man logo when the program starts.
void showLogo(struct Game* game) {
    const char* pacman_art[31] = {
        "        .########.          ",
        "      .############.        ",
        "     ################       ",
        "    ##################      ",
        "   #############..#####     ",
        "  #############....#####    ",
        "  #############....#####    ",
        " ###############..#######   ",
        " ####################       ",
        " #################          ",
        " ##############             ",
        " ###########                ",
        " ########         O    O    ",
        " ########                   ",
        " ########                   ",
        " ########         O    O    ",
        " ###########                ",
        " ##############             ",
        " #################          ",
        " ####################       ",
        " ########################   ",
        "  ######################    ",
        "  ######################    ",
        "   ####################     ",
        "    ##################      ",
        "     ################       ",
        "      '############'        ",
        "        '########'          ",
        "                            ",
        "   P a C m a n   2 8 x 31   ",
        "       Juraj Ondovcik       "
    };

    for (int i = 0; i < 31; i++) {
        printf("\x1b[93m%s\x1b[0m\n", pacman_art[i]);
    }
    Sleep(2000);
    clearTerminal();
}

// Shows the start menu and returns start, load, or quit.
int showStartMenu() {
    clearTerminal();
    setCursor(0, 0);

    const char* menu[MAP_HEIGHT] = {
        "############################",
        "#                          #",
        "#      P a C - m a n       #",
        "#                          #",
        "############################",
        "#                          #",
        "#         CONTROLS         #",
        "#                          #",
        "#     W / UP    - Up       #",
        "#     S / DOWN  - Down     #",
        "#     A / LEFT  - Left     #",
        "#     D / RIGHT - Right    #",
        "#     Q         - Quit     #",
        "#     F         - Save     #",
        "############################",
        "#                          #",
        "#          ITEMS           #",
        "#                          #",
        "#   .  Dot        (10 pts) #",
        "#   O  Power Dot  (Hunt)   #",
        "#   ^  Special    (Boost)  #",
        "#                          #",
        "#   B  Blinky   P  Pinky   #",
        "#   I  Inky     C  Clyde   #",
        "#                          #",
        "#   Press ENTER to Start   #",
        "#     Press Q to Quit      #",
        "#   Press F to load Save   #",
        "#                          #",
        "#                          #",
        "############################"
    };

    for (int i = 0; i < MAP_HEIGHT; i++) {
        printf("%s\n", menu[i]);
    }
    fflush(stdout);

    //register presses on screen
    while (1) {
        int c = _getch();
        if (c == '\r' || c == ' ') { // Enter or Space
            clearTerminal();
            return 1;
        }
        if (c == 'f' || c == 'F') {
            return 2;
        }
        if (c == 'q' || c == 'Q') {
            return 0;
        }
    }
}

// Shows the end screen and handles restart, save, load, and quit.
int showEndScreen(struct Game* game, bool won) {
    clearTerminal();
    setCursor(0, 0);

    const char* screen[MAP_HEIGHT] = {
        "############################",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "############################",
        "#                          #",
        "#                          #",
        "#                          #",
        "############################",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "#                          #",
        "############################",
        "#                          #",
        "#                          #",
        "#   Press R to Restart     #",
        "#   Press P to Pause       #",
        "#   Press L to Load Save   #",
        "#   Press F to Save Game   #",
        "#                          #",
        "#                          #",
        "#                          #",
        "############################"
    };

    for (int i = 0; i < MAP_HEIGHT; i++) {
        printf("%s\n", screen[i]);
    }

    // Center the header text inside the 28-wide frame
    const char* title = won ? "Y O U   W I N !" : "G A M E   O V E R";
    setCursor((MAP_WIDTH - (int)strlen(title)) / 2, 7);
    printf("%s", title);

    // Format and center Final Score
    char scoreText[28];
    snprintf(scoreText, sizeof(scoreText), "FINAL SCORE: %d", game->score);
    setCursor((MAP_WIDTH - (int)strlen(scoreText)) / 2, 13);
    printf("%s", scoreText);

    // Format and center Level Reached
    char levelText[28];
    snprintf(levelText, sizeof(levelText), "LEVEL REACHED: %d", level);
    setCursor((MAP_WIDTH - (int)strlen(levelText)) / 2, 16);
    printf("%s", levelText);

    fflush(stdout);

    // register presses on screen
    while (1) {
        int c = _getch();
        if (c == 'r' || c == 'R') {
            return 1;
        }
        if (c == 'f' || c == 'F') {
            saveGame(game, "save.txt");
            continue;
        }
        if (c == 'l' || c == 'L') {
            return 2;
        }
        if (c == 'q' || c == 'Q' || c == '\r') {
            return 0;
        }
    }
}

// Writes the current level and game state to the save file.
void saveGame(struct Game* game, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) return;
    /*
        1 205 330 // level, shells, score
        6 29 2 a a // pacman x, pacman y, lives, direction, wanted direction
        9 1 10 1 0 // ghost0 x, ghost0 y, prev x, prev y, frozen ghost
        7 20 8 20 0 // ghost1 x, ghost1 y, prev x, prev y, frozen ghost
        9 7 9 8 0 // ghost2 x, ghost2 y, prev x, prev y, frozen ghost
        8 13 8 12 0 // ghost3 x, ghost3 y, prev x, prev y, frozen ghost
        and a map tiles to remember dots and power-ups and stuff
     */
    fprintf(file, "PACMAN_SAVE_1\n");
    fprintf(file, "%d %d %d\n", level, shells, game->score);
    fprintf(file, "%d %d %d %c %c\n", game->pacman.x, game->pacman.y,
        game->pacman.lives, game->pacman.direction, game->pacman.wantedDirection);
    for (int i = 0; i < 4; i++) {
        fprintf(file, "%d %d %d %d %d\n", game->ghosts[i].x, game->ghosts[i].y,
            game->ghosts[i].prevX, game->ghosts[i].prevY, game->ghosts[i].frozenGhost);
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        fwrite(game->map[y], sizeof(char), MAP_WIDTH, file);
        fputc('\n', file);
    }
    fclose(file);
}

// Loads a saved level and game state from the save file.
bool loadGame(struct Game* game, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) return false;

    char line[256];
    if (fgets(line, sizeof(line), file) == NULL || strcmp(line, "PACMAN_SAVE_1\n") != 0) {
        fclose(file);
        return false;
    }
    if (fscanf(file, "%d %d %d\n", &level, &shells, &game->score) != 3 ||
        fscanf(file, "%d %d %d %c %c\n", &game->pacman.x, &game->pacman.y,
            &game->pacman.lives, &game->pacman.direction, &game->pacman.wantedDirection) != 5) {
        fclose(file);
        return false;
    }

    game->pacman.texture = PACMAN;
    game->pacman.isChasing = 0;
    game->pacman.speed = 1;
    for (int i = 0; i < 4; i++) {
        if (fscanf(file, "%d %d %d %d %d\n", &game->ghosts[i].x, &game->ghosts[i].y,
            &game->ghosts[i].prevX, &game->ghosts[i].prevY,
            &game->ghosts[i].frozenGhost) != 5) {
            fclose(file);
            return false;
        }
        game->ghosts[i].id = i;
        game->ghosts[i].texture = ghostTextures[i];
        game->ghosts[i].repelledGhosts = 0;
        game->ghosts[i].frozenUntil = 0;
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        if (fread(game->map[y], sizeof(char), MAP_WIDTH, file) != MAP_WIDTH) {
            fclose(file);
            return false;
        }
        fgetc(file);
    }
    for (int i = 0; i < POWER_UP_COUNT; i++) {
        game->powerUps[i].active = false;
        game->powerUpExpires[i] = 0;
    }
    game->nextPowerUpSpawn = GetTickCount64() + POWER_UP_SPAWN_INTERVAL;
    fclose(file);
    return true;
}

// Makes ghosts move faster as the level increases.
int getGhostMoveDelay(void) {
    int delay = PACMAN_MOVE_DELAY - ((level - 1) / 2);
    if (delay < 1) delay = 1;
    return delay;
}

// Checks whether another ghost occupies a map position.
bool isGhostAt(struct Game* game, int ghostIndex, int x, int y) {
    for (int i = 0; i < 4; i++) {
        if (i != ghostIndex && game->ghosts[i].x == x && game->ghosts[i].y == y) {
            return true;
        }
    }
    return false;
}

// Reads one keyboard input without blocking the game loop.
char readMove() {
    if (_kbhit()) {
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            c = _getch();
            switch (c) {
                case 72: return 'w';
                case 80: return 's';
                case 75: return 'a';
                case 77: return 'd';
            }
            return 0;
        }
        return (char)c;
    }
    return 0;
}

// Moves Pac-Man and handles dots, power dots, and power-ups.
void movePacman(struct Game* game, char m) {
    int nextX = game->pacman.x;
    int nextY = game->pacman.y;

    // Check if the move is valid - not a wall
    if (m == 'w' && nextY > 0 && game->map[nextY - 1][nextX] != WALL) nextY--;
    else if (m == 's' && nextY < MAP_HEIGHT - 1 && game->map[nextY + 1][nextX] != WALL) nextY++;
    else if (m == 'a' && nextX > 0 && game->map[nextY][nextX - 1] != WALL) nextX--;
    else if (m == 'd' && nextX < MAP_WIDTH - 1 && game->map[nextY][nextX + 1] != WALL) nextX++;
    else return;

    // Check if the next move is valid
    if (nextY >= 0 && nextY < MAP_HEIGHT && nextX >= 0 && nextX < MAP_WIDTH) {
        char targetTile = game->map[nextY][nextX];
        if (targetTile != WALL) {
            game->pacman.x = nextX;
            game->pacman.y = nextY;

            // Check if Pac-Man stepped on an active power-up
            if (targetTile == POWER_UP) {
                for (int i = 0; i < POWER_UP_COUNT; i++) {
                    struct PowerUp* powerUp = &game->powerUps[i];
                    if (powerUp->active && powerUp->x == nextX && powerUp->y == nextY) {
                        targetTile = powerUp->underlyingTile;
                        powerUp->active = false;
                        game->map[nextY][nextX] = targetTile;
                        applyPowerUp(game, powerUp->id);
                        break;
                    }
                }
            }

            // Check if the tile has a power-up or dot to collect
            if (targetTile == DOT) {
                game->score += (10 * level) *
                    (game->powerUpExpires[DOUBLE_SCORE] > GetTickCount64() ? 2 : 1);
                shells--;
                game->map[nextY][nextX] = EMPTY;
            } else if (targetTile == BIG_DOT) {
                game->score += (50 * level) *
                    (game->powerUpExpires[DOUBLE_SCORE] > GetTickCount64() ? 2 : 1);
                game->map[nextY][nextX] = EMPTY;
                applyPowerUp(game, HUNT_GHOSTS); // Walking on 'O' lets Pac-Man chase ghosts
            }
        }
    }
}

// Returns the simple grid distance between two positions.
int calculateDistance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

// Finds whichever of the 4 map corners is closest to the ghost.
void getClosestCorner(int gx, int gy, int* cornerX, int* cornerY) {
    int corners[4][2] = {
        { 1, 1 },
        { MAP_WIDTH - 2, 1 },
        { 1, MAP_HEIGHT - 2 },
        { MAP_WIDTH - 2, MAP_HEIGHT - 2 }
    };

    int bestDist = 999999;
    for (int i = 0; i < 4; i++) {
        int d = calculateDistance(gx, gy, corners[i][0], corners[i][1]);
        if (d < bestDist) {
            bestDist = d;
            *cornerX = corners[i][0];
            *cornerY = corners[i][1];
        }
    }
}

// Chooses the next direction for a ghost to move.
int decideGhostMove(struct Game* game, int ghostIndex, int targetX, int targetY) {
    int gx = game->ghosts[ghostIndex].x;
    int gy = game->ghosts[ghostIndex].y;
    int prevX = game->ghosts[ghostIndex].prevX;
    int prevY = game->ghosts[ghostIndex].prevY;

    // Fleeing mode (random valid move so ghosts split up)
    if (game->pacman.isChasing && !game->ghosts[ghostIndex].repelledGhosts) {
        int validMoves[4];
        int count = 0;

        // Find valid moves, excluding walls and other ghosts, and avoiding the previous move (bouncing)
        if (gy > 0 && game->map[gy - 1][gx] != WALL && !isGhostAt(game, ghostIndex, gx, gy - 1) && !(gx == prevX && gy - 1 == prevY)) validMoves[count++] = 0;
        if (gy < MAP_HEIGHT - 1 && game->map[gy + 1][gx] != WALL && !isGhostAt(game, ghostIndex, gx, gy + 1) && !(gx == prevX && gy + 1 == prevY)) validMoves[count++] = 2;
        if (gx > 0 && game->map[gy][gx - 1] != WALL && !isGhostAt(game, ghostIndex, gx - 1, gy) && !(gx - 1 == prevX && gy == prevY)) validMoves[count++] = 3;
        if (gx < MAP_WIDTH - 1 && game->map[gy][gx + 1] != WALL && !isGhostAt(game, ghostIndex, gx + 1, gy) && !(gx + 1 == prevX && gy == prevY)) validMoves[count++] = 1;

        if (count > 0) {
            return validMoves[rand() % count];
        }
    } else {
        // Normal chase / repel-to-corner mode
        int distance = 999999;
        int nextMove = -1;

        // Find the closest valid move towards the target
        if (gy > 0 && game->map[gy - 1][gx] != WALL && !isGhostAt(game, ghostIndex, gx, gy - 1) && !(gx == prevX && gy - 1 == prevY)) {
            int upDistance = calculateDistance(gx, gy - 1, targetX, targetY);
            if (upDistance < distance) { distance = upDistance; nextMove = 0; }
        }
        if (gy < MAP_HEIGHT - 1 && game->map[gy + 1][gx] != WALL && !isGhostAt(game, ghostIndex, gx, gy + 1) && !(gx == prevX && gy + 1 == prevY)) {
            int downDistance = calculateDistance(gx, gy + 1, targetX, targetY);
            if (downDistance < distance) { distance = downDistance; nextMove = 2; }
        }
        if (gx > 0 && game->map[gy][gx - 1] != WALL && !isGhostAt(game, ghostIndex, gx - 1, gy) && !(gx - 1 == prevX && gy == prevY)) {
            int leftDistance = calculateDistance(gx - 1, gy, targetX, targetY);
            if (leftDistance < distance) { distance = leftDistance; nextMove = 3; }
        }
        if (gx < MAP_WIDTH - 1 && game->map[gy][gx + 1] != WALL && !isGhostAt(game, ghostIndex, gx + 1, gy) && !(gx + 1 == prevX && gy == prevY)) {
            int rightDistance = calculateDistance(gx + 1, gy, targetX, targetY);
            if (rightDistance < distance) { distance = rightDistance; nextMove = 1; }
        }

        if (nextMove != -1) {
            return nextMove;
        }
    }

    // If no valid move was found, return the opposite of the previous move to avoid bouncing
    if (prevY < gy && !isGhostAt(game, ghostIndex, gx, prevY)) return 0;
    if (prevY > gy && !isGhostAt(game, ghostIndex, gx, prevY)) return 2;
    if (prevX < gx && !isGhostAt(game, ghostIndex, prevX, gy)) return 3;
    if (prevX > gx && !isGhostAt(game, ghostIndex, prevX, gy)) return 1;

    return -1;
}

// Moves one ghost toward its target unless it is frozen.
void moveGhostToTarget(struct Game* game, int ghostIndex, int targetX, int targetY) {
    ULONGLONG now = GetTickCount64(); // Get the current time

    // If the ghost is frozen, return early
    if (game->ghosts[ghostIndex].frozenGhost || now < game->ghosts[ghostIndex].frozenUntil) {
        return;
    }

    // If the ghost is repelled, move to the closest corner
    if (game->ghosts[ghostIndex].repelledGhosts) {
        getClosestCorner(game->ghosts[ghostIndex].x, game->ghosts[ghostIndex].y, &targetX, &targetY);
    } else if (game->pacman.isChasing) {
        targetX = game->pacman.x;
        targetY = game->pacman.y;
    }

    // Decide the next move and move if valid
    int nextMove = decideGhostMove(game, ghostIndex, targetX, targetY);
    if (nextMove != -1) {
        int nextX = game->ghosts[ghostIndex].x + (nextMove == 1 ? 1 : (nextMove == 3 ? -1 : 0));
        int nextY = game->ghosts[ghostIndex].y + (nextMove == 2 ? 1 : (nextMove == 0 ? -1 : 0));

        // Check if the next position is valid and not a wall or another ghost
        if (nextY >= 0 && nextY < MAP_HEIGHT && nextX >= 0 && nextX < MAP_WIDTH) {
            if (game->map[nextY][nextX] != WALL && !isGhostAt(game, ghostIndex, nextX, nextY)) {
                game->ghosts[ghostIndex].prevX = game->ghosts[ghostIndex].x;
                game->ghosts[ghostIndex].prevY = game->ghosts[ghostIndex].y;
                game->ghosts[ghostIndex].x = nextX;
                game->ghosts[ghostIndex].y = nextY;
            }
        }
    }
}

// Moves Blinky toward Pac-Man.
void moveBlinky(struct Game* game) {
    moveGhostToTarget(game, 0, game->pacman.x, game->pacman.y);
}

// Moves Pinky toward a position ahead of Pac-Man.
void movePinky(struct Game* game) {
    int targetX = game->pacman.x;
    int targetY = game->pacman.y;

    if (game->pacman.direction == 'w') targetY -= 4;
    else if (game->pacman.direction == 's') targetY += 4;
    else if (game->pacman.direction == 'a') targetX -= 4;
    else if (game->pacman.direction == 'd') targetX += 4;

    moveGhostToTarget(game, 1, targetX, targetY);
}

// Moves Inky using Pac-Man's position and Blinky's position.
void moveInky(struct Game* game) {
    int pivotX = game->pacman.x;
    int pivotY = game->pacman.y;

    if (game->pacman.direction == 'w') pivotY -= 2;
    else if (game->pacman.direction == 's') pivotY += 2;
    else if (game->pacman.direction == 'a') pivotX -= 2;
    else if (game->pacman.direction == 'd') pivotX += 2;

    int targetX = pivotX + (pivotX - game->ghosts[0].x);
    int targetY = pivotY + (pivotY - game->ghosts[0].y);

    moveGhostToTarget(game, 2, targetX, targetY);
}

// Moves Clyde toward Pac-Man or back toward his corner.
void moveClyde(struct Game* game) {
    int dist = calculateDistance(game->ghosts[3].x, game->ghosts[3].y, game->pacman.x, game->pacman.y);
    if (dist > 8) {
        moveGhostToTarget(game, 3, game->pacman.x, game->pacman.y);
    } else {
        moveGhostToTarget(game, 3, 1, MAP_HEIGHT - 2);
    }
}

// Fills the game map with the original level layout.
void constructMap(struct Game* game) {
    const char* map[MAP_HEIGHT] = {
        "############################",
        "#............##............#",
        "#.####.#####.##.#####.####.#",
        "#O####.#####.##.#####.####O#",
        "#.####.#####.##.#####.####.#",
        "#..........................#",
        "#.####.##.########.##.####.#",
        "#.####.##.########.##.####.#",
        "#......##....##....##......#",
        "######.##### ## #####.######",
        "     #.##### ## #####.#     ",
        "     #.#            #.#     ",
        "     #.#            #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.#            #.#     ",
        "######.## ######## ##.######",
        "#............##............#",
        "#.####.#####.##.#####.####.#",
        "#.####.#####.##.#####.####.#",
        "#O..##.......  .......##..O#",
        "###.##.##.########.##.##.###",
        "###.##.##.########.##.##.###",
        "#......##....##....##......#",
        "#.##########.##.##########.#",
        "#.##########.##.##########.#",
        "#..........................#",
        "############################"
    };

    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            game->map[i][j] = map[i][j];
        }
    }
}

// Resets player and ghost positions while keeping Pac-Man's lives.
void resetPositions(struct Game* game) {
    int lives = game->pacman.lives;
    game->pacman = (struct Pacman){ .x = 13, .y = 23, .lives = lives, .texture = PACMAN, .direction = 0, .isChasing = 0, .speed = 1 };
    for (int i = 0; i < POWER_UP_COUNT; i++) {
        game->powerUpExpires[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        game->ghosts[i] = (struct Ghost){
            .id = i,
            .x = 12 + i,
            .y = 12,
            .isChasing = 1,
            .texture = ghostTextures[i],
            .direction = 0,
            .prevX = -1,
            .prevY = -1,
            .frozenGhost = 0,
            .repelledGhosts = 0,
            .frozenUntil = 0
        };
    }
}

// Starts a completely new game with a fresh score and three lives.
void startNewGame(struct Game* game) {
    game->score = 0;
    game->pacman.lives = 3;
    for (int i = 0; i < POWER_UP_COUNT; i++) {
        game->powerUps[i] = (struct PowerUp){ .id = i, .active = false };
    }
    resetPositions(game);
    game->nextPowerUpSpawn = GetTickCount64() + POWER_UP_SPAWN_INTERVAL;
}

// Draws one visible tile with the color belonging to its game entity.
void printTileAt(struct Game* game, int x, int y) {
    static const int ghostColors[4] = { 31, 95, 96, 33 }; // red, pink, cyan, orange/yellow
    char tile = game->map[y][x];
    int color = 0;

    if (game->pacman.x == x && game->pacman.y == y) {
        tile = game->pacman.texture;
        color = 93; // bright yellow Pac-Man
    } else {
        for (int i = 0; i < 4; i++) {
            if (game->ghosts[i].x == x && game->ghosts[i].y == y) {
                tile = game->ghosts[i].texture;
                color = game->ghosts[i].isChasing ? ghostColors[i] : 94; // frightened ghosts are blue
                break;
            }
        }

        if (color == 0 && (tile == POWER_UP || tile == BIG_DOT)) {
            color = 92; // bright green power-ups and power pellets
        }
    }

    if (color != 0) printf("\x1b[%dm%c\x1b[0m", color, tile);
    else printf("%c", tile);
}

// Shows the score and currently active power-ups below the map.
void printHud(struct Game* game) {
    static const char* powerUpNames[POWER_UP_COUNT] = {
        "Hunt", "Freeze", "Speed", "Repel", "Shield", "2x Score"
    };
    char scoreLine[64];
    char activeLine[96] = "Active:";
    char firstLine[41] = { 0 };
    char secondLine[41] = { 0 };
    size_t used = strlen(activeLine);
    int activeCount = 0;
    ULONGLONG now = GetTickCount64();

    snprintf(scoreLine, sizeof(scoreLine), "Score: %d   Lives: %d", game->score, game->pacman.lives);
    setCursor(0, MAP_HEIGHT);
    printf("%-40.40s", scoreLine);

    for (int i = 0; i < POWER_UP_COUNT; i++) {
        if (game->powerUpExpires[i] > now) {
            int written = snprintf(activeLine + used, sizeof(activeLine) - used,
                "%s%s", activeCount ? ", " : " ", powerUpNames[i]);
            if (written < 0 || (size_t)written >= sizeof(activeLine) - used) break;
            used += (size_t)written;
            activeCount++;
        }
    }
    if (!activeCount) {
        strcpy(activeLine, "Active: none");
    }

    size_t activeLength = strlen(activeLine);
    size_t split = activeLength > 40 ? 39 : activeLength;
    if (activeLength > 40) {
        while (split > 0 && activeLine[split] != ' ') split--;
        if (split == 0) split = 39;
        memcpy(firstLine, activeLine, split);
        firstLine[split] = '\0';
        const char* remainder = activeLine + split;
        while (*remainder == ' ') remainder++;
        snprintf(secondLine, sizeof(secondLine), "%s", remainder);
    } else {
        snprintf(firstLine, sizeof(firstLine), "%s", activeLine);
    }

    setCursor(0, MAP_HEIGHT + 1);
    printf("%-40.40s", firstLine);
    setCursor(0, MAP_HEIGHT + 2);
    printf("%-40.40s", secondLine);
    fflush(stdout);
}

// Draws the map, Pac-Man, ghosts, score, lives, and active power-ups.
void printMap(struct Game* game) {
    setCursor(0, 0);
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            printTileAt(game, j, i);
        }
        printf("\n");
    }
    printHud(game);
}

// Returns the visible tile at a map position.
char getTileAt(struct Game* game, int x, int y) {
    if (game->pacman.x == x && game->pacman.y == y) return game->pacman.texture;
    for (int i = 0; i < 4; i++) {
        if (game->ghosts[i].x == x && game->ghosts[i].y == y) return game->ghosts[i].texture;
    }
    return game->map[y][x];
}

// Checks collisions and lets Pac-Man eat a ghost while hunting.
bool checkGhostCollision(struct Game* game) {
    for (int i = 0; i < 4; i++) {
        if (game->ghosts[i].x == game->pacman.x && game->ghosts[i].y == game->pacman.y) {
            if (!game->pacman.isChasing) {
                if (game->powerUpExpires[SHIELD] > GetTickCount64()) {
                    int pacmanX = game->pacman.x;
                    int pacmanY = game->pacman.y;
                    game->powerUpExpires[SHIELD] = 0;
                    game->ghosts[i].x = 12 + i;
                    game->ghosts[i].y = 12;
                    game->ghosts[i].prevX = -1;
                    game->ghosts[i].prevY = -1;
                    game->ghosts[i].frozenUntil = GetTickCount64() + 1000;
                    setCursor(pacmanX, pacmanY);
                    printTileAt(game, pacmanX, pacmanY);
                    setCursor(game->ghosts[i].x, game->ghosts[i].y);
                    printTileAt(game, game->ghosts[i].x, game->ghosts[i].y);
                    printHud(game);
                    return false;
                }
                return true;
            }

            // Eat ghost: award points, return to spawn, and freeze for 1 second (1000 ms)
            game->score += 200 * level *
                (game->powerUpExpires[DOUBLE_SCORE] > GetTickCount64() ? 2 : 1);
            game->ghosts[i].x = 12 + i;
            game->ghosts[i].y = 12;
            game->ghosts[i].prevX = -1;
            game->ghosts[i].prevY = -1;
            game->ghosts[i].frozenUntil = GetTickCount64() + 1000;

            setCursor(game->ghosts[i].x, game->ghosts[i].y);
            printTileAt(game, game->ghosts[i].x, game->ghosts[i].y);
            printHud(game);
        }
    }
    return false;
}

// Places a random special power-up on an available map tile.
void spawnPowerUp(struct Game* game) {
    // Pick a random power-up type (FREEZE_GHOSTS, DOUBLE_SPEED, or REPEL_GHOSTS)
    int type = 1 + (rand() % (POWER_UP_COUNT - 1));

    // If that type is already sitting on the map, replace it cleanly
    if (game->powerUps[type].active) {
        int oldX = game->powerUps[type].x;
        int oldY = game->powerUps[type].y;
        game->map[oldY][oldX] = game->powerUps[type].underlyingTile;
        setCursor(oldX, oldY);
        printTileAt(game, oldX, oldY);
    }

    for (int attempt = 0; attempt < 200; attempt++) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;

        // Avoid outer blank spaces on tunnel rows (10-18) and avoid Pac-Man
        if ((y >= 10 && y <= 18) && (x < 5 || x > 22)) continue;
        if (x == game->pacman.x && y == game->pacman.y) continue;

        char tile = game->map[y][x];
        if (tile == DOT || tile == EMPTY) {
            game->powerUps[type].id = type;
            game->powerUps[type].x = x;
            game->powerUps[type].y = y;
            game->powerUps[type].active = true;
            game->powerUps[type].underlyingTile = tile;

            game->map[y][x] = POWER_UP;
            setCursor(x, y);
            printTileAt(game, x, y);
            fflush(stdout);
            return;
        }
    }
}

// Activates the selected power-up effect.
void applyPowerUp(struct Game* game, int type) {
    ULONGLONG expireTime = GetTickCount64() + POWER_UP_DURATION;
    game->powerUpExpires[type] = expireTime;

    if (type == HUNT_GHOSTS) {
        game->pacman.isChasing = 1;
        for (int i = 0; i < 4; i++) {
            game->ghosts[i].isChasing = 0;
            game->ghosts[i].texture = (char)tolower(ghostTextures[i]); // 'b', 'p', 'i', 'c'
            game->ghosts[i].prevX = -1; // Allow immediate 180-degree turn
            game->ghosts[i].prevY = -1;
            setCursor(game->ghosts[i].x, game->ghosts[i].y);
            printTileAt(game, game->ghosts[i].x, game->ghosts[i].y);
        }
        fflush(stdout);
    } else if (type == FREEZE_GHOSTS) {
        for (int i = 0; i < 4; i++) {
            game->ghosts[i].frozenGhost = 1;
        }
    } else if (type == DOUBLE_SPEED) {
        game->pacman.speed = 2;
    } else if (type == REPEL_GHOSTS) {
        for (int i = 0; i < 4; i++) {
            game->ghosts[i].repelledGhosts = 1;
            game->ghosts[i].prevX = -1;
            game->ghosts[i].prevY = -1;
        }
    }
}

// Spawns power-ups and removes effects after their duration ends.
void updatePowerUps(struct Game* game) {
    ULONGLONG now = GetTickCount64();

    // 1. Spawn a new power-up every 15 seconds
    bool hudChanged = false;

    if (now >= game->nextPowerUpSpawn) {
        spawnPowerUp(game);
        game->nextPowerUpSpawn = now + POWER_UP_SPAWN_INTERVAL;
    }

    // Check if any active power-up effect has expired
    if (game->powerUpExpires[HUNT_GHOSTS] > 0 && now >= game->powerUpExpires[HUNT_GHOSTS]) {
        game->powerUpExpires[HUNT_GHOSTS] = 0;
        hudChanged = true;
        game->pacman.isChasing = 0;
        for (int i = 0; i < 4; i++) {
            game->ghosts[i].isChasing = 1;
            game->ghosts[i].texture = ghostTextures[i];
            setCursor(game->ghosts[i].x, game->ghosts[i].y);
            printTileAt(game, game->ghosts[i].x, game->ghosts[i].y);
        }
        fflush(stdout);
    }

    // Check if the freeze power-up has expired and remove the effect
    if (game->powerUpExpires[FREEZE_GHOSTS] > 0 && now >= game->powerUpExpires[FREEZE_GHOSTS]) {
        game->powerUpExpires[FREEZE_GHOSTS] = 0;
        hudChanged = true;
        for (int i = 0; i < 4; i++) {
            game->ghosts[i].frozenGhost = 0;
        }
    }

    // Check if the double speed power-up has expired and remove the effect
    if (game->powerUpExpires[DOUBLE_SPEED] > 0 && now >= game->powerUpExpires[DOUBLE_SPEED]) {
        game->powerUpExpires[DOUBLE_SPEED] = 0;
        hudChanged = true;
        game->pacman.speed = 1;
    }

    // Check if the repel power-up has expired and remove the effect
    if (game->powerUpExpires[REPEL_GHOSTS] > 0 && now >= game->powerUpExpires[REPEL_GHOSTS]) {
        game->powerUpExpires[REPEL_GHOSTS] = 0;
        hudChanged = true;
        for (int i = 0; i < 4; i++) {
            game->ghosts[i].repelledGhosts = 0;
        }
    }

    if (game->powerUpExpires[SHIELD] > 0 && now >= game->powerUpExpires[SHIELD]) {
        game->powerUpExpires[SHIELD] = 0;
        hudChanged = true;
    }

    if (game->powerUpExpires[DOUBLE_SCORE] > 0 && now >= game->powerUpExpires[DOUBLE_SCORE]) {
        game->powerUpExpires[DOUBLE_SCORE] = 0;
        hudChanged = true;
    }

    if (hudChanged) printHud(game);
}

// Runs menus, the game loop, level changes, and the end screen.
int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL)); // Seed the random number generator
    struct Game g = { 0 }; // Initialize the game state
    setupConsole(45, MAP_HEIGHT + 6, CONSOLE_FONT_HEIGHT); // Set up the console with room around the map
    enableConsoleColors();
    clearTerminal();
    setCursorVisibility(false);
    showLogo(&g); // Show the logo

    // Show the start menu and get the user's choice
    int startChoice = showStartMenu();
    if (startChoice == 0) {
        setCursorVisibility(true);
        return 0;
    }

    // Initialize the game state based on the user's choice
    bool running = true;
    bool loadedGame = false;
    if (startChoice == 2) {
        constructMap(&g);
        loadedGame = loadGame(&g, "save.txt");
    }

    // Main game loop
    while (running) {
        if (!loadedGame) {
            level = 1;
            shells = SHELLS;
            playing = 1;
            constructMap(&g);
            startNewGame(&g);
        } else {
            playing = 1;
            loadedGame = false;
        }
        clearTerminal();
        printMap(&g);

        int pacmanMoveTimer = 0; //timers
        int ghostMoveTimer = 0;
        bool quitRequested = false;

        while (playing) {
            if (shells == 0) {
                // Start the next level while keeping score and lives.
                level++;
                constructMap(&g);
                for (int i = 0; i < POWER_UP_COUNT; i++) {
                    g.powerUps[i] = (struct PowerUp){ .id = i, .active = false };
                    g.powerUpExpires[i] = 0;
                }
                shells = SHELLS;
                resetPositions(&g);
                g.nextPowerUpSpawn = GetTickCount64() + POWER_UP_SPAWN_INTERVAL;
                pacmanMoveTimer = 0;
                ghostMoveTimer = 0;
                clearTerminal();
                printMap(&g);
                continue;
            }

            updatePowerUps(&g);

            if (checkGhostCollision(&g)) {
                g.pacman.lives--;
                if (g.pacman.lives == 0) {
                    playing = 0;
                    printMap(&g);
                    break;
                }
                resetPositions(&g);
                pacmanMoveTimer = 0;
                ghostMoveTimer = 0;
                printMap(&g);
                Sleep(2000);
                continue;
            }

            char move = readMove();
            if (move == 'q' || move == 'Q') {
                break;
            }

            // F saves during the game. Loading is only available at the start or end.
            if (move == 'f' || move == 'F') {
                saveGame(&g, "save.txt");
                continue;
            }

            if (move == 'w' || move == 'a' || move == 's' || move == 'd') {
                g.pacman.wantedDirection = move;
            }

            // Pac-Man moves twice as fast when speed == 2 (every 2 ticks instead of 4)
            int pacmanDelay = PACMAN_MOVE_DELAY / g.pacman.speed;
            if (++pacmanMoveTimer >= pacmanDelay) {
                pacmanMoveTimer = 0;

                int oldPacmanX = g.pacman.x;
                int oldPacmanY = g.pacman.y;

                movePacman(&g, g.pacman.wantedDirection);
                if (oldPacmanX == g.pacman.x && oldPacmanY == g.pacman.y) {
                    movePacman(&g, g.pacman.direction);
                } else {
                    g.pacman.direction = g.pacman.wantedDirection;
                }

                if (oldPacmanX != g.pacman.x || oldPacmanY != g.pacman.y) {
                    setCursor(oldPacmanX, oldPacmanY);
                    printTileAt(&g, oldPacmanX, oldPacmanY);

                    setCursor(g.pacman.x, g.pacman.y);
                    printTileAt(&g, g.pacman.x, g.pacman.y);

                    printHud(&g);
                }

                if (checkGhostCollision(&g)) {
                    continue;
                }
            }

            // Ghosts move on the standard 4-tick cadence
            if (++ghostMoveTimer >= getGhostMoveDelay()) {
                ghostMoveTimer = 0;

                int oldGhostX[4], oldGhostY[4];
                for (int i = 0; i < 4; i++) {
                    oldGhostX[i] = g.ghosts[i].x;
                    oldGhostY[i] = g.ghosts[i].y;
                }

                moveBlinky(&g);
                movePinky(&g);
                moveInky(&g);
                moveClyde(&g);

                for (int i = 0; i < 4; i++) {
                    if (oldGhostX[i] != g.ghosts[i].x || oldGhostY[i] != g.ghosts[i].y) {
                        setCursor(oldGhostX[i], oldGhostY[i]);
                        printTileAt(&g, oldGhostX[i], oldGhostY[i]);

                        setCursor(g.ghosts[i].x, g.ghosts[i].y);
                        printTileAt(&g, g.ghosts[i].x, g.ghosts[i].y);
                    }
                }

                if (checkGhostCollision(&g)) {
                    g.pacman.lives--;
                    if (g.pacman.lives == 0) {
                        playing = 0;
                        printMap(&g);
                        break;
                    }
                    resetPositions(&g);
                    pacmanMoveTimer = 0;
                    ghostMoveTimer = 0;
                    printMap(&g);
                    Sleep(2000);
                    continue;
                }
                fflush(stdout);
            }

            Sleep(50);
        }

        if (quitRequested) {
            break;
        }

        bool won = (shells == 0);
        int endChoice = showEndScreen(&g, won);
        if (endChoice == 0) {
            running = false;
        } else if (endChoice == 1) {
            loadedGame = false;
        } else if (endChoice == 2) {
            constructMap(&g);
            loadedGame = loadGame(&g, "save.txt");
        }
    }

    clearTerminal();
    setCursor(0, 0);
    setCursorVisibility(true);
    return 0;
}
