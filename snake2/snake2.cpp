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

// libraries imported for sound
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;

// size of the game board differs because of different sizes in x and y characters
const int height = 20;
const int width = 30;

const int MAX_MULTIPLE_FOODS = 12; // max number of food items on screen
const int MIN_FOODS = 2; // minimum number of food items
const int EFFECT_DURATION = 3000; // duration of power-up effects in ms
bool gameover; // game over flag
int score; // counter variable for player score
int baseGameSpeed; // base speed of the game
int currentGameSpeed; // current game speed
int difficultyMode; // difficulty setting
int gameMode; // selected game mode

bool speedEffectActive = false; // flag for speed effect
bool scoreMultiplierActive = false; // flag for score multiplier effect
int scoreMultiplier = 1; // multiplier for score during effect
int foodGenerationTimer = 0; // timer for controlling food spawn
const int FOOD_GENERATION_INTERVAL = 7000; // interval to generate new food
int currentFoodCount = 2; // current number of food on screen
clock_t lastFoodGenerationTime; // time when last food was generated
int shrinkFoodCount = 0; // count of shrink food items on screen
const int MAX_SHRINK_FOOD = 2; // max shrink food allowed

enum eDirection { STOP = 0, UP, DOWN, LEFT, RIGHT }; // directions for snake movement
eDirection dir; // current direction

// Snake body class to store the position of each segment (differs from the snake head)
class SnakeBody {
private:
    int x, y; // current position of the snake body in x and y coordinates with respect to the game board
    int prevX, prevY; // previous position of the snake body in x and y coordinates

public:
    SnakeBody(int x = 0, int y = 0) : x(x), y(y), prevX(x), prevY(y) {} // constructor with default values

    void updatePosition(int newX, int newY) { // update the position of the snake body
        prevX = x;
        prevY = y;
        x = newX;
        y = newY;
    }

    int getX() const { return x; } // get current x position
    int getY() const { return y; } // get current y position
    int getPrevX() const { return prevX; } // get previous x position
    int getPrevY() const { return prevY; } // get previous y position
};

// Timer class for handling power-up durations
class EffectTimer {
private:
    clock_t startTime; // time when effect started
    bool isActive; // whether effect is active
    int duration; // effect duration in milliseconds

public:
    EffectTimer() : startTime(0), isActive(false), duration(0) {} // constructor

    void start(int durationMs) { // start the effect timer
        startTime = clock();
        duration = durationMs;
        isActive = true;
    }

    bool isExpired() { // check if the effect time has expired
        if (!isActive) return true;

        clock_t currentTime = clock();
        double elapsedMs = (currentTime - startTime) * 1000.0 / CLOCKS_PER_SEC;

        if (elapsedMs >= duration) {
            isActive = false;
            return true;
        }
        return false;
    }

    int getRemainingTime() { // get time left for the effect
        if (!isActive) return 0;

        clock_t currentTime = clock();
        double elapsedMs = (currentTime - startTime) * 1000.0 / CLOCKS_PER_SEC;
        int remaining = duration - static_cast<int>(elapsedMs);

        return (remaining > 0) ? remaining : 0;
    }

    bool isRunning() { // check if the timer is running
        return isActive;
    }

    void reset() { // stop the timer
        isActive = false;
    }
};

EffectTimer speedEffectTimer; // timer for speed effect duration
EffectTimer scoreMultiplierTimer; // timer for score multiplier duration

// Food class to store the position of the food in x and y coordinates with respect to the game board
class Food {
protected:
    int x, y; // position of the food
    char leftSymbol, rightSymbol; // characters to represent the food visually

public:
    Food(int x = 0, int y = 0) : x(x), y(y), leftSymbol('('), rightSymbol(')') {}
    virtual ~Food() {}

    void getFoodPosition(int& foodX, int& foodY) const { // returns food coordinates
        foodX = x;
        foodY = y;
    }

    virtual void respawn(int maxWidth, int maxHeight) { // randomize food position
        x = rand() % maxWidth;
        y = rand() % maxHeight;
    }

    int getX() const { return x; } // get x coordinate
    int getY() const { return y; } // get y coordinate
    void setPosition(int newX, int newY) { // set new coordinates
        x = newX;
        y = newY;
    }

    char getLeftSymbol() const { return leftSymbol; } // left visual symbol
    char getRightSymbol() const { return rightSymbol; } // right visual symbol

    // apply specific food effect (pure virtual function)
    virtual void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) = 0;

    virtual Food* clone() const = 0; // For creating new food objects dynamically
};

// Different types of food classes inheriting from the Food class
class RegularFood : public Food {
public:
    RegularFood(int x = 0, int y = 0) : Food(x, y) {
        leftSymbol = '(';
        rightSymbol = ')';
    }

    void applyEffect(int& baseSpeed, int& currentSpeed, int& tailSize, bool& speedEffectActive,
        EffectTimer& speedTimer, int& score, int& scoreMultiplier,
        bool& scoreMultiplierActive, EffectTimer& scoreMultiplierTimer) override {
        tailSize++; // grow snake by 1
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
        currentSpeed = static_cast<int>(baseSpeed * 0.5); // increase speed
        speedEffectActive = true;
        speedTimer.start(EFFECT_DURATION);
        tailSize++; // grow snake by 1
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
        currentSpeed = static_cast<int>(baseSpeed * 1.5); // slow down speed
        speedEffectActive = true;
        speedTimer.start(EFFECT_DURATION);
        tailSize++; // grow snake by 1
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
        tailSize = max(0, tailSize - 2); // shrink snake
        score = max(0, score - 20); // decrease score
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
        scoreMultiplier = 2; // double the score
        scoreMultiplierActive = true;
        scoreMultiplierTimer.start(EFFECT_DURATION);
        tailSize++; // grow snake by 1
    }

    Food* clone() const override {
        return new ScoreMultiplierFood(*this);
    }
};
// Snake class to store the position of the snake in x and y coordinates with respect to the game board
// This is where the snake moves
class Snake {
private:
    SnakeBody body[100]; // stores snake segments
    int ntail; // number of tail segments
    eDirection direction; // current movement direction
    int snakeScore; // player's score

public:
    Snake(int startX, int startY) {
        direction = STOP;
        ntail = 0;
        body[0] = SnakeBody(startX, startY); // initialize snake head
        snakeScore = 0;
    }

    void setScore(int newScore) { snakeScore = newScore; } // set score
    int getScore() const { return snakeScore; } // get score
    void addScore(int points) { snakeScore += points; } // add to score

    void move() { // update snake body positions
        for (int i = ntail; i > 0; i--) {
            body[i] = body[i - 1];
        }

        int headX = body[0].getX();
        int headY = body[0].getY();

        // switch statement to determine the direction of the snake
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

    bool checkCollision() { // check if the snake collides with itself or the walls
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

    void eatFood() { // add a segment to the snake when it eats food
        ntail++;
    }

    void removeTail(int count) { // remove a segment from the snake when it eats shrink food
        ntail = max(0, ntail - count);
    }

    int getHeadX() const { return body[0].getX(); } // get head x
    int getHeadY() const { return body[0].getY(); } // get head y
    int getNTail() const { return ntail; } // get tail length
    void setNTail(int newTailSize) { ntail = newTailSize; } // set tail length

    eDirection getDirection() const { return direction; } // get direction
    void setDirection(eDirection newDir) { direction = newDir; } // set direction

    const SnakeBody& getBodyPart(int index) const { // access a body part
        return body[index];
    }
};

Snake* snake = nullptr; // pointer to player 1 snake
Food* foods[MAX_MULTIPLE_FOODS] = { nullptr }; // active food list
int activeFoodCount = 0; // number of food items on board
Snake* snake2 = nullptr; // pointer to player 2 snake (if active)

enum eControlMode { PLAYER1 = 0, PLAYER2 }; // control scheme enum
eDirection dir2; // direction for second player
eControlMode activePlayer; // who is currently playing
bool twoPlayerMode = false; // toggle for multiplayer

// All available food types to choose from when generating random food
Food* foodTemplates[5] = { nullptr }; // food template pool

void setup() {
    SetConsoleOutputCP(437); // set console output for special characters

    // initialize game state variables
    gameover = false;
    score = 0;
    dir = STOP;
    dir2 = STOP;
    scoreMultiplier = 1;
    scoreMultiplierActive = false;
    currentFoodCount = MIN_FOODS;
    shrinkFoodCount = 0;

    // display game mode options
    cout << "Snake Game with Double-Line Borders" << endl;
    cout << "Select game mode:" << endl;
    cout << "1. Classic mode (regular food only)" << endl;
    cout << "2. Multiple food types mode" << endl;
    cout << "3. Two Player mode" << endl;
    cin >> gameMode;

    twoPlayerMode = (gameMode == 3); // enable 2-player mode if chosen

    if (twoPlayerMode) {
        // ask for type of two-player mode
        cout << "Select Two Player type:" << endl;
        cout << "1. Classic (regular food only)" << endl;
        cout << "2. Multiple food types (excluding score multiplier)" << endl;
        int twoPlayerType;
        cin >> twoPlayerType;

        gameMode = (twoPlayerType == 1) ? 1 : 2;
    }

    // ask for difficulty level
    cout << "Select difficulty:" << endl;
    cout << "1. Hard (faster)" << endl;
    cout << "2. Easy (slower)" << endl;
    cin >> difficultyMode;

    baseGameSpeed = (difficultyMode == 1) ? 60 : 200; // set base speed
    currentGameSpeed = baseGameSpeed;

    // initialize player 1 snake
    snake = new Snake(width / 3, height / 2);

    if (twoPlayerMode) {
        // initialize player 2 snake
        snake2 = new Snake(2 * width / 3, height / 2);
        snake2->setScore(0);
        activePlayer = PLAYER1;
    }

    srand(static_cast<unsigned>(time(0))); // seed random number generator

    // Initialize food templates (pool of possible food types)
    foodTemplates[0] = new RegularFood();
    foodTemplates[1] = new SpeedUpFood();
    foodTemplates[2] = new SlowDownFood();
    foodTemplates[3] = new ShrinkFood();
    if (!twoPlayerMode || gameMode == 1) {
        foodTemplates[4] = new ScoreMultiplierFood();
    }

    // Initialize the starting food items
    activeFoodCount = MIN_FOODS;

    if (gameMode == 1) {
        // classic mode: only regular food
        for (int i = 0; i < MIN_FOODS; i++) {
            foods[i] = new RegularFood(rand() % width, rand() % height);
        }
    }
    else {
        // multiple food types mode
        for (int i = 0; i < MIN_FOODS; i++) {
            int maxFoodTypes = (twoPlayerMode) ? 4 : 5;  // Exclude multiplier food in 2P mode
            int randomFoodType = rand() % maxFoodTypes;
            foods[i] = foodTemplates[randomFoodType]->clone();

            if (randomFoodType == 3) {
                shrinkFoodCount++; // track how many shrink foods on screen
            }

            foods[i]->setPosition(rand() % width, rand() % height);
        }
    }

    // Initialize the food generation timer
    lastFoodGenerationTime = clock();

    // Clear screen before game starts
    system("cls");

    // display controls
    if (twoPlayerMode) {
        cout << "Player 1: Use WASD keys to move" << endl;
        cout << "Player 2: Use Arrow keys to move" << endl;
    }
    else {
        cout << "Use WASD or arrow keys to move the snake." << endl;
    }
    cout << "Press X to quit the game." << endl;

    // food legend and behavior in multi-food mode
    if (gameMode == 2) {
        cout << "Food types:" << endl;
        cout << "() - Regular food (adds tail segment)" << endl;
        cout << "<> - Removes part of the snake and reduces score by 20" << endl;
        cout << "{} - Increases speed by 1.5x for 3 seconds" << endl;
        cout << "[] - Reduces speed to 0.5x for 3 seconds" << endl;
        if (!twoPlayerMode) {
            cout << "&& - Doubles score points for 3 seconds" << endl;
        }
        cout << "\nFood will gradually spawn over time (every 7 seconds)" << endl;
    }
    else {
        cout << "Food will gradually spawn over time (every 7 seconds)" << endl;
    }

    // wait for user to press a key
    cout << "Press any key to start...";
    _getch();
    system("cls");
}

// Function to handle keyboard input
void input() {
    if (_kbhit()) {
        char key = _getch();

        if (twoPlayerMode) {
            // Player 1 controls (WASD)
            switch (key) {
            case 'w':
            case 'W':
                if (snake->getDirection() != DOWN) {
                    dir = UP;
                    snake->setDirection(UP);
                }
                break;
            case 's':
            case 'S':
                if (snake->getDirection() != UP) {
                    dir = DOWN;
                    snake->setDirection(DOWN);
                }
                break;
            case 'a':
            case 'A':
                if (snake->getDirection() != RIGHT) {
                    dir = LEFT;
                    snake->setDirection(LEFT);
                }
                break;
            case 'd':
            case 'D':
                if (snake->getDirection() != LEFT) {
                    dir = RIGHT;
                    snake->setDirection(RIGHT);
                }
                break;

                // Player 2 controls (Arrow keys)
            case 72: // ASCII code for up arrow key
                if (snake2->getDirection() != DOWN) {
                    dir2 = UP;
                    snake2->setDirection(UP);
                }
                break;
            case 80: // ASCII code for down arrow key
                if (snake2->getDirection() != UP) {
                    dir2 = DOWN;
                    snake2->setDirection(DOWN);
                }
                break;
            case 75: // ASCII code for left arrow key
                if (snake2->getDirection() != RIGHT) {
                    dir2 = LEFT;
                    snake2->setDirection(LEFT);
                }
                break;
            case 77: // ASCII code for right arrow key
                if (snake2->getDirection() != LEFT) {
                    dir2 = RIGHT;
                    snake2->setDirection(RIGHT);
                }
                break;
            case 'x':
            case 'X':
                gameover = true; // quit game
                break;
            }
        }
        else {
            // Original single player controls
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
                gameover = true; // quit game
                break;
            }
        }
    }
}

// Check if a position is occupied by any part of either snake
bool isPositionOnSnake(int x, int y) {
    if (x == snake->getHeadX() && y == snake->getHeadY()) {
        return true;
    }

    for (int i = 1; i <= snake->getNTail(); i++) {
        if (x == snake->getBodyPart(i).getX() && y == snake->getBodyPart(i).getY()) {
            return true;
        }
    }

    if (twoPlayerMode) {
        if (x == snake2->getHeadX() && y == snake2->getHeadY()) {
            return true;
        }

        for (int i = 1; i <= snake2->getNTail(); i++) {
            if (x == snake2->getBodyPart(i).getX() && y == snake2->getBodyPart(i).getY()) {
                return true;
            }
        }
    }

    return false;
}

// Check if a position is already occupied by another food (not the one at given index)
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

// Respawn a food item at a new random position and possibly change its type
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
            } while (randomFoodType == 3); // skip shrink food
        }
        else {
            randomFoodType = rand() % 5;
        }

        if (randomFoodType == 3) {
            shrinkFoodCount++;
        }

        delete foods[index]; // delete old food
        foods[index] = foodTemplates[randomFoodType]->clone(); // assign new food type
    }

    foods[index]->setPosition(newX, newY); // set new position
}

// Add a new food item to the board if there's room
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
            } while (randomFoodType == 3); // avoid too many shrink foods
        }
        else {
            randomFoodType = rand() % 5;
        }

        if (randomFoodType == 3) {
            shrinkFoodCount++;
        }

        foods[newIndex] = foodTemplates[randomFoodType]->clone();
    }

    // find a valid spawn position
    bool validPosition = false;
    int newX, newY;

    while (!validPosition) {
        newX = rand() % width;
        newY = rand() % height;

        validPosition = !isPositionOnSnake(newX, newY) && !isPositionOnOtherFood(newIndex, newX, newY);
    }

    foods[newIndex]->setPosition(newX, newY);
}

// Handle expiring effects and generate new food on timer
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
            generateNewFood();
        }
        lastFoodGenerationTime = currentTime;
    }
}
void logic() {
    updateEffects();

    if (dir != STOP) {
        snake->move();
    }

    if (twoPlayerMode && dir2 != STOP) {
        snake2->move();
    }

    int headX = snake->getHeadX();
    int headY = snake->getHeadY();
    int head2X = (twoPlayerMode) ? snake2->getHeadX() : -1;
    int head2Y = (twoPlayerMode) ? snake2->getHeadY() : -1;

    for (int i = 0; i < activeFoodCount; i++) {
        if (foods[i] != nullptr && headX == foods[i]->getX() && headY == foods[i]->getY()) {
            if (dynamic_cast<ShrinkFood*>(foods[i]) == nullptr) {
                int addScore = 10 * scoreMultiplier;
                snake->addScore(addScore);
                score = snake->getScore(); 
            }

            bool wasShrinkFood = dynamic_cast<ShrinkFood*>(foods[i]) != nullptr;

            int tailSize = snake->getNTail();
            foods[i]->applyEffect(baseGameSpeed, currentGameSpeed, tailSize, speedEffectActive,
                speedEffectTimer, score, scoreMultiplier,
                scoreMultiplierActive, scoreMultiplierTimer);
            snake->setNTail(tailSize);
            snake->setScore(score); 

            if (wasShrinkFood) {
                shrinkFoodCount--;
            }

            respawnFood(i);
            break;
        }
    }

    if (twoPlayerMode) {
        for (int i = 0; i < activeFoodCount; i++) {
            if (foods[i] != nullptr && head2X == foods[i]->getX() && head2Y == foods[i]->getY()) {
                if (dynamic_cast<ShrinkFood*>(foods[i]) == nullptr) {
                    int addScore = 10 * scoreMultiplier;
                    snake2->addScore(addScore);
                }

                bool wasShrinkFood = dynamic_cast<ShrinkFood*>(foods[i]) != nullptr;

                int tailSize = snake2->getNTail();
                int player2Score = snake2->getScore();
                foods[i]->applyEffect(baseGameSpeed, currentGameSpeed, tailSize, speedEffectActive,
                    speedEffectTimer, player2Score, scoreMultiplier,
                    scoreMultiplierActive, scoreMultiplierTimer);
                snake2->setNTail(tailSize);
                snake2->setScore(player2Score);

                if (wasShrinkFood) {
                    shrinkFoodCount--;
                }

                respawnFood(i);
                break;
            }
        }
    }

    if (snake->checkCollision()) {
        if (twoPlayerMode) {
            gameover = true;
            activePlayer = PLAYER2; 
        }
        else {
            gameover = true;
        }
    }

    if (twoPlayerMode && snake2->checkCollision()) {
        gameover = true;
        activePlayer = PLAYER1;
    }

    if (twoPlayerMode) {
        if (headX == head2X && headY == head2Y) {
            gameover = true;
            if (snake->getScore() > snake2->getScore()) {
                activePlayer = PLAYER1; 
            }
            else if (snake2->getScore() > snake->getScore()) {
                activePlayer = PLAYER2; 
            }
            else {
                activePlayer = PLAYER1; 
            }
        }

        for (int i = 1; i <= snake2->getNTail(); i++) {
            if (headX == snake2->getBodyPart(i).getX() && headY == snake2->getBodyPart(i).getY()) {
                gameover = true;
                activePlayer = PLAYER2;
                break;
            }
        }

        for (int i = 1; i <= snake->getNTail(); i++) {
            if (head2X == snake->getBodyPart(i).getX() && head2Y == snake->getBodyPart(i).getY()) {
                gameover = true;
                activePlayer = PLAYER1; 
                break;
            }
        }
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

    // Character for player 2's snake (inverted)
    char head2Char = 178; // ASCII code for medium shade ▓
    char tail2Char = 177; // ASCII code for dark shade ▒

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
            // Snake head (player 1)
            if (i == snake->getHeadY() && j == snake->getHeadX())
                cout << headChar << headChar;
            // Snake head (player 2)
            else if (twoPlayerMode && i == snake2->getHeadY() && j == snake2->getHeadX())
                cout << head2Char << head2Char;
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
                    // Check for Player 1's tail segments
                    bool tailFound = false;
                    for (int k = 1; k <= snake->getNTail(); k++) {
                        const SnakeBody& body = snake->getBodyPart(k);
                        if (i == body.getY() && j == body.getX()) {
                            cout << tailChar << tailChar;
                            tailFound = true;
                            break;
                        }
                    }

                    // Check for Player 2's tail segments
                    if (!tailFound && twoPlayerMode) {
                        for (int k = 1; k <= snake2->getNTail(); k++) {
                            const SnakeBody& body = snake2->getBodyPart(k);
                            if (i == body.getY() && j == body.getX()) {
                                cout << tail2Char << tail2Char;
                                tailFound = true;
                                break;
                            }
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

    if (twoPlayerMode) {
        cout << "Player 1 Score: " << snake->getScore();
        cout << " | Player 2 Score: " << snake2->getScore();
    }
    else {
        cout << "Score: " << score;
    }

    // Only show the multiplier when it's active and not in two player mode
    if (!twoPlayerMode && scoreMultiplierActive) {
        cout << " (x" << scoreMultiplier << " multiplier active)";
    }
    cout << endl;

    if (gameMode == 2) {
        cout << "Food types: ";
        cout << "() Add segment | ";
        cout << "<> -2 segments & -20 score | ";
        cout << "{} Speed up 1.5x | ";
        cout << "[] Slow down 0.5x";
        if (!twoPlayerMode) {
            cout << " | && 2x Score multiplier";
        }
        cout << endl;

        // Only show speed percentage in mode 2
        int speedPercent = (baseGameSpeed == 0) ? 0 : (baseGameSpeed * 100 / currentGameSpeed);
        cout << "Speed: " << speedPercent << "%" << endl;
    }

    if (activeFoodCount < MAX_MULTIPLE_FOODS) {
        int remainingMs = FOOD_GENERATION_INTERVAL -
            ((clock() - lastFoodGenerationTime) * 1000.0 / CLOCKS_PER_SEC);
        cout << "Next food in: " << remainingMs / 1000 << " seconds" << endl;
    }

    if (speedEffectActive) {
        int remainingMs = speedEffectTimer.getRemainingTime();
        cout << "Speed effect: " << remainingMs / 1000 << "." << (remainingMs % 1000) / 100 << " seconds" << endl;
    }

    if (!twoPlayerMode && scoreMultiplierActive) {
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
        if (dir == LEFT || dir == RIGHT || (twoPlayerMode && (dir2 == LEFT || dir2 == RIGHT)))
            Sleep(currentGameSpeed); // Slower for horizontal movement
        else
            Sleep(currentGameSpeed / 2); // Faster for vertical movement
    }

    // Clean up
    delete snake;

    if (twoPlayerMode) {
        delete snake2;
    }

    for (int i = 0; i < activeFoodCount; i++) {
        delete foods[i];
    }

    for (int i = 0; i < 5; i++) {
        delete foodTemplates[i];
    }

    cout << endl << endl << endl << "GAME OVER" << endl;

    if (twoPlayerMode) {
        if (activePlayer == PLAYER1) {
            cout << "Player 1 wins with a score of " << snake->getScore() << "!" << endl;
        }
        else {
            cout << "Player 2 wins with a score of " << snake2->getScore() << "!" << endl;
        }
    }
    else {
        cout << "Final Score: " << score << endl;
    }

    PlaySound(NULL, 0, 0);
    return 0;
}