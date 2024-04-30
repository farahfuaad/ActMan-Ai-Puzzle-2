//ffyyx 

#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <unordered_map>
#include <string>
#include <tuple>
#include <cmath>
#include <unordered_set>
#include <functional>

using namespace std;

enum ElementType { EMPTY, WALL, ACT_MAN, DEMON, OGRE, CORPSE, ACT_MAN_DEAD };
struct Position {
    int row, col;
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    Position operator+(const Position& other) const {
        return {row + other.row, col + other.col};
    }
};

struct Entity {
    ElementType type;
    Position pos;
};

struct GameBoard {
    vector<vector<ElementType>> board;
    Position actManPosition;
    vector<Entity> entities;
    int score = 50;
    bool bulletFired = false;
};

struct Action {
    char type;
};

unordered_map<char, Position> movementDirections = {
    {'8', {-1, 0}}, // North
    {'9', {-1, 1}}, // Northeast
    {'6', {0, 1}},  // East
    {'3', {1, 1}},  // Southeast
    {'2', {1, 0}},  // South
    {'1', {1, -1}}, // Southwest
    {'4', {0, -1}}, // West
    {'7', {-1, -1}} // Northwest
};

unordered_map<char, Position> firingDirections = {
    {'N', {-1, 0}},
    {'E', {0, 1}},
    {'S', {1, 0}},
    {'W', {0, -1}}
};

size_t hashGameBoard(const GameBoard& board) {
    std::hash<int> hasher;
    size_t hash = 0;
    for (const auto& row : board.board) {
        for (ElementType cell : row) {
            hash ^= hasher(static_cast<int>(cell)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
    }
    hash ^= hasher(board.actManPosition.row) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= hasher(board.actManPosition.col) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= hasher(board.bulletFired) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

GameBoard readGameBoard(const string& inputFile) {
    ifstream input(inputFile);
    if (!input.is_open()) {
        cerr << "Failed to open input file: " << inputFile << endl;
        exit(1);
    }

    int rows, cols;
    input >> rows >> cols;
    GameBoard board;
    board.board.resize(rows, vector<ElementType>(cols, EMPTY));

    char cell;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            input >> noskipws >> cell;
            if (cell == '\n') {
                j--;
                continue;
            }
            switch (cell) {
                case 'A':
                    board.actManPosition = {i, j};
                    board.board[i][j] = ACT_MAN;
                    break;
                case 'D':
                    board.entities.push_back({DEMON, {i, j}});
                    board.board[i][j] = DEMON;
                    break;
                case 'G':
                    board.entities.push_back({OGRE, {i, j}});
                    board.board[i][j] = OGRE;
                    break;
                case '@':
                    board.entities.push_back({CORPSE, {i, j}});
                    board.board[i][j] = CORPSE;
                    break;
                case '#':
                    board.board[i][j] = WALL;
                    break;
                case ' ':
                    board.board[i][j] = EMPTY;
                    break;
            }
        }
    }
    input.close();
    return board;
}

void writeSolution(const string& outputFile, const vector<Action>& actions, const GameBoard& finalState, int score) {
    ofstream output(outputFile);
    if (!output.is_open()) {
        cerr << "Failed to open " << outputFile << " for writing." << endl;
        return;
    }

    for (const auto& action : actions) {
        output << action.type;
    }
    output << endl;

    output << score << endl;

    for (const auto& row : finalState.board) {
        for (const auto& cell : row) {
            switch (cell) {
                case ACT_MAN: output << 'A'; break;
                case OGRE: output << 'G'; break;
                case DEMON: output << 'D'; break;
                case CORPSE: output << '@'; break;
                case WALL: output << '#'; break;
                case EMPTY: output << ' '; break;
                case ACT_MAN_DEAD: output << 'X'; break;
            }
        }
        output << '\n';
    }
    output.close();
}

pair<GameBoard, int> transitionFunction(const GameBoard& state, const Action& action, bool& isValid) {
    GameBoard newState = state;
    int updatedScore = newState.score;
    isValid = true;

    if (movementDirections.find(action.type) != movementDirections.end()) {
        Position newPos = newState.actManPosition + movementDirections[action.type];
        if (newPos.row >= 0 && newPos.row < newState.board.size() && newPos.col >= 0 && newPos.col < newState.board[0].size()) {
            if (newState.board[newPos.row][newPos.col] == WALL) {
                isValid = false; // Move is invalid if it's into a wall
            } else if (newState.board[newPos.row][newPos.col] == EMPTY) {
                newState.board[newState.actManPosition.row][newState.actManPosition.col] = EMPTY;
                newState.actManPosition = newPos;
                newState.board[newPos.row][newPos.col] = ACT_MAN;
                updatedScore -= 1;
            }
        } else {
            isValid = false; // Move is out of bounds
        }
    } else if (firingDirections.find(action.type) != firingDirections.end() && !newState.bulletFired) {
        newState.bulletFired = true;
        updatedScore -= 20;
        Position dir = firingDirections[action.type];
        Position currentPos = newState.actManPosition + dir;
        while (currentPos.row >= 0 && currentPos.row < newState.board.size() && currentPos.col >= 0 && currentPos.col < newState.board[0].size()) {
            if (newState.board[currentPos.row][currentPos.col] == DEMON || newState.board[currentPos.row][currentPos.col] == OGRE) {
                newState.board[currentPos.row][currentPos.col] = CORPSE;
                updatedScore += 5;
            } else if (newState.board[currentPos.row][currentPos.col] == WALL) {
                break;
            }
            currentPos = currentPos + dir;
        }
    }

    for (auto& entity : newState.entities) {
        if (entity.type == DEMON || entity.type == OGRE) {
            Position targetPos = newState.actManPosition;
            Position currentPos = entity.pos;
            Position moveDir = {0, 0};
            if (currentPos.row < targetPos.row) moveDir.row = 1;
            else if (currentPos.row > targetPos.row) moveDir.row = -1;
            if (currentPos.col < targetPos.col) moveDir.col = 1;
            else if (currentPos.col > targetPos.col) moveDir.col = -1;

            Position newPos = currentPos + moveDir;
            if (newState.board[newPos.row][newPos.col] == EMPTY) {
                newState.board[currentPos.row][currentPos.col] = EMPTY;
                entity.pos = newPos;
                newState.board[newPos.row][newPos.col] = entity.type;
            } else if (newState.board[newPos.row][newPos.col] == CORPSE || newState.board[newPos.row][newPos.col] == DEMON || newState.board[newPos.row][newPos.col] == OGRE) {
                newState.board[currentPos.row][currentPos.col] = EMPTY;
                newState.board[newPos.row][newPos.col] = CORPSE;
                updatedScore += 5;
            }
        }
    }

    return {newState, updatedScore};
}

bool isVictory(const GameBoard& state) {
    for (const auto& row : state.board) {
        for (const auto& cell : row) {
            if (cell == DEMON || cell == OGRE) {
                return false;
            }
        }
    }
    return true;
}

bool depthLimitedDFS(const GameBoard& initialState, vector<Action>& path, int& score, int depthLimit, unordered_set<size_t>& visited) {
    if (depthLimit < 0) {
        return false;
    }
    
    size_t stateHash = hashGameBoard(initialState);
    if (visited.find(stateHash) != visited.end()) {
        return false;
    }
    visited.insert(stateHash);

    if (isVictory(initialState)) {
        return true;
    }

    for (const auto& direction : movementDirections) {
        vector<Action> newPath = path;
        newPath.push_back({direction.first});
        bool isValid;
        pair<GameBoard, int> result = transitionFunction(initialState, {direction.first}, isValid);
        if (isValid && depthLimitedDFS(result.first, newPath, result.second, depthLimit - 1, visited)) {
            path = newPath;
            score = result.second;
            return true;
        }
    }

    for (const auto& direction : firingDirections) {
        vector<Action> newPath = path;
        newPath.push_back({direction.first});
        bool isValid;
        pair<GameBoard, int> result = transitionFunction(initialState, {direction.first}, isValid);
        if (isValid && depthLimitedDFS(result.first, newPath, result.second, depthLimit - 1, visited)) {
            path = newPath;
            score = result.second;
            return true;
        }
    }

    return false;
}

vector<Action> iddfs(const GameBoard& initialState) {
    int initialScore = initialState.score;
    int depthLimit = 0;
    while (true) { // Remove depth limit
        vector<Action> path;
        int score = initialScore;
        unordered_set<size_t> visited;
        if (depthLimitedDFS(initialState, path, score, depthLimit, visited)) {
            return path;
        }
        depthLimit++;
    }
    return {};
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <inputFile> <outputFile>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = argv[2];

    GameBoard initialState = readGameBoard(inputFile);
    vector<Action> solution = iddfs(initialState);
    GameBoard finalState = initialState;
    int finalScore = initialState.score;
    pair<GameBoard, int> result;
    for (const Action& action : solution) {
        bool isValid;
        result = transitionFunction(finalState, action, isValid);
        finalState = result.first;
        finalScore = result.second;
    }

    writeSolution(outputFile, solution, finalState, finalScore);
    return 0;
}