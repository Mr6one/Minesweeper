#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <random>
#include <chrono>
#include <queue>
#include <unordered_set>
#include <iomanip>

#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>

using namespace std;

const int MAX_INT = 2147483647;

enum {
    mine = 10
};

void show_menu() {
    cout << "        Welcome to Minesweeper Game!        " << endl;
    cout << "Enter one of the following options (number):" << endl;
    cout << " ------------------------------------------ " << endl;
    cout << "|               (1) New Game               |" << endl;
    cout << " ------------------------------------------ " << endl;
    cout << "|               (2) Load Game              |" << endl;
    cout << " ------------------------------------------ " << endl;
    cout << "|              (3) How to Play             |" << endl;
    cout << " ------------------------------------------ " << endl;
    cout << "|                 (4) Exit                 |" << endl;
    cout << " ------------------------------------------ " << endl;
}

void show_rules() {
    cout << "* type [X, Y, Flag] (NO brackets) to mark cell as bomb" << endl;
    cout << "* type [X, Y, Open] (NO brackets) to open the cell" << endl;
    cout << "* '#' - free cell, '*' - bomb, space - no bombs nearby, 'f' - flag" << endl;
    cout << "* type 'Exit' (in game mode) to return to main menu" << endl;
    cout << "* type 'Help' (in game mode) to show this again" << endl;
}

class Board {
public:
    Board() = default;

    explicit Board(size_t M, size_t N, size_t mines_number);

    vector<size_t> size() const {
        vector<size_t> size = {board.size(), board[0].size()};
        return size;
    }

    bool is_mine(int i, int j) const {
        return board[i][j] == mine;
    }

    bool is_open(int i, int j) const {
        return !closed[i][j];
    }

    int board_value(int i, int j) const {
        return board[i][j];
    }

    bool flag(int i, int j) const {
        return is_flag[i][j];
    }

    void set_flag(int i, int j) {
        is_flag[i][j] = !is_flag[i][j];
        closed[i][j] = !closed[i][j];
    }

    size_t to_open() const {
        return cells_to_open;
    }

    size_t number_of_mines() const {
        return mines_number;
    }

    vector<pair<int, int>> get_neighbours(int x, int y);
    void open_cell(int i, int j);
    void generate(int X, int Y);
    void print_all_board() const;
    friend ostream& operator<<(ostream& stream, const Board& game_board);
    void load(const string& file_name);

private:
    size_t cells_to_open{};
    size_t mines_number{};

    vector<vector<bool>> is_flag;
    vector<vector<bool>> closed;
    vector<vector<int>> board;
};

Board::Board(size_t M, size_t N, size_t mines_number) : board(M), closed(M), is_flag(M), mines_number(mines_number) {
    for (size_t i = 0; i < M; ++i) {
        board[i].resize(N, 0);
        closed[i].resize(N, true);
        is_flag[i].resize(N, false);
    }
    cells_to_open = M * N - mines_number;
}

vector<pair<int, int>> Board::get_neighbours(int x, int y) {
    vector<pair<int, int>> neighbors;
    for (int i = -1; i < 2; ++i) {
        for (int j = -1; j < 2; ++j) {
            if ((i != 0 || j != 0) && x + i >= 0 && y + j >= 0 && x + i < (int)board.size() && y + j < (int)board[0].size()) {
                neighbors.emplace_back(make_pair(x + i, y + j));
            }
        }
    }

    return neighbors;
}

void Board::open_cell(int i, int j) {
    int N = (int)board[0].size();

    queue<int> stack;
    stack.push(i * N + j);
    unordered_set<int> visited;
    visited.insert(i * N + j);

    while (!stack.empty()) {
        int value = stack.front();
        stack.pop();

        --cells_to_open;
        closed[value / N][value % N] = false;
        is_flag[value / N][value % N] = false;

        if (board[value / N][value % N] == 0) {
            for (pair<int, int> neighbor: get_neighbours(value / N, value % N)) {
                if (visited.find(neighbor.first * N + neighbor.second) == visited.end()) {
                    stack.push(neighbor.first * N + neighbor.second);
                    visited.insert(neighbor.first * N + neighbor.second);
                }
            }
        }
    }
}

void Board::generate(int X, int Y) {
    random_device rd{};
    mt19937 rng{rd()};
    rng.seed(chrono::system_clock::now().time_since_epoch().count());

    double p = (double)mines_number / ((double)board.size() * board[0].size());
    bernoulli_distribution distribution(p);

    size_t mines_to_set = mines_number;
    vector<vector<int>> counter(board.size());
    int M = (int)board.size();
    int N = (int)board[0].size();
    for (size_t i = 0; i < board.size(); ++i) {
        counter[i].resize(board[i].size(), M * N - (min(X + 1, M) - max(X - 1, 1) + 1) * (min(Y + 1, N) - max(Y - 1, 1) + 1));
    }
    int value = counter[0][0];
    for (size_t i = 0; i < board.size(); ++i) {
        for (size_t j = 0; j < board[i].size(); ++j) {
            counter[i][j] = value;
            if (mines_to_set > 0 and distribution(rng) and ((int)i < X - 2 || (int)i > X || (int)j < Y - 2 || (int)j > Y)) {
                board[i][j] = mine;
                --mines_to_set;
            }
            if (((int)i >= X - 2) && ((int)i <= X) && ((int)j >= Y - 2) && ((int)j <= Y)) {
                counter[i][j] = value;
                ++value;
            }
            --value;
        }
    }

    if (mines_to_set > 0) {
        for (size_t i = 0; i < board.size(); ++i) {
            for (size_t j = 0; j < board[i].size(); ++j) {
                if (!((int)i >= X - 2 && (int)i <= X && (int)j >= Y - 2 && (int)j <= Y) && counter[i][j] != 0) {
                    p = (double)mines_to_set / (double)counter[i][j];
                    bernoulli_distribution distrib(p);

                    if (mines_to_set > 0 and distrib(rng)) {
                        board[i][j] = mine;
                        --mines_to_set;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < board.size(); ++i) {
        for (size_t j = 0; j < board[i].size(); ++j) {
            if (board[i][j] != mine) {
                for (int m = -1; m < 2; ++m) {
                    for (int n = -1; n < 2; ++n) {
                        int x_coord = (int)i + m;
                        int y_coord = (int)j + n;
                        if (x_coord >= 0 && x_coord < M && y_coord >= 0 && y_coord < N && board[x_coord][y_coord] == mine) {
                            board[i][j] += 1;
                        }
                    }
                }
            }
        }
    }
}

void Board::print_all_board() const {
    cout << "X\\Y ";
    for (size_t j = 0; j < board[0].size(); ++j) {
        cout << j + 1 << " ";
    }
    for (size_t i = 0; i < board.size(); ++i) {
        cout << endl;
        cout << i + 1 << "  |";
        for (size_t j = 0; j < board[0].size(); ++j) {
            if (board[i][j] == mine) {
                cout << "*|";
            } else if (is_flag[i][j]) {
                cout << " |";
            } else if (board[i][j] == 0) {
                cout << "0|";
            } else {
                cout << board[i][j] << "|";
            }
        }
    }

    cout << endl;
}

ostream &operator<<(ostream &stream, const Board &game_board) {
    stream << "X\\Y ";
    for (size_t j = 0; j < game_board.board[0].size(); ++j) {
        stream << j + 1 << " ";
    }
    for (size_t i = 0; i < game_board.board.size(); ++i) {
        stream << endl;
        stream << i + 1 << "  |";
        for (size_t j = 0; j < game_board.board[0].size(); ++j) {
            if (game_board.closed[i][j]) {
                stream << "#|";
            } else if (game_board.is_flag[i][j]) {
                stream << "f|";
            } else if (game_board.board[i][j] == 0) {
                stream << " |";
            } else {
                stream << game_board.board[i][j] << "|";
            }
        }
    }

    stream << endl;
    stream << "cells to open: " << game_board.cells_to_open;

    return stream;
}

void Board::load(const string &file_name) {
    fstream infile("save_folder/" + file_name);
    size_t M, N, mine_numbers, to_open;
    infile >> M >> N >> mine_numbers >> to_open;

    mines_number = mine_numbers;
    cells_to_open = to_open;

    board.resize(M);
    closed.resize(M);
    is_flag.resize(M);
    for (size_t i = 0; i < M; ++i) {
        board[i].resize(N, 0);
        closed[i].resize(N, true);
        is_flag[i].resize(N, false);

        for (size_t j = 0; j < N; ++j) {
            int is_mine, flag, value, is_open;
            infile >> is_mine >> flag >> value >> is_open;
            if (is_mine) {
                board[i][j] = mine;
            } else {
                board[i][j] = value ^ MAX_INT;
            }
            closed[i][j] = !is_open;
            is_flag[i][j] = flag;
        }
    }
}

size_t enter_size(const string& message, const string& error) {
    size_t board_size = 0;
    while (!board_size) {
        try {
            cout << message;
            if(cin >> board_size) {
                if (board_size < 5) {
                    cout << error << endl;
                    board_size = 0;
                }
            } else {
                cout << "Invalid input" << endl;
                board_size = 0;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
        }
        catch (exception const& e) {
            cout << "Invalid input" << endl;
            board_size = 0;
        }
    }

    return board_size;
}

Board initialize_board() {
    cout << "Enter the board size (MxN)" << endl;
    size_t M = enter_size("M (>=5):", "M is too small");
    size_t N = enter_size("N (>=5):", "N is too small");

    size_t mines_number = 0;
    size_t allow_mines = N * M - 9;
    while (!mines_number) {
        try {
            cout << "Enter number of mines (<=" << allow_mines << "):";
            cin >> mines_number;
            if (mines_number > allow_mines) {
                cout << "Too many mines" << endl;
                mines_number = 0;
            }
            if (mines_number == 0) {
                cout << "Enter positive value" << endl;
            }
        }
        catch (const exception& e) {
            cout << "Invalid input" << endl;
            mines_number = 0;
        }
    }

    Board board(M, N, mines_number);

    return board;
}

bool move(string& input, string& action, int& X, int& Y, size_t M, size_t N) {
    try {
        if (!input.empty()) {
            stringstream ss(input);

            char comma;
            ss >> X >> comma >> Y >> comma >> action;
            for(auto& c : action)
            {
                c = tolower(c, locale());
            }

            if (X < 1 || X > M) {
                cout << "Invalid X coordinate. Use help to see rules" << endl;
                return false;
            }
            if (Y < 1 || Y > N) {
                cout << "Invalid Y coordinate. Use help to see rules" << endl;
                return false;
            }
            if (action != "open" && action != "flag") {
                cout << "Invalid action input. Use help to see rules" << endl;
                return false;
            }
        }
    }
    catch(exception const& e) {
        cout << "Invalid input. Use help to see rules" << endl;
        return false;
    }

    return true;
}

void save_game(Board& board) {
    struct stat st = {0};

    if (stat("./save_folder/", &st) == -1) {
        mkdir("./save_folder/");
    }

    vector<size_t> board_size = board.size();
    size_t M = board_size[0];
    size_t N = board_size[1];
    size_t mine_number = board.number_of_mines();

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto str = oss.str();

    string file_name = to_string(M) + "x" + to_string(N) + "-" + to_string(mine_number) + " mines " + str;
    ofstream outfile ("./save_folder/" + file_name);

    outfile << M << ' ' << N << ' ' << mine_number << ' ' << board.to_open() << endl;
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            if (board.is_mine((int)i, (int)j)) {
                outfile << 1 << ' ';
            } else {
                outfile << 0 << ' ';
            }
            if (board.flag((int)i, (int)j)) {
                outfile << 1 << ' ';
            } else {
                outfile << 0 << ' ';
            }
            outfile << (board.board_value((int)i, (int)j) ^ MAX_INT) << ' ';
            outfile << board.is_open((int)i, (int)j);
            outfile << endl;
        }
    }
}

string load_save_file() {
    DIR *d;
    struct dirent *dir;
    d = opendir("save_folder");
    vector<string> files;
    if (d) {
        while ((dir = readdir(d)) != nullptr) {
            string file_name(dir->d_name);
            if (file_name != "." && file_name != "..") {
                files.push_back(file_name);
            }
        }
        closedir(d);
    }

    if (files.empty()) {
        return "";
    }

    cout << "Chose file number (" << 1 << "-" << files.size() << "):" << endl;
    for (size_t i = 0; i < files.size(); ++i) {
        cout << "(" << i + 1 << ") " << files[i] << endl;
    }

    int file_number = 0;
    while (!file_number) {
        try {
            if(cin >> file_number) {
                if (file_number < 1 || file_number > files.size()) {
                    cout << "Invalid input" << endl;
                    file_number = 0;
                }
            } else {
                cout << "Invalid input" << endl;
                file_number = 0;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
        }
        catch (exception const& e) {
            cout << "Invalid input" << endl;
            file_number = 0;
        }
    }

    return files[file_number - 1];
}

void game(bool new_game = true) {
    Board board;
    bool first_move;

    if (new_game) {
        board = initialize_board();
        first_move = true;
    } else {
        string file_name = load_save_file();
        if (file_name.empty()) {
            cout << "No save files found" << endl;
            return;
        }
        board.load(file_name);
        first_move = false;
    }

    while (true) {
        string input;

        int X = -1, Y = -1;
        string action;
        while (true) {
            getline(cin, input);
            input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
            input.erase(std::remove(input.begin(), input.end(), ' '), input.end());

            string tmp = input;
            for (auto &c: tmp) {
                c = tolower(c, locale());
            }

            if (tmp == "help") {
                show_rules();
                continue;
            }
            if (tmp == "exit") {
                if (first_move) {
                    cout << "Do at least one move" << endl;
                    continue;
                }
                cout << "Do you want to save the game (yes - to save, no - otherwise)?" << endl;

                string answer;

                while (answer.empty()) {
                    try {
                        cin >> answer;
                        answer.erase(std::remove(answer.begin(), answer.end(), '\n'), answer.end());
                        answer.erase(std::remove(answer.begin(), answer.end(), ' '), answer.end());

                        for (auto &c: tmp) {
                            c = tolower(c, locale());
                        }

                        if (answer == "yes") {
                            save_game(board);
                        } else if (answer != "no") {
                            cout << "Invalid input. Enter 'yes' or 'no'" << endl;
                            answer = "";
                        }
                    }
                    catch (const exception& e) {
                        cout << "Invalid input. Enter 'yes' or 'no'" << endl;
                        answer = "";
                    }
                }

                show_menu();
                return;
            }

            vector<size_t> board_size = board.size();
            if (!move(input, action, X, Y, board_size[0], board_size[1])) {
                continue;
            }

            break;
        }

        if (X != -1 && Y != -1) {
            if (first_move) {
                board.generate(X, Y);
                first_move = false;
            }

            if (board.is_open(X - 1, Y - 1) && !board.flag(X - 1, Y - 1)) {
                cout << "Cell is open already. Try another coordinates" << endl;
                continue;
            }

            if (action == "open" && board.is_mine(X - 1, Y - 1)) {
                board.print_all_board();
                cout << " ------------ " << endl;
                cout << "| You Lost:( |" << endl;
                cout << " ------------ " << endl;
                break;
            } else if (action == "flag") {
                board.set_flag(X - 1, Y - 1);
            } else {
                board.open_cell(X - 1, Y - 1);
            }

            if (board.to_open() == 0) {
                board.print_all_board();
                cout << " ---------- " << endl;
                cout << "| You Won! |" << endl;
                cout << " ---------- " << endl;
                break;
            }
        }

        cout << board << endl;
    }
}

int main() {
    show_menu();
    while (true) {
        string input;
        getline(cin, input);
        try {
            int option = stoi(input);
            if (option == 1) {
                game();
                show_menu();
            } else if (option == 2) {
                game(false);
            } else if (option == 3) {
                show_rules();
            } else if (option == 4) {
                break;
            } else {
                cout << "Choose appropriate option" << endl;
            }
        }
        catch (invalid_argument const& ex) {
            cout << "Choose appropriate option" << endl;
        }
    }
    return 0;
}
