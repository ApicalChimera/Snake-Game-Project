/*
* Snake Game Project
* 
* 
* sean
* 
*/
#include<iostream>
#include<windows.h>
#include<conio.h>
using namespace std;

bool gameover;

class SnakeBody {
private:
    int x, y;
    int prevX, prevY;

public:
    SnakeBody(int x, int y);
    void updatePosition(int newX, int newY);
    int getX() const;
    int getY() const;
    int getPrevX() const;
    int getPrevY() const;
};

class Food {
protected:
    int x, y;

public:
    Food(int x, int y);
    void getFoodPosition(int& foodX, int& foodY) const;
};


class NormalFood : public Food {
public:
    NormalFood(int x, int y);
};

class SpeedBoostFood : public Food {
public:
    SpeedBoostFood(int x, int y);
};

int main()
{
    while (!gameover)
    {
        //rewrites screen use this for gamestate
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0,0 });
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = false;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
        cout << "need 2 shit";
    }
}

void input() // if statements will be needed for logic so the snake cant go backwards into its own body, we can also set wsad to a second player if we want multiplayer
{
    if (_kbhit())
    { 
        char key = _getch();
        switch (key)
        {
        case 'w':
        case 'W':
        case 72:
            //if ()
            //{
            //}
            break;
        case 's':
        case 'S':
        case 80: 
            //if ()
            //{
            //}
            break;
        case 'a':
        case 'A':
        case 75: 
            //if ()
            //{
            //}
            break;
        case 'd':
        case 'D':
        case 77: 
            //if ()
            //{
            //}
            break;
        default:
            break;
        }
    }
}