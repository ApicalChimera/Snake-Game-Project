/*
* Snake Game Project
* By: Eleek Zhang, Daniel Chen, Sean Lin, & Brian Dietz
* Due Date: April 3rd, 2025
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
int score;
int baseGameSpeed;
int currentGameSpeed;
int difficultyMode;
int gameMode;
const int MAX_MULTIPLE_FOODS = 12;
const int MIN_FOODS = 2;
const int EFFECT_DURATION = 3000;
bool speedEffectActive = false;
bool scoreMultiplierActive = false;
int scoreMultiplier = 1;
int foodGenerationTimer = 0;
const int FOOD_GENERATION_INTERVAL = 7000;
int currentFoodCount = 2;
clock_t lastFoodGenerationTime;
int shrinkFoodCount = 0;
const int MAX_SHRINK_FOOD = 2;

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

class EffectTimer {
private:
    clock_t startTime;
    bool isActive;
    int duration;

public:
    EffectTimer() : startTime(0), isActive(false), duration(0) {}

    void start(int durationMs) {
        startTime = clock();
        duration = durationMs;
        isActive = true;
    }

    bool isExpired() {
        if (!isActive) return true;

        clock_t currentTime = clock();
        double elapsedMs = (currentTime - startTime) * 1000.0 / CLOCKS_PER_SEC;

        if (elapsedMs >= duration) {
            isActive = false;
            return true;
        }
        return false;
    }

    int getRemainingTime() {
        if (!isActive) return 0;

        clock_t currentTime = clock();
        double elapsedMs = (currentTime - startTime) * 1000.0 / CLOCKS_PER_SEC;
        int remaining = duration - static_cast<int>(elapsedMs);

        return (remaining > 0) ? remaining : 0;
    }

    bool isRunning() {
        return isActive;
    }

    void reset() {
        isActive = false;
    }
};

EffectTimer speedEffectTimer;
EffectTimer scoreMultiplierTimer;

class Food {
protected:
    int x, y;
    char leftSymbol, rightSymbol;

public:
    Food(int x = 0, int y = 0) : x(x), y(y), leftSymbol('('), rightSymbol(')') {}
    virtual ~Food() {}

    void getFoodPosition(int& foodX, int& foodY) const {
        foodX = x;
        foodY = y;
    }

    virtual void respawn(int maxWidth, int maxHeight) {
        x = rand() % maxWidth;
        y = rand() % maxHeight;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }

    char getLeftSymbol() const { return leftSymbol; }
    char getRightSymbol() const { return rightSymbol; }

    virtual void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) = 0;

    virtual Food* clone() const = 0; // For creating new food objects dynamically
};

class RegularFood : public Food {
public:
    RegularFood(int x = 0, int y = 0) : Food(x, y) {
        leftSymbol = '(';
        rightSymbol = ')';
    }

    void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) override {
        tailSize++;
    }

    Food* clone() const override {
        return new RegularFood(*this);
    }
};

class SpeedUpFood : public Food {
public:
    SpeedUpFood(int x = 0, int y = 0) : Food(x, y) {
        leftSymbol = '{';
        rightSymbol = '}';
    }

    void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) override {
        currentSpeed = static_cast<int>(baseSpeed * 0.5);
        speedEffectActive = true;
        speedTimer.start(EFFECT_DURATION);
        tailSize++;
    }

    Food* clone() const override {
        return new SpeedUpFood(*this);
    }
};

class SlowDownFood : public Food {
public:
    SlowDownFood(int x = 0, int y = 0) : Food(x, y) {
        leftSymbol = '[';
        rightSymbol = ']';
    }

    void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) override {
        currentSpeed = static_cast<int>(baseSpeed * 1.5);
        speedEffectActive = true;
        speedTimer.start(EFFECT_DURATION);
        tailSize++;
    }

    Food* clone() const override {
        return new SlowDownFood(*this);
    }
};

class ShrinkFood : public Food {
public:
    ShrinkFood(int x = 0, int y = 0) : Food(x, y) {
        leftSymbol = '<';
        rightSymbol = '>';
    }

    void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) override {
        tailSize = max(0, tailSize - 2);
        score = max(0, score - 20);
    }

    Food* clone() const override {
        return new ShrinkFood(*this);
    }
};

class ScoreMultiplierFood : public Food {
public:
    ScoreMultiplierFood(int x = 0, int y = 0) : Food(x, y) {
        leftSymbol = '&';
        rightSymbol = '&';
    }

    void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) override {
        scoreMultiplier = 2;
        scoreMultiplierActive = true;
        scoreMultiplierTimer.start(EFFECT_DURATION);
        tailSize++;
    }

    Food* clone() const override {
        return new ScoreMultiplierFood(*this);
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

    void removeTail(int count) {
        ntail = max(0, ntail - count);
    }

    int getHeadX() const { return body[0].getX(); }
    int getHeadY() const { return body[0].getY(); }
    int getNTail() const { return ntail; }
    void setNTail(int newTailSize) { ntail = newTailSize; }

    eDirection getDirection() const { return direction; }
    void setDirection(eDirection newDir) { direction = newDir; }

    const SnakeBody& getBodyPart(int index) const {
        return body[index];
    }
};

Snake* snake = nullptr;
Food* foods[MAX_MULTIPLE_FOODS] = { nullptr };
int activeFoodCount = 0;

// All available food types to choose from when generating random food
Food* foodTemplates[5] = { nullptr };

void setup() {
    SetConsoleOutputCP(437);

    gameover = false;
    score = 0;
    dir = STOP;
    scoreMultiplier = 1;
    scoreMultiplierActive = false;
    currentFoodCount = MIN_FOODS;
    shrinkFoodCount = 0;

    cout << "Snake Game with Double-Line Borders" << endl;
    cout << "Select game mode:" << endl;
    cout << "1. Classic mode (regular food only)" << endl;
    cout << "2. Multiple food types mode" << endl;
    cin >> gameMode;

    cout << "Select difficulty:" << endl;
    cout << "1. Hard (faster)" << endl;
    cout << "2. Easy (slower)" << endl;
    cin >> difficultyMode;

    baseGameSpeed = (difficultyMode == 1) ? 60 : 200;
    currentGameSpeed = baseGameSpeed;

    snake = new Snake(width / 2, height / 2);

    srand(static_cast<unsigned>(time(0)));

    // Initialize food templates
    foodTemplates[0] = new RegularFood();
    foodTemplates[1] = new SpeedUpFood();
    foodTemplates[2] = new SlowDownFood();
    foodTemplates[3] = new ShrinkFood();
    foodTemplates[4] = new ScoreMultiplierFood();

    // Initialize minimum number of food items
    activeFoodCount = MIN_FOODS;

    if (gameMode == 1) {
        for (int i = 0; i < MIN_FOODS; i++) {
            foods[i] = new RegularFood(rand() % width, rand() % height);
        }
    }
    else {
        for (int i = 0; i < MIN_FOODS; i++) {
            int randomFoodType = rand() % 5;
            foods[i] = foodTemplates[randomFoodType]->clone();

            if (randomFoodType == 3) {
                shrinkFoodCount++;
            }

            foods[i]->setPosition(rand() % width, rand() % height);
        }
    }

    // Initialize the food generation timer
    lastFoodGenerationTime = clock();

    // Clear screen before game start
    system("cls");

    cout << "Use WASD or arrow keys to move the snake." << endl;
    cout << "Press X to quit the game." << endl;

    if (gameMode == 2) {
        cout << "Food types:" << endl;
        cout << "() - Regular food (adds tail segment)" << endl;
        cout << "<> - Removes part of the snake and reduces score by 20" << endl;
        cout << "{} - Increases speed by 1.5x for 3 seconds" << endl;
        cout << "[] - Reduces speed to 0.5x for 3 seconds" << endl;
        cout << "&& - Doubles score points for 3 seconds" << endl;
        cout << "\nFood will gradually spawn over time (every 7 seconds)" << endl;
    }
    else {
        cout << "Food will gradually spawn over time (every 7 seconds)" << endl;
    }

    cout << "Press any key to start...";
    _getch();
    system("cls");
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

bool isPositionOnSnake(int x, int y) {
    if (x == snake->getHeadX() && y == snake->getHeadY()) {
        return true;
    }

    for (int i = 1; i <= snake->getNTail(); i++) {
        if (x == snake->getBodyPart(i).getX() && y == snake->getBodyPart(i).getY()) {
            return true;
        }
    }

    return false;
}

bool isPositionOnOtherFood(int index, int x, int y) {
    for (int i = 0; i < activeFoodCount; i++) {
        if (i != index && foods[i] != nullptr) {
            if (x == foods[i]->getX() && y == foods[i]->getY()) {
                return true;
            }
        }
    }

    return false;
}

void respawnFood(int index) {
    bool validPosition = false;
    int newX, newY;
    bool wasShrinkFood = dynamic_cast<ShrinkFood*>(foods[index]) != nullptr;

    while (!validPosition) {
        newX = rand() % width;
        newY = rand() % height;

        validPosition = !isPositionOnSnake(newX, newY) && !isPositionOnOtherFood(index, newX, newY);
    }

    if (gameMode == 2) {
        int randomFoodType;

        if (wasShrinkFood) {
            shrinkFoodCount--;
        }

        if (shrinkFoodCount >= MAX_SHRINK_FOOD) {
            do {
                randomFoodType = rand() % 5;
            } while (randomFoodType == 3);
        }
        else {
            randomFoodType = rand() % 5;
        }

        if (randomFoodType == 3) {
            shrinkFoodCount++;
        }

        delete foods[index];

        foods[index] = foodTemplates[randomFoodType]->clone();
    }

    foods[index]->setPosition(newX, newY);
}

void generateNewFood() {
    if (activeFoodCount >= MAX_MULTIPLE_FOODS) {
        return;
    }

    int newIndex = activeFoodCount;
    activeFoodCount++;

    if (gameMode == 1) {
        foods[newIndex] = new RegularFood();
    }
    else {
        int randomFoodType;

        if (shrinkFoodCount >= MAX_SHRINK_FOOD) {
            do {
                randomFoodType = rand() % 5;
            } while (randomFoodType == 3);
        }
        else {
            randomFoodType = rand() % 5;
        }

        if (randomFoodType == 3) {
            shrinkFoodCount++;
        }

        foods[newIndex] = foodTemplates[randomFoodType]->clone();
    }

    bool validPosition = false;
    int newX, newY;

    while (!validPosition) {
        newX = rand() % width;
        newY = rand() % height;

        validPosition = !isPositionOnSnake(newX, newY) && !isPositionOnOtherFood(newIndex, newX, newY);
    }

    foods[newIndex]->setPosition(newX, newY);
}

void updateEffects() {
    if (speedEffectActive && speedEffectTimer.isExpired()) {
        currentGameSpeed = baseGameSpeed;
    }

    if (scoreMultiplierActive && scoreMultiplierTimer.isExpired()) {
        scoreMultiplier = 1;
        scoreMultiplierActive = false;
    }

    clock_t currentTime = clock();
    double elapsedMs = (currentTime - lastFoodGenerationTime) * 1000.0 / CLOCKS_PER_SEC;

    if (elapsedMs >= FOOD_GENERATION_INTERVAL) {
        if (activeFoodCount < MAX_MULTIPLE_FOODS) {
            generateNewFood(); // Now both game modes will generate food
        }
        lastFoodGenerationTime = currentTime;
    }
}

void logic() {
    updateEffects();

    if (dir != STOP) {
        snake->move();
    }

    int headX = snake->getHeadX();
    int headY = snake->getHeadY();

    for (int i = 0; i < activeFoodCount; i++) {
        if (foods[i] != nullptr && headX == foods[i]->getX() && headY == foods[i]->getY()) {
            if (dynamic_cast<ShrinkFood*>(foods[i]) == nullptr) {
                score += 10 * scoreMultiplier;
            }

            bool wasShrinkFood = dynamic_cast<ShrinkFood*>(foods[i]) != nullptr;

            int tailSize = snake->getNTail();
            foods[i]->applyEffect(baseGameSpeed, currentGameSpeed, tailSize, speedEffectActive,
                speedEffectTimer, score, scoreMultiplier,
                scoreMultiplierActive, scoreMultiplierTimer);
            snake->setNTail(tailSize);

            if (wasShrinkFood) {
                shrinkFoodCount--;
            }

            respawnFood(i);
            break;
        }
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
            // Snake head
            if (i == snake->getHeadY() && j == snake->getHeadX())
                cout << headChar << headChar;
            else {
                // Check for food
                bool foodFound = false;
                for (int k = 0; k < activeFoodCount; k++) {
                    if (foods[k] != nullptr && i == foods[k]->getY() && j == foods[k]->getX()) {
                        cout << foods[k]->getLeftSymbol() << foods[k]->getRightSymbol();
                        foodFound = true;
                        break;
                    }
                }

                if (!foodFound) {
                    // Check for tail segments
                    bool tailFound = false;
                    for (int k = 1; k <= snake->getNTail(); k++) {
                        const SnakeBody& body = snake->getBodyPart(k);
                        if (i == body.getY() && j == body.getX()) {
                            cout << tailChar << tailChar;
                            tailFound = true;
                            break;
                        }
                    }

                    // Draw empty space if no snake or food
                    if (!tailFound)
                        cout << "  ";
                }
            }
        }
        cout << verticalChar << endl;
    }

    cout << bottomLeftChar;
    for (int i = 0; i < width; i++)
        cout << horizontalChar << horizontalChar;
    cout << bottomRightChar << endl;

    cout << "Score: " << score;

    // Only show the multiplier when it's active
    if (scoreMultiplierActive) {
        cout << " (x" << scoreMultiplier << " multiplier active)";
    }
    else if (!scoreMultiplierActive)
    {
        cout << "                                                  ";

    }
    cout << endl;

    if (gameMode == 2) {
        cout << "Food types: ";
        cout << "() Add segment | ";
        cout << "<> -2 segments & -20 score | ";
        cout << "{} Speed up 1.5x | ";
        cout << "[] Slow down 0.5x | ";
        cout << "&& 2x Score multiplier" << endl;

        // Only show speed percentage in mode 2
        int speedPercent = (baseGameSpeed == 0) ? 0 : (baseGameSpeed * 100 / currentGameSpeed);
        cout << "Speed: " << speedPercent << "%" << endl;
    }

    // Display shrink food counter for debugging if needed
    // cout << "Shrink foods: " << shrinkFoodCount << "/" << MAX_SHRINK_FOOD << endl;

    if (activeFoodCount < MAX_MULTIPLE_FOODS) {
        int remainingMs = FOOD_GENERATION_INTERVAL -
            ((clock() - lastFoodGenerationTime) * 1000.0 / CLOCKS_PER_SEC);
        cout << "Next food in: " << remainingMs / 1000 << " seconds" << endl;
    }

    if (speedEffectActive) {
        int remainingMs = speedEffectTimer.getRemainingTime();
        cout << "Speed effect: " << remainingMs / 1000 << "." << (remainingMs % 1000) / 100 << " seconds" << endl;
    }

    if (scoreMultiplierActive) {
        int remainingMs = scoreMultiplierTimer.getRemainingTime();
        cout << "Score multiplier: " << remainingMs / 1000 << "." << (remainingMs % 1000) / 100 << " seconds" << endl;
    }
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
            Sleep(currentGameSpeed); // Slower for horizontal movement
        else
            Sleep(currentGameSpeed / 2); // Faster for vertical movement
    }

    // Clean up
    delete snake;

    for (int i = 0; i < activeFoodCount; i++) {
        delete foods[i];
    }

    for (int i = 0; i < 5; i++) {
        delete foodTemplates[i];
    }

    cout << endl << endl << endl << "GAME OVER" << endl;
    cout << "Final Score: " << score << endl;

    PlaySound(NULL, 0, 0);
    return 0;
}