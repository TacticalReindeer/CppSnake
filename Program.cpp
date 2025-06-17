#include <iostream>
#include <thread> // multithread
#include <chrono> // time data
#include <conio.h> // getch
#include <atomic> // thread safe parameter
#include <list>
#include <windows.h> // console control
#include "Vector2.h"
#include <random> // random number generater
#include <cstring> // std::memset
#include <mutex> // thread lock
// using namespace std;
using namespace std::chrono;

enum class Input :byte
{
    none, up, down, right, left, esc, backspace, enter, tab, space
};

enum class State :byte
{
    unstarted, // wait for dir input to start the game
    running, // game processing
    stopped // gameover, wait for n seconds and change state to unstarted
};

enum class Tile :byte
{
    none, snake, apple
};

void MoveSnake(Input dir);
void GameOver();
void SpawnApple();

// print functions
void PrintAt(Vector2Int position, const char* c);
void PrintInterface();
void PrintBorder();
void PrintScore();
void PrintMessage(byte contentIndex, const char* msg);
void ClearMap();
void MapReplace(Tile tile, char ch);

// input handler
void InputHandler();
void InvokeInputEvent(Input input);
void RemoveInputEvent(void(*rm)(Input));

// input events
void GameStart(Input input);
void SetDirection(Input input);
void Exit(Input input);

#pragma region Parameters
// ---------- Global ----------
State currentState;
constexpr Vector2Int mapsize(20, 20), startPos(10, 10);
constexpr int fps = 30;
constexpr double frameTime = 1000.0 / fps; // refresh per x milliseconds
std::atomic<bool> running, hasInput;
std::mutex outputMtx;

// ---------- Game ----------
std::list<Vector2Int> snake;
Tile map[mapsize.x][mapsize.y];
constexpr float startSpeed = 1.5, speedIncrease = 0.1, restartCountDownTime = 5;
float speed, moveCountDown, restartCountDown;
byte toGrow, score, hiScore;
Input currentDir, inputDir;
std::list<void(*)(Input)> inputEvents;
Vector2Int restartCountDownDisplayPos;
bool restartReady;

// random number generater
std::random_device rd;
std::mt19937 gen(rd());

// ---------- Render ----------
HANDLE consoleHandle;
CONSOLE_SCREEN_BUFFER_INFO csbi;
CONSOLE_CURSOR_INFO cursorInfo;
Vector2Int mapPos{ 1,1 };
Vector2Int msgPos{ mapsize.x + 2,0 };
byte contentLength[4]{}; // length of messages, '\0' not included

// ---------- Debug ----------
#pragma endregion

void ChangeState(State to)
{
    // exit state
    switch (currentState)
    {
        case State::running:
            RemoveInputEvent(SetDirection);
            break;

        case State::stopped:
            break;
    }

    currentState = to;

    // enter state
    switch (currentState)
    {
        case State::running:
            // clear map
            std::memset(map, 0, sizeof(map));
            ClearMap();

            PrintAt(mapPos + startPos, "#");
            snake.clear();
            snake.push_front(startPos);
            map[startPos.x][startPos.y] = Tile::snake;

            SpawnApple();

            score = 0;
            PrintScore();

            speed = startSpeed;
            toGrow = 2;
            hasInput = false;
            moveCountDown = 1;
            inputEvents.push_front(SetDirection);

            PrintMessage(2, "\0");
            break;

        case State::stopped:
            // replace "#" to "X" to indicate snake is dead
            MapReplace(Tile::snake, 'X');

            PrintMessage(2, "Game over, new game available in ");

            GetConsoleScreenBufferInfo(consoleHandle, &csbi);
            restartCountDownDisplayPos.x = csbi.dwCursorPosition.X;
            restartCountDownDisplayPos.y = csbi.dwCursorPosition.Y;

            restartCountDown = restartCountDownTime;
            restartReady = false;
            break;
    }
}

int main()
{
    running = true;
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // no buffer
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // add events
    inputEvents.push_front(Exit);
    inputEvents.push_front(GameStart);

    // start InputHandler thread
    std::thread inputHandler(InputHandler);

    // clear screen
    system("cls");

    // hide cursor
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);

    // write map
    PrintInterface();
    PrintAt(mapPos + startPos, "#");

    PrintMessage(2, "Press any direction key to start...");
    PrintMessage(3, "[esc]: stop program");

    // clock
    time_point lastTime = steady_clock::now();
    double elapsed = 0, runtime; // milliseconds

    char c[4];
    while (running)
    {
        elapsed = duration_cast<milliseconds>(steady_clock::now() - lastTime).count();
        lastTime = steady_clock::now();

        // ---------- Update ----------
        if (currentState == State::running)
        {
            // count down
            moveCountDown -= (elapsed / 1000) * speed;

            // change direction
            if (hasInput)
            {
                hasInput = false;

                if (((currentDir == Input::up || currentDir == Input::down) && (inputDir == Input::left || inputDir == Input::right)) ||
                    ((currentDir == Input::right || currentDir == Input::left) && (inputDir == Input::up || inputDir == Input::down)))
                {
                    moveCountDown = 1; // reset timer
                    MoveSnake(inputDir);
                }
            }

            // move
            if (moveCountDown < 0) // if time is up, move
            {
                moveCountDown += 1;
                MoveSnake(currentDir);
            }
        }

        else if (currentState == State::stopped && !restartReady)
        {
            restartCountDown -= elapsed / 1000;

            if (restartCountDown < 0)
            {
                PrintMessage(2, "Game over, Press any direction key to restart...");
                inputEvents.push_front(GameStart);
                restartReady = true;
            }
            else
            {
                std::snprintf(c, sizeof(c), "%.1f", restartCountDown);
                PrintAt(restartCountDownDisplayPos, c);
            }
        }
        // -------------------------------

        // refresh
        runtime = duration_cast<milliseconds>(steady_clock::now() - lastTime).count();
        if (runtime < frameTime)std::this_thread::sleep_for(milliseconds((int)(frameTime - runtime)));
    }

    inputHandler.join();

    // show cursor
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
    system("cls");
    printf("\nProgram terminated.\n");

    return 0;
}

void MoveSnake(Input dir)
{
    Vector2Int pos = *snake.begin();

    if (currentDir != Input::left && dir == Input::right)
    {
        if (pos.x == mapsize.x - 1 || map[pos.x + 1][pos.y] == Tile::snake)goto GameOver;
        pos += Vector2Int::right;
    }
    else if (currentDir != Input::right && dir == Input::left)
    {
        if (pos.x == 0 || map[pos.x - 1][pos.y] == Tile::snake)goto GameOver;
        pos += Vector2Int::left;
    }
    else if (currentDir != Input::down && dir == Input::up)
    {
        if (pos.y == 0 || map[pos.x][pos.y - 1] == Tile::snake)goto GameOver;
        pos += Vector2Int::down;
    }
    else if (currentDir != Input::up && dir == Input::down)
    {
        if (pos.y == mapsize.y - 1 || map[pos.x][pos.y + 1] == Tile::snake)goto GameOver;
        pos += Vector2Int::up;
    }
    else return;

    currentDir = dir;

    // eat apple
    if (map[pos.x][pos.y] == Tile::apple)
    {
        toGrow++;
        score++;
        if (score > hiScore)hiScore++;
        PrintScore();
        speed += speedIncrease;
        SpawnApple();
    }

    map[pos.x][pos.y] = Tile::snake;
    snake.push_front(pos);
    PrintAt(mapPos + pos, "#");

    // remove last element
    if (toGrow <= 0)
    {
        pos = *snake.rbegin();
        PrintAt(mapPos + pos, " ");
        map[pos.x][pos.y] = Tile::none;
        snake.pop_back();
    }
    else toGrow--; // if snake needs to grow, do nothing

    return;

GameOver: GameOver();
}

void GameOver()
{
    ChangeState(State::stopped);
}

void SpawnApple()
{
    // count spaces
    int spaceCount = 0;
    for (int y = 0; y < mapsize.y; y++)
        for (int x = 0; x < mapsize.x; x++)
            if (map[x][y] == Tile::none)spaceCount++;

    // get random number
    std::uniform_int_distribution<> dist(0, spaceCount - 1);
    int rndNum = dist(gen);

    // write map
    for (int y = 0; y < mapsize.y; y++)
        for (int x = 0; x < mapsize.x; x++)
            if (map[x][y] == Tile::none && rndNum-- == 0)
            {
                PrintAt(mapPos + Vector2Int(x, y), "@");
                map[x][y] = Tile::apple;
                return;
            }
}

#pragma region Print functions

void PrintAt(Vector2Int position, const char* c)
{
    std::lock_guard<std::mutex> lock(outputMtx);
    SetConsoleCursorPosition(consoleHandle, { static_cast<short>(position.x), static_cast<short>(position.y) });
    printf(c);
}

void PrintInterface()
{
    PrintBorder();
    PrintScore();
}

void PrintBorder()
{
    std::lock_guard<std::mutex> lock(outputMtx);
    SetConsoleCursorPosition(consoleHandle, { 0,0 });

    // print border of map
    // ╔═══╗
    // ║   ║
    // ╚═══╝

    // formula of c size explain:
    // reminder: border symbles such as "╔" takes 3 char to store ( ╔ is 226, 149, 148)
    //
    // first and last line "╔═╗, ╚═╝", formula: (mapsize.x + 2) * 6 + 2 => 
    //     ( mapsize.x + left border(1) + right border(1) ) * symbol size * first and last line(2) + new line "\n"(1) + string end "\0"(1)
    //
    // middle lines "║ ║", formula: (mapsize.x + 7) * mapsize.y =>
    //     ( mapsize.x + two borders "║"(6) + new line "\n"(1) ) * mapsize.y

    char c[(mapsize.x + 2) * 6 + 2 + (mapsize.x + 7) * mapsize.y];

    char* cptr = c;

    // first line
    memcpy(cptr, "╔", 3);
    cptr += 3;

    for (int i = 0; i < mapsize.x; i++)
    {
        memcpy(cptr, "═", 3);
        cptr += 3;
    }

    memcpy(cptr, "╗\n", 4);
    cptr += 4;

    // middle line
    for (int y = 0; y < mapsize.y; y++)
    {
        memcpy(cptr, "║", 3);
        cptr += 3;

        for (int x = 0; x < mapsize.x; x++, cptr++)*cptr = ' ';

        memcpy(cptr, "║\n", 4);
        cptr += 4;
    }

    // last line
    memcpy(cptr, "╚", 3);
    cptr += 3;

    for (int i = 0; i < mapsize.x; i++)
    {
        memcpy(cptr, "═", 3);
        cptr += 3;
    }

    memcpy(cptr, "╝", 4);
    printf(c);
}

void PrintScore()
{
    PrintMessage(0, ("Score: " + std::to_string(score)).c_str());
    PrintMessage(1, ("Hi-Score: " + std::to_string(hiScore)).c_str());
}

void PrintMessage(byte contentIndex, const char* msg)
{
    // wipe
    if (contentLength[contentIndex] > 0)
    {
        byte formerContentLength = contentLength[contentIndex];
        char c[formerContentLength + 1]{};
        memset(c, ' ', formerContentLength);
        c[formerContentLength] = '\0';
        PrintAt(msgPos + Vector2Int(0, contentIndex), c);
    }

    // print
    PrintAt(msgPos + Vector2Int(0, contentIndex), msg);

    // update content length
    byte length = 0;
    while (msg[length] != '\0')length++;
    contentLength[contentIndex] = length;
}

void ClearMap()
{
    char c[mapsize.x + 1]{ '\0' };
    for (int x = 0; x < mapsize.x; x++)*(c + x) = ' ';

    for (int y = 0; y < mapsize.y; y++)
        PrintAt(mapPos + Vector2Int(0, y), c);
}

// replace char based on map data
void MapReplace(Tile tile, char ch)
{
    char c[mapsize.x + 1]{ '\0' }; // store string to replace, max size of c is mapsize.x + '\0'
    byte length = 0; // content length of c

    for (int y = 0; y < mapsize.y; y++)
    {
        for (int x = 0; x < mapsize.x; x++)
        {
            if (map[x][y] == tile)
            {
                *(c + length) = ch;
                length++;
            }
            else if (*c != 0)
            {
                PrintAt(Vector2Int(x + mapPos.x - length, y + mapPos.y), c);
                std::memset(c, 0, sizeof(c)); // fill c with \0
                length = 0;
            }
        }

        // clear c before next line
        if (length != 0)
        {
            PrintAt(Vector2Int(mapsize.x + mapPos.x - length, y + mapPos.y), c);
            std::memset(c, 0, sizeof(c));
            length = 0;
        }
    }
}
#pragma endregion

#pragma region InputHandler
void InputHandler()
{
    char c;
    while (running)
    {
        c = _getch();
        switch (c)
        {
            case 0:
            specialCode:
                c = _getch();
                switch (c)
                {
                    case 72:
                        InvokeInputEvent(Input::up);
                        break;
                    case 80:
                        InvokeInputEvent(Input::down);
                        break;
                    case 75:
                        InvokeInputEvent(Input::left);
                        break;
                    case 77:
                        InvokeInputEvent(Input::right);
                        break;
                }
                break;
            case 13:
                InvokeInputEvent(Input::enter);
                break;
            case 8:
                InvokeInputEvent(Input::backspace);
                break;
            case 9:
                InvokeInputEvent(Input::tab);
                break;
            case 27:
                InvokeInputEvent(Input::esc);
                break;
            case 32:
                InvokeInputEvent(Input::space);
                break;
            default:
                if (c == -32)goto specialCode;
                break;
        }
        std::this_thread::sleep_for(milliseconds(0));
    }
}

void InvokeInputEvent(Input input)
{
    auto copy(inputEvents);
    for (auto& f : copy)f(input);
}

void RemoveInputEvent(void(*rm)(Input))
{
    inputEvents.remove_if([rm](auto& f) { return f == rm; });
}
#pragma endregion

#pragma region Events
void GameStart(Input input)
{
    if (input != Input::up && input != Input::down && input != Input::right && input != Input::left)return;
    RemoveInputEvent(GameStart);
    currentDir = input;
    ChangeState(State::running);
}

void SetDirection(Input input)
{
    if (input != Input::up && input != Input::down && input != Input::right && input != Input::left)return;
    inputDir = input;
    hasInput = true;
}

void Exit(Input input)
{
    if (input == Input::esc)running = false;
}
#pragma endregion