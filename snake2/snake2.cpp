/*
* Snake Game Project
<<<<<<< HEAD
*
* tails
*
=======
* By: Eleek Zhang, Daniel Chen, Sean Lin, & Brian Dietz
* Due Date: April 3rd, 2025
>>>>>>> 3086b9df577c71bce551b6f4edd2a67f4a592403
*/
#include<iostream>
#include<random>
#include <ctime>
#include <cmath>
#include<windows.h>
#include<conio.h> // for keyboard inputs
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;

const int height = 20;
const int width = 30;
bool gameover;
int foodx, foody, score;
int gameSpeed;
int difficultyMode;

enum eDirection { STOP = 0, UP, DOWN, LEFT, RIGHT };
eDirection dir;

class SnakeBody {
private:
    int x, y;
    int prevX, prevY;

public:
    SnakeBody(int x = 0, int y = 0) : x(x), y(y), prevX(x), prevY(y) {}

    void updatePosition(int newX, int newY) {
        prevX = x;
        prevY = y;
        x = newX;
        y = newY;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    int getPrevX() const { return prevX; }
    int getPrevY() const { return prevY; }
};

class Food {
private:
    int x, y;

public:
    Food(int x = 0, int y = 0) : x(x), y(y) {}

    void getFoodPosition(int& foodX, int& foodY) const {
        foodX = x;
        foodY = y;
    }

    void respawn(int maxWidth, int maxHeight) {
        x = rand() % maxWidth;
        y = rand() % height;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }
};

class Snake {
private:
    SnakeBody body[100];
    int ntail;
    eDirection direction;

public:
    Snake(int startX, int startY) {
        direction = STOP;
        ntail = 0;
        body[0] = SnakeBody(startX, startY);
    }

    void move() {
        for (int i = ntail; i > 0; i--) {
            body[i] = body[i - 1];
        }

        int headX = body[0].getX();
        int headY = body[0].getY();

        switch (direction) {
        case UP:
            body[0].updatePosition(headX, headY - 1);
            break;
        case DOWN:
            body[0].updatePosition(headX, headY + 1);
            break;
        case LEFT:
            body[0].updatePosition(headX - 1, headY);
            break;
        case RIGHT:
            body[0].updatePosition(headX + 1, headY);
            break;
        default:
            break;
        }
    }

    bool checkCollision() {
        int headX = body[0].getX();
        int headY = body[0].getY();

        if (headX < 0 || headX >= width || headY < 0 || headY >= height) {
            return true;
        }

        for (int i = 1; i <= ntail; i++) {
            if (headX == body[i].getX() && headY == body[i].getY()) {
                return true;
            }
        }

        return false;
    }

    void eatFood() {
        ntail++;
    }

    int getHeadX() const { return body[0].getX(); }
    int getHeadY() const { return body[0].getY(); }
    int getNTail() const { return ntail; }

    eDirection getDirection() const { return direction; }
    void setDirection(eDirection newDir) { direction = newDir; }

    const SnakeBody& getBodyPart(int index) const {
        return body[index];
    }
};

Snake* snake = nullptr;
Food* food = nullptr;

void setup() {
    SetConsoleOutputCP(437);

    gameover = false;
    score = 0;
    dir = STOP;

    snake = new Snake(width / 2, height / 2);

    srand(static_cast<unsigned>(time(0)));
    food = new Food(rand() % width, rand() % height);
    foodx = food->getX();
    foody = food->getY();

    cout << "Snake Game with Double-Line Borders" << endl;
    cout << "Use WASD or arrow keys to move the snake." << endl;
    cout << "Press X to quit the game." << endl;
    cout << "Press 1 to play in difficult mode, 2 to play in easy mode." << endl;
    cin >> difficultyMode;

    // Set game speed based on difficulty
    gameSpeed = (difficultyMode == 1) ? 60 : 200;
}

void input() {
    if (_kbhit()) {
        char key = _getch();
        switch (key) {
        case 'w':
        case 'W':
        case 72: // ASCII code for up arrow key
            if (dir != DOWN) {
                dir = UP;
                snake->setDirection(UP);
            }
            break;
        case 's':
        case 'S':
        case 80: // ASCII code for down arrow key
            if (dir != UP) {
                dir = DOWN;
                snake->setDirection(DOWN);
            }
            break;
        case 'a':
        case 'A':
        case 75: // ASCII code for left arrow key
            if (dir != RIGHT) {
                dir = LEFT;
                snake->setDirection(LEFT);
            }
            break;
        case 'd':
        case 'D':
        case 77: // ASCII code for right arrow key
            if (dir != LEFT) {
                dir = RIGHT;
                snake->setDirection(RIGHT);
            }
            break;
        case 'x':
        case 'X':
            gameover = true;
            break;
        default:
            break;
        }
    }
}

void logic() {
    if (dir != STOP) {
        snake->move();
    }

    // collision with food
    if (snake->getHeadX() == foodx && snake->getHeadY() == foody) {
        score += 10;
        snake->eatFood();

        bool validPosition = false;
        int newX, newY;

        // Make sure food doesn't spawn on the snake
        while (!validPosition) {
            newX = rand() % width;
            newY = rand() % height;

            validPosition = true;

            // Check if the new position is on the snake
            if (newX == snake->getHeadX() && newY == snake->getHeadY()) {
                validPosition = false;
                continue;
            }

            // Check if on tail segments
            for (int i = 1; i <= snake->getNTail(); i++) {
                if (newX == snake->getBodyPart(i).getX() && newY == snake->getBodyPart(i).getY()) {
                    validPosition = false;
                    break;
                }
            }
        }

        food->setPosition(newX, newY);
        foodx = food->getX();
        foody = food->getY();
    }

    if (snake->checkCollision()) {
        gameover = true;
    }
}

void draw() {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0,0 });
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

    char headChar = 219; // ASCII code for solid block █
    char tailChar = 176; // ASCII code for light shade ░

    char topLeftChar = 201;     // ╔
    char topRightChar = 187;    // ╗
    char bottomLeftChar = 200;  // ╚
    char bottomRightChar = 188; // ╝
    char horizontalChar = 205;  // ═
    char verticalChar = 186;    // ║

    cout << topLeftChar;
    for (int i = 0; i < width; i++)
        cout << horizontalChar << horizontalChar;
    cout << topRightChar << endl;

    for (int i = 0; i < height; i++) {
        cout << verticalChar;
        for (int j = 0; j < width; j++) {
            //snake head
            if (i == snake->getHeadY() && j == snake->getHeadX())
                cout << headChar << headChar;
            //food
            else if (i == foody && j == foodx)
                cout << "()";
            else {
                //tail segments
                bool print = false;
                for (int k = 1; k <= snake->getNTail(); k++) {
                    const SnakeBody& body = snake->getBodyPart(k);
                    if (i == body.getY() && j == body.getX()) {
                        cout << tailChar << tailChar;
                        print = true;
                        break;
                    }
                }
                // Draw empty space if no snake or food
                if (!print)
                    cout << "  ";
            }
        }
        cout << verticalChar << endl;
    }

    cout << bottomLeftChar;
    for (int i = 0; i < width; i++)
        cout << horizontalChar << horizontalChar;
    cout << bottomRightChar << endl;

    cout << "Score: " << score << endl;
}

int main() {
    setup();
    PlaySound(TEXT("gameMusic.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);

    while (!gameover) {
        draw();
        input();
        logic();

        // Adjust speed based on direction to compensate for aspect ratio
        if (dir == LEFT || dir == RIGHT)
            Sleep(gameSpeed); // Slower for horizontal movement
        else
            Sleep(gameSpeed / 2); //Faster for vertical movement
    }
    PlaySound(NULL, 0, 0);  // Stop sound when game ends

    delete snake;
    delete food;

    cout << "GAME OVER" << endl;
    cout << "Final Score: " << score << endl;

    return 0;
}