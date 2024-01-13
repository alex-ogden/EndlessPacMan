/*
    Endless PacMan - Alex Ogden, 2024

    A game similar to PacMan for the terminal. With far less colours and much more unicode wchar_t's

    Enemy AI:
        Currently, the enemy uses the A* pathfinding algorithm to carve a path to the player. It moves around walls
        and coins. This path is updated every time the handleEnemyMovement() function is called. This takes a lot of
        calculation and is likely the slowest part of the program. Especially with how frequently it runs. Might try to
        optimise this a bit if possible.
*/

#include <Windows.h>
#undef max // Stops errors occuring due to the use of std::numeric_limits<int>::max()

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <conio.h>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <limits>
#include <format>
#include <ctime>
#include <cmath>
#include <set>
#include <map>

// Constant globals
const int mapWidth = 30;
const int mapHeight = 31;

// Struct to represent a point on the map (for the pathfinding algo)
struct Point 
{
    int x, y;

    // Overloading the < operator to stop VS complaining
    bool operator<(const Point& other) const {
        if (x == other.x) {
            return y < other.y;
        }
        return x < other.x;
    }
};

// Entity chars
enum Char : wchar_t 
{
    PLAYER_UP           = L'▲',
    PLAYER_DOWN         = L'▼',
    PLAYER_LEFT         = L'◄',
    PLAYER_RIGHT        = L'►',
    ENEMY               = L'X',
    COIN                = L'O',
    WALL                = L'#',
    FLOOR               = L' ',
    PLAYER_PLACEHOLDER  = L'P',
    NEXT_LEVEL_DOOR     = L'D'
};

// Difficulty
enum Difficulty
{
    EASY,       // Extremely easy
    MEDIUM,     // Quite easy
    HARD,       // Fairly difficult
    VERY_HARD,  // Very very hard
    NIGHTMARE,  // Basically impossible
};

// Set entity chars - player not const as it changes on direction
enum Char playerChar = PLAYER_UP;
enum Char enemyChar = ENEMY;
enum Char coinChar = COIN;
enum Char wallChar = WALL;
enum Char floorChar = FLOOR;
enum Char playerPlaceholderChar = PLAYER_PLACEHOLDER;
enum Char nextLevelDoorChar = NEXT_LEVEL_DOOR;

// Set game difficulty
const enum Difficulty difficulty = HARD;

/* 
    Function forward declarations
*/
// Generation Entity Functions
void generateCoins(int numCoins, std::wstring& map);
void generateEnemies(int numEnemies, std::wstring& map);

// Conversion Functions
int coordConvert2T1(int px, int py);
std::vector<int> coordConvert1T2(int idx);

// Get Details Functions
std::vector<int> getEnemyIndexes(std::wstring& map);
int getCurrentCoins(std::wstring& map);

// Movement Functions
void getPlayerPos(std::wstring& map, int& playerX, int& playerY, int& playerCurrentIndex, int& playerPreviousIndex);
void handlePlayerMovement(int& playerX, int& playerY, std::wstring& map);
void handleEnemyMovement(std::vector<int> enemyIndexes, std::wstring& map, int playerCurrentIndex);

// Collision Functions
bool isCoinHere(std::wstring& map, int playerCurrentIndex);
bool isEnemyHere(std::wstring& map, int playerCurrentIndex);
bool playerCrossingDoor(int& playerCurrentIndex, int& nextLevelDoorIndex);

// A* Functions
int getHeuristic(Point a, Point b);
std::vector<Point> getNeighbours(Point p, std::wstring& map);
std::vector<Point> aStar(Point start, Point goal, std::wstring& map);

// Drawing Functions
void drawMap(std::wstring& map, wchar_t* screen, HANDLE& hConsole, int& playerScore, int& currentCoins, int& currentLevel, int& numLevels);
void displayScore(int currentLevel, int numLevels, int playerScore);

// Level loading
std::wstring initMap(std::string fileName);
int getNumLevels(std::string levelDir);
void getNextLevelDoorIndex(std::wstring& map, int& nextLevelDoorIndex);
void clearDoor(std::wstring& map, int& nextLevelDoorIndex);


int main()
{
    int currentLevel = 0;
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string directoryPath = currentPath.string();
    std::string levelDir = std::format("{}\\levels", directoryPath);

    // Build file name
    std::string levelFile = std::format("{}\\level{}.txt", levelDir, std::to_string(currentLevel));

    // Create our map
    std::wstring map;
    // Initialise map
    map = initMap(levelFile);

    // Find the number of levels
    int numLevels = getNumLevels(levelDir);
    
    // Player vars
    int playerScore = 0;

    // Player is placed based on where 'P' lands on the map
    int playerX;
    int playerY;
    int playerCurrentIndex;
    int playerPreviousIndex;

    getPlayerPos(map, playerX, playerY, playerCurrentIndex, playerPreviousIndex);

    // Index of the next-level door
    int nextLevelDoorIndex;

    // Vector of enemy indexes
    std::vector<int> enemyIndexes;

    // Var to track current number of coins and enemies on the map
    int numEnemies = 1;
    int numCoins = 10;
    int currentCoins = numCoins;

    /*
        Get a handle to the console
        Get console buffer info
        Set console buffer size (map height and width)
    */
    wchar_t* screen = new wchar_t[mapWidth * mapHeight * 2];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);

    // Set enemy delay based on difficulty
    int delay;
    switch (difficulty)
    {
    case EASY:
        delay = 7;
        break;
    case MEDIUM:
        delay = 5;
        break;
    case HARD:
        delay = 3;
        break;
    case VERY_HARD:
        delay = 2;
        break;
    case NIGHTMARE:
        delay = 1;
        break;
    }

    // Seed RNG
    srand(time(0));

    generateCoins(numCoins, map);
    if (numEnemies > 0)
        generateEnemies(numEnemies, map);

    // Game loop
    bool gameOver = false;
    int counter = 0;
    while (!gameOver)
    {
        if (counter == 0)
        {
            getNextLevelDoorIndex(map, nextLevelDoorIndex);
        }
        counter++;

        currentCoins = getCurrentCoins(map);

        // Draw the map to the screen buffer
        drawMap(map, screen, hConsole, playerScore, currentCoins, currentLevel, numLevels);

        // Handle movement
        playerPreviousIndex = playerCurrentIndex;
        handlePlayerMovement(playerX, playerY, map);
        playerCurrentIndex = coordConvert2T1(playerX, playerY);

        // Handle enemy movement
        if (counter % delay == 0)
        {
            enemyIndexes = getEnemyIndexes(map);
            handleEnemyMovement(enemyIndexes, map, playerCurrentIndex);
        }

        // Check for enemy collision
        if (isEnemyHere(map, playerCurrentIndex))
        {
            gameOver = true;
        }

        // Check for coin collision
        if (isCoinHere(map, playerCurrentIndex))
        {
            playerScore++;
            currentCoins--;
        }

        /*
            If we run out of coins on the map, randomly
            generate some more
        */
        if (currentCoins == 0)
        {
            // Clear the door to the next level
            clearDoor(map, nextLevelDoorIndex);

            if (playerCrossingDoor(playerCurrentIndex, nextLevelDoorIndex))
            {
                // Increment the level
                if (currentLevel == numLevels)
                {
                    gameOver = true;
                    break;
                }
                else
                {
                    currentLevel += 1;
                    // Read new map from level file
                    levelFile = std::format("{}\\level{}.txt", levelDir, std::to_string(currentLevel));
                    map = initMap(levelFile);

                    // Get player position
                    getPlayerPos(map, playerX, playerY, playerCurrentIndex, playerPreviousIndex);

                    currentCoins = numCoins;
                    generateCoins(numCoins, map);
                    if (numEnemies > 0)
                        generateEnemies(numEnemies, map);

                    // Reset game counter
                    counter = 0;
                }
            }
        }

        // Place the player in the world
        map[playerPreviousIndex] = floorChar;
        map[playerCurrentIndex] = playerChar;

        // Sleep
        Sleep(50);
    }

    displayScore(currentLevel, numLevels, playerScore);

    return 0;
}

bool playerCrossingDoor(int& playerCurrentIndex, int& nextLevelDoorIndex)
{
    return playerCurrentIndex == nextLevelDoorIndex ? true : false;
}

void getNextLevelDoorIndex(std::wstring& map, int& nextLevelDoorIndex)
{
    for (int i = 0; i < mapWidth * mapHeight; i++)
        if (map[i] == nextLevelDoorChar)
        {
            nextLevelDoorIndex = i;
            map[i] = wallChar;
        }
}

void clearDoor(std::wstring& map, int& nextLevelDoorIndex)
{
    map[nextLevelDoorIndex] = floorChar;
}

void displayScore(int currentLevel, int numLevels, int playerScore)
{
    std::cout << "********** GAME OVER **********" << std::endl;
    std::cout << "\nYour Score:" << std::endl;
    std::cout << "Levels played:   " << currentLevel << "/" << numLevels << std::endl;
    std::cout << "Coins collected: " << playerScore << std::endl;
    std::cout << "\n******************************" << std::endl;
    std::cout << "Thanks for playing!" << std::endl;
    _getch();
}

int getNumLevels(std::string levelDir)
{
    int levelCount = 0;

    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(levelDir))
            if (entry.is_regular_file())
                levelCount++;
    }
    catch (std::filesystem::filesystem_error& e)
    {
        std::cout << "Failed to open directory: " << levelDir << std::endl;
        std::cout << "Error: " << e.what() << std::endl;
    }

    return levelCount;
}

void getPlayerPos(std::wstring& map, int& playerX, int& playerY, int& playerCurrentIndex, int& playerPreviousIndex)
{
    // Loop through the map and find the P character
    for (int i = 0; i < mapWidth * mapHeight; i++)
        if (map[i] == playerPlaceholderChar)
        {
            playerCurrentIndex = i;
            playerPreviousIndex = i;
            std::vector<int> playerXY = coordConvert1T2(playerCurrentIndex);
            playerX = playerXY[0];
            playerY = playerXY[1];
        }
}

std::wstring initMap(std::string fileName)
{
    std::wstring line;
    std::wstring map;

    // Load our level file
    std::wifstream levelFile(fileName);

    if (levelFile.is_open())
    {
        while (std::getline(levelFile, line))
            map += line;
    }
    else
    {
        std::wcout << L"Failed to open file: " << fileName.c_str() << std::endl;
        return map;
    }
    
    levelFile.close();

    return map;
}

int getHeuristic(Point a, Point b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::vector<Point> getNeighbours(Point p, std::wstring& map)
{
    std::vector<Point> neighbours;

    // Check cells to left, right, above and below current point
    if (p.x > 0 && map[coordConvert2T1(p.x - 1, p.y)] != wallChar && p.x > 0 && map[coordConvert2T1(p.x - 1, p.y)] != coinChar)                             neighbours.push_back({ p.x - 1, p.y });
    if (p.x < mapWidth && map[coordConvert2T1(p.x + 1, p.y)] != wallChar && p.x < mapWidth && map[coordConvert2T1(p.x + 1, p.y)] != coinChar)               neighbours.push_back({ p.x + 1, p.y });
    if (p.y > 0 && map[coordConvert2T1(p.x, p.y - 1)] != wallChar && p.y > 0 && map[coordConvert2T1(p.x, p.y - 1)] != coinChar)                             neighbours.push_back({ p.x, p.y - 1 });
    if (p.y < mapHeight - 1 && map[coordConvert2T1(p.x, p.y + 1)] != wallChar && p.y < mapHeight - 1 && map[coordConvert2T1(p.x, p.y + 1)] != coinChar)     neighbours.push_back({ p.x, p.y + 1 });

    return neighbours;
}

std::vector<Point> aStar(Point start, Point goal, std::wstring& map)
{
    std::set<Point> openSet = { start };
    std::map<Point, Point> cameFrom;
    std::map<Point, int> gScore;
    gScore[start] = 0;
    std::map<Point, int> fScore;
    fScore[start] = getHeuristic(start, goal);

    while (!openSet.empty())
    {
        // Find the node in openSet with the lowest fScore[] value
        Point current;
        int lowestFScore = std::numeric_limits<int>::max();
        for (Point p : openSet)
        {
            if (fScore[p] < lowestFScore)
            {
                lowestFScore = fScore[p];
                current = p;
            }
        }

        if (current.x == goal.x && current.y == goal.y)
        {
            // We've reached our target!
            std::vector<Point> path;
            while (cameFrom.find(current) != cameFrom.end())
            {
                path.push_back(current);
                current = cameFrom[current];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        openSet.erase(current);

        for (Point neighbour : getNeighbours(current, map))
        {
            int tentativeGScore = gScore[current] + 1;
            if (gScore.find(neighbour) == gScore.end() || tentativeGScore < gScore[neighbour])
            {
                cameFrom[neighbour] = current;
                gScore[neighbour] = tentativeGScore;
                fScore[neighbour] = gScore[neighbour] + getHeuristic(neighbour, goal);

                if (openSet.find(neighbour) == openSet.end())
                    openSet.insert(neighbour);
            }
        }
    }

    return std::vector<Point>();
}

std::vector<int> getEnemyIndexes(std::wstring& map)
{
    /*
        Finds all enemy indexes on the map and returns them as a vector
    */

    std::vector<int> enemyIndexes;

    for (int i = 0; i < mapHeight * mapWidth; i++)
        if (map[i] == enemyChar)
            enemyIndexes.push_back(i);

    return enemyIndexes;
}

int getCurrentCoins(std::wstring& map)
{
    int numCoinsCounted = 0;
    for (char c : map)
        if (c == coinChar)
            numCoinsCounted++;

    return numCoinsCounted;
}

bool isCoinHere(std::wstring& map, int playerCurrentIndex)
{
    return (map[playerCurrentIndex] == coinChar);
}

bool isEnemyHere(std::wstring& map, int playerCurrentIndex)
{
    return (map[playerCurrentIndex] == enemyChar);
}

void handleEnemyMovement(std::vector<int> enemyIndexes, std::wstring& map, int playerCurrentIndex)
{
    /*
        Finds where the enemies are on the map in relation to the player and
        moves enemies towards the player
    */

    // Convert the player index into an X and Y position
    std::vector<int> playerXY = coordConvert1T2(playerCurrentIndex);
    int playerX = playerXY[0];
    int playerY = playerXY[1];
    Point playerPoint = { playerX, playerY };

    // Now loop through enemy indexes, work out their X and Y and move towards the player
    for (int prevEnemyIndex : enemyIndexes)
    {
        std::vector<int> enemyXY = coordConvert1T2(prevEnemyIndex);
        int enemyX = enemyXY[0];
        int enemyY = enemyXY[1];

        // Get enemy point on map
        Point enemyPoint = { enemyX, enemyY };

        // Get a list of points from the enemy to the player
        std::vector<Point> path = aStar(enemyPoint, playerPoint, map);

        // If the path is not empty, the next point is the second point in the path
        if (path.size() > 1)
        {
            Point nextPoint = path[1];
            // Get the index the enemy should be at
            int newEnemyIndex = coordConvert2T1(nextPoint.x, nextPoint.y);
            if (map[newEnemyIndex] == enemyChar)
                newEnemyIndex = prevEnemyIndex;
            else
                map[prevEnemyIndex] = floorChar;
            
            map[newEnemyIndex] = enemyChar;
        }
    }
}

void handlePlayerMovement(int& playerX, int& playerY, std::wstring& map)
{
    int localPlayerCurrentIndex;
    if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
    {
        // Go up
        playerY--;
        playerChar = PLAYER_UP;
        if (map[coordConvert2T1(playerX, playerY)] == wallChar)     playerY++; // Ensure player does not pass through walls
    }
    if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
    {
        // Go left
        playerX--;
        playerChar = PLAYER_LEFT;
        if (map[coordConvert2T1(playerX, playerY)] == wallChar)     playerX++; // Ensure player does not pass through walls
    }
    if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
    {
        // Go down
        playerY++;
        playerChar = PLAYER_DOWN;
        if (map[coordConvert2T1(playerX, playerY)] == wallChar)     playerY--; // Ensure player does not pass through walls
    }
    if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
    {
        // Go right
        playerX++;
        playerChar = PLAYER_RIGHT;
        if (map[coordConvert2T1(playerX, playerY)] == wallChar)     playerX--; // Ensure player does not pass through walls
    }
}

void generateCoins(int numCoins, std::wstring& map)
{
    /*
        Loops through the map, and finds places where there are no
        wall characters etc... to place coins.

        Coins are placed at random as we shuffle the list of eligible 
        cells to place them in
    */

    // Get all eligible cells to place coins in
    // We start at mapWidth, to ensure the coins aren't
    // placed in the top row where the score/stats are displayed
    std::vector<int> eligibleCells;
    for (int i = mapWidth; i < mapHeight * mapWidth; i++)
        if (map[i] != wallChar && map[i] != enemyChar && map[i] != playerChar && map[i] != coinChar)
            eligibleCells.push_back(i);

    // Shuffle the list to ensure coins are placed at random
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(eligibleCells.begin(), eligibleCells.end(), std::default_random_engine(seed));

    for (int i = 0; i < numCoins; ++i)
        if (i < eligibleCells.size())   map[eligibleCells[i]] = coinChar;
        else                            break;
}

void generateEnemies(int numEnemies, std::wstring& map)
{
    /*
        Loops through the map, and finds places where there are no
        wall characters etc... to place enemies.

        Enemies are placed at random as we shuffle the list of eligible 
        cells to place them in
    */
    // Get all eligible cells to place enemies in
    // We start at mapWidth, to ensure the enemies aren't
    // placed in the top row where the score/stats are displayed
    std::vector<int> eligibleCells;
    for (int i = mapWidth; i < mapHeight * mapWidth; i++)
        if (map[i] != wallChar && map[i] != enemyChar && map[i] != playerChar && map[i] != coinChar)
            eligibleCells.push_back(i);

    // Shuffle the list to ensure coins are placed at random
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(eligibleCells.begin(), eligibleCells.end(), std::default_random_engine(seed));

    for (int i = 0; i < numEnemies; ++i)
        if (i < eligibleCells.size())   map[eligibleCells[i]] = enemyChar;
        else                            break;
}

int coordConvert2T1(int px, int py)
{
    /*
        2D to 1D coordinate converter

        Takes an x and y coordinate, and turns it into an index for referencing a 1D array
        as though it was a 2D array:

        y * mapWidth + x = index

           0  1  2  3
          +----------
        0 |0  1  2  3
        1 |4  5  6  7
        2 |8  9  10 11
        3 |12 13 14 15

        To find the index of 15 (x = 3, y = 3) - map width = 4
        3 * 4 + 3 = index
           12 + 3 = index
               15 = index
    */
    return py * mapWidth + px;
}

std::vector<int> coordConvert1T2(int idx)
{
    /*
        1D to 2D coordinate converter

        Takes an index of a 1 dimensional array, and the "width" of the map
        and works out the 2D coordinates of a point on the map.

        index % width = x
        (index - x) / width = y

           0  1  2  3
          +----------
        0 |0  1  2  3
        1 |4  5  6  7
        2 |8  9  10 11
        3 |12 13 14 15

        To find the x and y coordinates of index 11 (x = 3, y = 2)

              11 % 4 = x
                  [3 = x] <- found the X
        (11 - 3) / 4 = y
               8 / 4 = y
                  [2 = y] <- found the Y
    */
    int x = idx % mapWidth;
    int y = (idx - x) / mapWidth;
    std::vector<int> xyVals = { x, y };
    return xyVals;
}

void drawMap(std::wstring& map, wchar_t* screen, HANDLE& hConsole, int& playerScore, int& currentCoins, int& currentLevel, int& numLevels) 
{
    const wchar_t* scoreString = L"Coins: %d Score: %d Level: %d/%d\0";

    // Copy the map content line by line into the screen buffer
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            screen[y * mapWidth + x] = map[y * mapWidth + x];
        }
    }

    // Write each line separately to the console
    DWORD dwBytesWritten = 0;
    COORD cursorPosition = { 0, 0 };  // Starting position
    for (short y = 0; y < (short)mapHeight; y++) {
        if (y == 0)
        {
            // Write the score
            swprintf_s(&screen[y * mapWidth], 120, scoreString, currentCoins, playerScore, currentLevel, numLevels);
        }
        WriteConsoleOutputCharacter(hConsole, &screen[y * mapWidth], mapWidth, cursorPosition, &dwBytesWritten);
        cursorPosition.Y++; // Move to the next line
    }
}