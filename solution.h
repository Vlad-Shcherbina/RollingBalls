#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <map>

#include "pretty_printing.h"

using namespace std;


#define debug(x) \
    cerr << #x " = " << (x) << endl
#define debug2(x, y) \
    cerr << #x " = " << (x) \
    << ", " #y " = " << (y) << endl
#define debug3(x, y, z) \
    cerr << #x " = " << (x) \
    << ", " #y " = " << (y) \
    << ", " #z " = " << (z) << endl


int W = -1;
int H = -1;


array<int, 4> DIRS;


typedef char Cell;
const Cell WALL = '#';
const Cell EMPTY = '.';
bool is_valid_cell(Cell c) {
    return c == WALL || c == EMPTY || (c >= '0' && c <= '9');
}
bool is_ball(Cell c) {
    assert(is_valid_cell(c));
    if (c == WALL || c == EMPTY)
        return false;
    return true;
}


typedef vector<Cell> Board;


typedef int PackedCoord;
PackedCoord pack(int x, int y) {
    return x + y * ::W;
}
int unpack_x(PackedCoord p) {
    return p % ::W;
}
int unpack_y(PackedCoord p) {
    return p / ::W;
}
pair<int, int> unpack(PackedCoord p) {
    return {unpack_x(p), unpack_y(p)};
}


typedef pair<int, int> Move;
int move_dir(Move move) {
    int d = move.second - move.first;
    if (d <= -::W)
        return -::W;
    if (d < 0)
        return -1;
    assert(d != 0);
    if (d < ::W)
        return 1;
    return ::W;
}
pair<pair<int, int>, pair<int, int>> unpack_move(Move move) {
    return {unpack(move.first), unpack(move.second)};
}


const int DEFAULT_COLOR = -1;
const int RED = 31;
const int GREEN = 32;
const int YELLOW = 33;
const int BLUE = 34;
const int MAGENTA = 35;
const int CYAN = 36;
const int WHITE = 37;
void ansi_style(int color, bool inverse=false) {
    if (inverse)
        cerr << "\033[7";
    else
        cerr << "\033[0";
    if (color != DEFAULT_COLOR) {
        assert(color >= 30);
        assert(color <= 37);
        cerr << ";" << color;
    }
    cerr << "m";
}
void ansi_default() {
    cerr << "\033[0m";
}


void draw_board(function<void(PackedCoord)> draw_cell_fn) {
    for (int i = 1; i < ::H - 1; i++) {
        for (int j = 1; j < ::W - 1; j++) {
            draw_cell_fn(pack(j, i));
            ansi_default();
        }
        cerr << "|" << endl;
    }
}


template<typename OUT_ITER>
OUT_ITER gen_forward_rolls(
    PackedCoord from, const Board &board, OUT_ITER result) {

    assert(board[from] != WALL);
    for (int d : DIRS) {
        PackedCoord pos = from;
        while (board[pos + d] == EMPTY)
            pos += d;
        if (pos != from) {
            *result = pos; ++result;
        }
    }
    return result;
}
vector<PackedCoord> gen_forward_rolls_vector(
    PackedCoord from, const Board &board) {

    vector<PackedCoord> result;
    gen_forward_rolls(from, board, back_inserter(result));
    return result;
}


template<typename OUT_ITER>
OUT_ITER gen_backward_rolls(
    PackedCoord to, const Board &board, OUT_ITER result) {

    assert(board[to] == EMPTY);
    for (int d : DIRS) {
        if (board[to - d] != EMPTY) {
            PackedCoord pos = to + d;
            while (board[pos] == EMPTY) {
                *result = pos; ++result;
                pos += d;
            }
        }
    }
    return result;
}
vector<PackedCoord> gen_backward_rolls_vector(
    PackedCoord from, const Board &board) {

    vector<PackedCoord> result;
    gen_backward_rolls(from, board, back_inserter(result));
    return result;
}


typedef int CellSet;
const CellSet CS_UNKNOWN = 0;
const CellSet CS_EMPTY = 1;
const CellSet CS_ANY_BALL = 2;
// TODO: check that there are no off-by one errors for balls '0' and '9'
const CellSet CS_CONTRADICTION = 123;
bool is_valid_cs(CellSet s) {
    return
        s == CS_UNKNOWN ||
        s == CS_EMPTY ||
        (s >= CS_ANY_BALL && s <= CS_ANY_BALL + 10) ||
        s == CS_CONTRADICTION;
}
bool cs_is_ball(CellSet s) {
    assert(is_valid_cs(s));
    return s >= CS_ANY_BALL && s <= CS_ANY_BALL + 10;
}
CellSet cell_to_cs(Cell c) {
    if (c == EMPTY) {
        return 0;
    }
    assert(is_ball(c));
    return CS_ANY_BALL + 1 + (c - '0');
}
char cs_to_char(CellSet s) {
    switch (s) {
    case CS_UNKNOWN:
        return ' ';
    case CS_EMPTY:
        return '.';
    case CS_ANY_BALL:
        return '?';
    case CS_CONTRADICTION:
        return '!';
    default:
        assert(s > CS_ANY_BALL && s <= CS_ANY_BALL + 10);
        return '0' + (s - CS_ANY_BALL - 1);
    }
}
CellSet combine_cs_with_cell(CellSet s, Cell c) {
    assert(is_valid_cs(s));
    assert(s != CS_CONTRADICTION);
    assert(is_valid_cell(c));
    assert(c != WALL);

    if (c == EMPTY) {
        if (s == CS_UNKNOWN || s == CS_EMPTY)
            return CS_EMPTY;
        else
            return CS_CONTRADICTION;
    } else {
        assert(is_ball(c));
        CellSet concrete_ball = c - '0' + 1 + CS_ANY_BALL;
        if (s == CS_UNKNOWN || s == CS_ANY_BALL || s == concrete_ball)
            return concrete_ball;
        else
            return CS_CONTRADICTION;
    }
}
CellSet combine_cs_with_any_ball(CellSet s) {
    assert(is_valid_cs(s));
    assert(s != CS_CONTRADICTION);
    if (s == CS_UNKNOWN)
        return CS_ANY_BALL;
    else if (cs_is_ball(s))
        return s;
    else
        return CS_CONTRADICTION;
}


template <typename K, typename V>
V map_get_default(const map<K, V> &m, K k, V def) {
    auto p = m.find(k);
    if (p != m.end())
        return p->second;
    else
        return def;
}


typedef map<PackedCoord, CellSet> Infoset;
CellSet infoset_get(const Infoset &infoset, PackedCoord p) {
    return map_get_default(infoset, p, CS_UNKNOWN);
}


void show_infoset(const Infoset &infoset, const Board &initial_board) {
    draw_board([&](PackedCoord p) {
        auto cs = infoset.count(p) > 0 ? infoset.at(p) : CS_UNKNOWN;
        auto c = initial_board[p];

        if (c == WALL) {
            ansi_style(DEFAULT_COLOR, true);
            cerr << "  ";
            return;
        }

        if (combine_cs_with_cell(cs, c) == CS_CONTRADICTION)
            ansi_style(RED, true);
        else
            ansi_style(GREEN);
        cerr << cs_to_char(cs);

        ansi_default();
        cerr << c;
    });
}


class Backtracker {
public:
    Backtracker(const Board &initial_board, const Infoset &goal) :
        initial_board(initial_board) {

        solved = false;
        for (int depth = 1; depth <= 8; depth++) {
            debug(depth);
            rec(goal, depth);
            if (solved)
                break;
        }
        debug(solved);

        cerr << "=================" << endl;

        Infoset current = goal;
        show_infoset(current, initial_board);
        for (auto move : solution) {
            debug(unpack_move(move));
            apply_move(current, move);
            show_infoset(current, initial_board);
        }
    }
private:
    const Board &initial_board;
    vector<Move> moves;

    bool solved;
    vector<Move> solution;

    void rec(const Infoset &current, int depth) {
        if (solved)
            return;
        vector<PackedCoord> need_fill;
        vector<PackedCoord> need_clear;
        for (const auto &kv : current) {
            Cell c = initial_board[kv.first];
            assert(c != WALL);
            CellSet cs = kv.second;

            if (combine_cs_with_cell(cs, c) == CS_CONTRADICTION) {
                if (is_ball(c))
                    need_fill.push_back(kv.first);
                if (combine_cs_with_any_ball(cs) != CS_CONTRADICTION)
                    need_clear.push_back(kv.first);
            }
        }

        if (need_fill.empty() && need_clear.empty()) {
            solved = true;
            solution = moves;
            return;
        }

        if (max(need_fill.size(), need_clear.size()) > depth)
            return;

        for (PackedCoord p : need_clear) {
            auto moves = gen_moves_from(current, p);
            for (const Move &move : moves) {
                Infoset next = current;
                apply_move(next, move);
                this->moves.push_back(move);
                rec(next, depth - 1);
                this->moves.pop_back();

                if (solved)
                    break;
            }
        }
    }

    vector<Move> gen_moves_from(const Infoset &infoset, PackedCoord from) {
        assert(initial_board[from] != WALL);
        assert(
            combine_cs_with_any_ball(infoset_get(infoset, from)) !=
            CS_CONTRADICTION);

        vector<Move> result;
        for (int dir : DIRS) {
            bool has_fulcrum = false;
            if (initial_board[from - dir] == WALL) {
                has_fulcrum = true;
            } else {
                CellSet fulcrum = infoset_get(infoset, from - dir);
                if (combine_cs_with_any_ball(fulcrum) != CS_CONTRADICTION)
                    has_fulcrum = true;
            }
            if (!has_fulcrum)
                continue;

            PackedCoord p = from + dir;
            while (initial_board[p] != WALL) {
                CellSet cs = infoset_get(infoset, p);
                if (combine_cs_with_cell(cs, EMPTY) == CS_CONTRADICTION)
                    break;
                result.emplace_back(from, p);
                p += dir;
            }
        }
        return result;
    }

    void apply_move(Infoset &infoset, Move move) {
        int dir = move_dir(move);
        if (initial_board[move.first - dir] != WALL) {
            CellSet &fulcrum = infoset[move.first - dir];
            fulcrum = combine_cs_with_any_ball(fulcrum);
            assert(fulcrum != CS_CONTRADICTION);
        }

        PackedCoord p = move.first + dir;
        while (p != move.second) {
            CellSet &cs = infoset[p];
            cs = combine_cs_with_cell(cs, EMPTY);
            assert(cs != CS_CONTRADICTION);
            p += dir;
        }

        CellSet &from_cs = infoset[move.first];
        CellSet &to_cs = infoset[move.second];

        from_cs = combine_cs_with_any_ball(from_cs);
        assert(from_cs != CS_CONTRADICTION);

        assert(combine_cs_with_cell(to_cs, EMPTY) != CS_CONTRADICTION);
        to_cs = from_cs;
        from_cs = CS_EMPTY;
    }
};


class RollingBalls {
public:
    vector<string> restorePattern(vector<string> raw_start, vector<string> raw_target) {
        ::H = raw_start.size();
        ::W = raw_start.front().size();
        assert(raw_target.size() == ::H);
        assert(raw_target.front().size() == ::W);

        ::H += 2;
        ::W += 2;
        DIRS = {{1, -1, ::W, -::W}};

        Board start(::W * ::H, WALL);
        Board target(::W * ::H, WALL);
        for (int i = 1; i < ::H - 1; i++) {
            for (int j = 1; j < ::W - 1; j++) {
                start[pack(j, i)] = raw_start[i - 1][j - 1];
                target[pack(j, i)] = raw_target[i - 1][j - 1];
            }
        }

        Infoset goal;
        for (PackedCoord p = 0; p < target.size(); p++) {
            if (is_ball(target[p])) {
                goal[p] = cell_to_cs(target[p]);
                if (goal.size() == 1)
                    break;
            }
        }
        Backtracker bt(start, goal);

        vector<string> result;
        return result;
    }
};
