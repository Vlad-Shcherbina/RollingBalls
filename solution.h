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


typedef char Cell;
const Cell WALL = '#';
const Cell EMPTY = '.';
bool is_ball(Cell c) {
    if (c == WALL || c == EMPTY)
        return false;
    assert(c >= '0' && c <= '9');
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
    for (int d : {1, -1, ::W, -::W}) {
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
    for (int d : {1, -1, ::W, -::W}) {
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


// https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
class SCC {
public:
    struct Component {
        int id;
        vector<PackedCoord> vs;
        set<int> in_edges;  // component IDs
        set<int> out_edges;  // component IDs
    };

    vector<Component> components;
    map<PackedCoord, int> component_by_v;

    SCC(const Board &board)
        : board(board),
          visited(board.size(), false) {

        // only makes sense on empty board
        for (Cell c : board)
            assert(!is_ball(c));

        for (PackedCoord u = 0; u < board.size(); u++)
            if (board[u] == EMPTY)
                visit(u);

        reverse(L.begin(), L.end());

        for (PackedCoord u : L) {
            if (board[u] == EMPTY && component_by_v.count(u) == 0) {
                components.emplace_back();
                components.back().id = components.size() - 1;
                assign(u);
            }
        }

        for (PackedCoord u = 0; u < board.size(); u++) {
            if (board[u] == EMPTY) {
                int u_comp = component_by_v.at(u);
                for (PackedCoord v : gen_forward_rolls_vector(u, board)) {
                    int v_comp = component_by_v.at(v);
                    if (u_comp != v_comp) {
                        assert(u_comp < v_comp);
                        components[u_comp].out_edges.insert(v_comp);
                        components[v_comp].in_edges.insert(u_comp);
                    }
                }
            }
        }
    }

    const Component& get_component(PackedCoord p) const {
        return components[component_by_v.at(p)];
    }

private:
    const Board &board;
    vector<bool> visited;
    vector<PackedCoord> L;

    void visit(PackedCoord u) {
        if (visited[u])
            return;
        visited[u] = true;
        for (PackedCoord v : gen_forward_rolls_vector(u, board))
            visit(v);
        L.push_back(u);
    }

    void assign(PackedCoord u) {
        if (component_by_v.count(u) > 0)
            return;
        component_by_v[u] = components.size() - 1;
        components.back().vs.push_back(u);
        for (PackedCoord v : gen_backward_rolls_vector(u, board))
            assign(v);
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
        Board start(::W * ::H, WALL);
        Board target(::W * ::H, WALL);
        for (int i = 1; i < ::H - 1; i++) {
            for (int j = 1; j < ::W - 1; j++) {
                start[pack(j, i)] = raw_start[i - 1][j - 1];
                target[pack(j, i)] = raw_target[i - 1][j - 1];
            }
        }

        Board walls = start;
        for (auto &cell : walls)
            if (is_ball(cell))
                cell = EMPTY;

        for (int i = 0; i < ::H; i++) {
            for (int j = 0; j < ::W; j++) {
                cerr << walls[pack(j, i)] << ' ';
            }
            cerr << endl;
        }

        /*
        vector<pair<PackedCoord, PackedCoord>> all_forward_rolls;
        vector<pair<PackedCoord, PackedCoord>> all_backward_rolls;
        for (PackedCoord p = 0; p < ::W * ::H; p++) {
            if (walls[p] == EMPTY) {
                for (PackedCoord to : gen_forward_rolls_vector(p, walls))
                    all_forward_rolls.emplace_back(p, to);
                for (PackedCoord from : gen_backward_rolls_vector(p, walls))
                    all_backward_rolls.emplace_back(from, p);
            }
        }
        sort(all_forward_rolls.begin(), all_forward_rolls.end());
        sort(all_backward_rolls.begin(), all_backward_rolls.end());
        assert(all_forward_rolls == all_backward_rolls);
        */

        SCC scc(walls);
        debug(scc.component_by_v);
        for (auto c : scc.components) {
            cerr << "Component " << c.id << endl;
            debug(c.vs.size());
            debug(c.vs);
            debug(c.in_edges);
            debug(c.out_edges);
            cerr << "-------" << endl;
        }

        draw_board([&](PackedCoord p) {
            if (walls[p] == WALL) {
                ansi_style(DEFAULT_COLOR, true);
                cerr << "  ";
                return;
            }
            auto c = scc.get_component(p);
            if (c.in_edges.empty()) {
                ansi_style(RED);
                cerr << "x ";
                return;
            }
            if (c.vs.size() > 20) {
                ansi_style(GREEN);
                cerr << (c.id % 10) << " ";
                return;
            }
            cerr << "  ";
        });

        vector<string> result;
        return result;
    }
};
