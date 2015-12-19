#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <map>
#include <sstream>
#include <cstdint>

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
Move reversed_move(Move move) {
    return {move.second, move.first};
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
#ifdef DRAW_BOARDS
    for (int i = 1; i < ::H - 1; i++) {
        for (int j = 1; j < ::W - 1; j++) {
            draw_cell_fn(pack(j, i));
            ansi_default();
        }
        cerr << "|" << endl;
    }
#endif
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


void apply_move(Board &board, Move move) {
    auto tos = gen_forward_rolls_vector(move.first, board);
    assert(find(tos.begin(), tos.end(), move.second) != tos.end());
    board[move.second] = board[move.first];
    board[move.first] = EMPTY;
}


enum CellSet {
    CS_UNKNOWN = 0,
    CS_EMPTY = 1,
    CS_ANY_BALL = 2,
    CS_FIRST_BALL = CS_ANY_BALL + 1,
    // TODO: check that there are no off-by-ones
    CS_LAST_BALL = CS_ANY_BALL + 10,
    CS_WALL = 13,
    CS_CONTRADICTION = 123
};
bool is_valid_cs(CellSet s) {
    return
        s == CS_UNKNOWN ||
        s == CS_EMPTY ||
        (s >= CS_ANY_BALL && s <= CS_LAST_BALL) ||
        s == CS_WALL;
}
bool cs_is_ball(CellSet s) {
    assert(is_valid_cs(s));
    return s >= CS_ANY_BALL && s <= CS_LAST_BALL;
}
CellSet cell_to_cs(Cell c) {
    if (c == EMPTY)
        return CS_EMPTY;
    if (c == WALL)
        return CS_WALL;
    assert(is_ball(c));
    return CellSet(CS_FIRST_BALL + (c - '0'));
}
char cs_to_char(CellSet s) {
    switch (s) {
    case CS_UNKNOWN:
        return ' ';
    case CS_EMPTY:
        return '.';
    case CS_ANY_BALL:
        return '?';
    case CS_WALL:
        return 'W';
    case CS_CONTRADICTION:
        return '!';
    default:
        assert(s >= CS_FIRST_BALL && s <= CS_LAST_BALL);
        return '0' + (s - CS_FIRST_BALL);
    }
}

CellSet combine_cs_with_empty(CellSet s) {
    if (s == CS_UNKNOWN || s == CS_EMPTY)
        return CS_EMPTY;
    else
        return CS_CONTRADICTION;
}
CellSet combine_cs_with_concrete_ball(CellSet s, Cell ball) {
    assert(is_ball(ball));
    CellSet concrete_ball = CellSet(ball - '0' + 1 + CS_ANY_BALL);
    if (s == CS_UNKNOWN || s == CS_ANY_BALL || s == concrete_ball)
        return concrete_ball;
    else
        return CS_CONTRADICTION;
}
CellSet combine_cs_with_any_ball(CellSet s) {
    assert(is_valid_cs(s));
    if (s == CS_UNKNOWN)
        return CS_ANY_BALL;
    else if (cs_is_ball(s))
        return s;
    else
        return CS_CONTRADICTION;
}
CellSet combine_with_obstacle(CellSet s) {
    assert(is_valid_cs(s));
    if (s == CS_WALL)
        return CS_WALL;
    return combine_cs_with_any_ball(s);
}


typedef int Conflict;
const Conflict NO_CONFLICT = 0;
const Conflict CONFLICT_FILL = 1;
const Conflict CONFLICT_CLEAR = 2;
const Conflict CONFLICT_REPLACE = 3;


class State {
public:
    State(const Board &initial_board, map<PackedCoord, CellSet> goal)
        : initial_board(initial_board), cur(initial_board.size(), CS_UNKNOWN) {

        PackedCoord p = 0;
        for (Cell cell : initial_board) {
            if (cell == WALL)
                cur[p] = CS_WALL;
            p++;
        }

        for (auto kv : goal) {
            edit_cur(kv.first, kv.second);
        }

        // assert(check_conflicts());
    }

    void show() {
        // assert(check_conflicts());
        draw_board([this](PackedCoord p) {
            auto cs = cur[p];
            auto c = initial_board[p];

            if (c == WALL) {
                assert(cs == CS_WALL);
                ansi_style(DEFAULT_COLOR, true);
                cerr << "  ";
                return;
            }

            if (cs != CS_UNKNOWN) {
                switch (conflict_type(p)) {
                case NO_CONFLICT:
                    ansi_style(GREEN);
                    break;
                case CONFLICT_FILL:
                    ansi_style(GREEN, true);
                    break;
                case CONFLICT_CLEAR:
                    ansi_style(RED, true);
                    break;
                case CONFLICT_REPLACE:
                    ansi_style(YELLOW, true);
                    break;
                default:
                    assert(false);
                }
            }
            cerr << cs_to_char(cs);
            cerr << (c == EMPTY ? ' ' : c);
        });
        // cerr << "moves: ";
        // for (auto move : moves)
        //     cerr << unpack_move(move) << " ";
        // cerr << endl;
        // cerr << "conflicts: ";
        // for (auto p : conflicts)
        //     cerr << unpack(p) << " ";
        // cerr << endl;
        // debug2(undo_log, conflict_undo_log);
        // cerr << endl;
    }

    void enumerate_moves(function<void(Move move)> callback) {
        // TODO: switch to sorted vector and binsearch
        // (when building it we can be certain al items are unique)
        set<PackedCoord> explored_froms;

        vector<PackedCoord> stage0_froms;
        for (PackedCoord from : conflicts) {
            Conflict ct = conflict_type(from);
            assert(ct != NO_CONFLICT);
            if (ct != CONFLICT_FILL)
                stage0_froms.push_back(from);
        }
        set<PackedCoord> stage1_froms;

        for (int stage = 0; stage < 2; stage ++) {
            // stage 0: expand from non-fill conflicts
            // stage 1: expand from removable obstacles we saw on stage 1

            vector<PackedCoord> froms_to_explore;
            if (stage == 0) {
                froms_to_explore = stage0_froms;
            } else if (stage == 1) {
                for (auto from : stage0_froms)
                    stage1_froms.erase(from);
                froms_to_explore = {stage1_froms.begin(), stage1_froms.end()};
            } else {
                assert(false);
            }

            for (PackedCoord from : froms_to_explore) {
                explored_froms.insert(from);

                for (int dir : DIRS) {
                    CellSet fulcrum = combine_with_obstacle(cur[from - dir]);
                    if (fulcrum == CS_CONTRADICTION)
                        continue;

                    RestorePoint rp(*this);
                    edit_cur(from - dir, fulcrum);

                    // TODO: combine with any ball just in case
                    CellSet rolling_ball = cur[from];
                    edit_cur(from, CS_EMPTY);

                    PackedCoord p = from + dir;
                    while (true) {
                        auto e = combine_cs_with_empty(cur[p]);
                        if (e == CS_CONTRADICTION) {
                            if (cur[p] != CS_WALL && stage == 0) {
                                stage1_froms.insert(p);
                            }
                            break;
                        }

                        {
                            RestorePoint rp2(*this);
                            edit_cur(p, rolling_ball);
                            // assert(check_conflicts());
                            callback({from, p});
                        }

                        edit_cur(p, e);
                        p += dir;
                    }
                }
            }
        }

        auto conflicts_copy = conflicts;  // they will be modified
        for (PackedCoord to : conflicts_copy) {
            Conflict ct = conflict_type(to);
            assert(ct != NO_CONFLICT);
            if (ct != CONFLICT_FILL)
                continue;

            for (int dir : DIRS) {
                RestorePoint rp(*this);
                PackedCoord p = to - dir;
                while (true) {
                    auto rolling_ball = combine_cs_with_any_ball(cur[p]);
                    if (rolling_ball != CS_CONTRADICTION) {
                        auto fulcrum = combine_with_obstacle(cur[p - dir]);
                        if (fulcrum != CS_CONTRADICTION &&
                            explored_froms.count(p) == 0) {
                            RestorePoint rp2(*this);
                            edit_cur(p - dir, fulcrum);
                            edit_cur(p, CS_EMPTY);
                            edit_cur(to, rolling_ball);
                            // assert(check_conflicts());

                            callback({p, to});
                        }
                    }

                    auto e = combine_cs_with_empty(cur[p]);
                    if (e == CS_CONTRADICTION)
                        break;
                    edit_cur(p, e);
                    p -= dir;
                }
            }
        }

    }

    void apply_move(Move move) {
        PackedCoord from = move.first;
        PackedCoord to = move.second;
        int dir = move_dir(move);

        CellSet fulcrum = combine_with_obstacle(cur[from - dir]);
        assert(fulcrum != CS_CONTRADICTION);
        edit_cur(from - dir, fulcrum);

        CellSet rolling_ball = combine_cs_with_any_ball(cur[from]);
        assert(rolling_ball != CS_CONTRADICTION);

        edit_cur(from, CS_EMPTY);

        PackedCoord p = from + dir;
        while (p != to) {
            auto e = combine_cs_with_empty(cur[p]);
            assert(e != CS_CONTRADICTION);
            edit_cur(p, e);
            p += dir;
        }

        auto dst = combine_cs_with_empty(cur[to]);
        assert(dst != CS_CONTRADICTION);
        edit_cur(to, rolling_ball);
    }

    const vector<PackedCoord>& get_conflicts() const { return conflicts; }

    class RestorePoint {
    public:
        RestorePoint(State &state) : state(state) {
            undo_log_size = state.undo_log.size();
            conflict_undo_log_size = state.conflict_undo_log.size();
        }
        ~RestorePoint() {
            assert(state.undo_log.size() >= undo_log_size);
            while (state.undo_log.size() > undo_log_size) {
                state.cur[state.undo_log.back().first] = state.undo_log.back().second;
                state.undo_log.pop_back();
            }

            assert(state.conflict_undo_log.size() >= conflict_undo_log_size);
            while (state.conflict_undo_log.size() > conflict_undo_log_size) {
                auto &q = state.conflict_undo_log.back();
                if (q.second) {
                    state.conflicts.push_back(q.first);
                } else {
                    auto it = find(state.conflicts.begin(), state.conflicts.end(), q.first);
                    assert(it != state.conflicts.end());
                    swap(*it, state.conflicts.back());
                    state.conflicts.pop_back();
                }
                state.conflict_undo_log.pop_back();
            }
        }
    private:
        State &state;
        int undo_log_size;
        int conflict_undo_log_size;
    };

    Conflict conflict_type(PackedCoord p) const {
        assert(initial_board[p] != WALL);

        if (initial_board[p] == EMPTY) {
            if (combine_cs_with_empty(cur[p]) == CS_CONTRADICTION)
                return CONFLICT_CLEAR;
            else
                return NO_CONFLICT;
        } else {
            assert(is_ball(initial_board[p]));
            if (cur[p] == CS_EMPTY)
                return CONFLICT_FILL;

            if (combine_cs_with_concrete_ball(cur[p], initial_board[p]) ==
                CS_CONTRADICTION)
                return CONFLICT_REPLACE;
            else
                return NO_CONFLICT;
        }
    }

private:
    const Board &initial_board;
    vector<CellSet> cur;
    vector<PackedCoord> conflicts;

    vector<pair<PackedCoord, CellSet>> undo_log;
    // true to add, false to remove
    vector<pair<PackedCoord, bool>> conflict_undo_log;

    void edit_cur(PackedCoord p, CellSet new_cs) {
        assert(is_valid_cs(new_cs));
        if (cur[p] != new_cs) {
            assert(initial_board[p] != WALL);
            assert(new_cs != CS_WALL);

            undo_log.emplace_back(p, cur[p]);

            Conflict old_conflict = conflict_type(p);
            cur[p] = new_cs;
            Conflict new_conflict = conflict_type(p);

            if (old_conflict == NO_CONFLICT && new_conflict != NO_CONFLICT) {
                conflicts.push_back(p);
                conflict_undo_log.emplace_back(p, false);
            } else if (old_conflict != NO_CONFLICT && new_conflict == NO_CONFLICT) {
                auto it = find(conflicts.begin(), conflicts.end(), p);
                assert(it != conflicts.end());
                swap(*it, conflicts.back());
                conflicts.pop_back();
                conflict_undo_log.emplace_back(p, true);
            }
        }
    }

    bool check_conflicts() const {
        for (PackedCoord p = 0; p < initial_board.size(); p++) {
            if (initial_board[p] == WALL)
                continue;
            auto it = find(conflicts.begin(), conflicts.end(), p);
            if (conflict_type(p) == NO_CONFLICT)
                assert(it == conflicts.end());
            else
                assert(it != conflicts.end());
        }
        return true;
    }
};


class Backtracker {
public:
    bool solved;
    vector<Move> solution;

    Backtracker(State &state, int min_depth, int max_depth) : state(state) {
        solved = false;
        for (int depth = min_depth; depth <= max_depth; depth++) {
            debug(depth);
            rec(depth);
            debug(cnt);
            if (solved)
                break;
        }
        // debug(solved);

        // State::RestorePoint rp(state);
        // for (auto move : solution) {
        //     debug(unpack_move(move));
        //     state.apply_move(move);
        //     state.show();
        // }

        // debug(solution.size());
    }

private:
    State &state;
    vector<Move> moves;
    int64_t cnt = 0;

    void rec(int depth) {
        if (solved)
            return;

        cnt++;

        if (state.get_conflicts().empty()) {
            solved = true;
            solution = moves;
        }

        int n1 = 0;
        int n2 = 0;
        for (auto p : state.get_conflicts()) {
            switch (state.conflict_type(p)) {
            case NO_CONFLICT:
                break;
            case CONFLICT_CLEAR:
                n1++;
                break;
            case CONFLICT_FILL:
                n2++;
                break;
            case CONFLICT_REPLACE:
                n1++;
                n2++;
                break;
            default:
                assert(false);
            }
        }
        if (max(n1, n2) > depth)
            return;
        // TODO: same line heuristic

        state.enumerate_moves([this, depth](Move move){
            moves.push_back(move);

            // TODO: commutativity prunning
            rec(depth - 1);

            assert(moves.back() == move);
            moves.pop_back();
        });
    }
};


void show_start_and_target(const Board &start, const Board &target) {
    map<PackedCoord, CellSet> goal;
    for (PackedCoord p = 0; p < start.size(); p++) {
        if (is_ball(target[p]))
            goal[p] = cell_to_cs(target[p]);
    }
    State state(start, goal);
    state.show();
}


class RollingBalls {
public:
    vector<string> restorePattern(vector<string> raw_start, vector<string> raw_target) {
        vector<string> result;

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

        Board board = start;

        ////////////
        map<PackedCoord, CellSet> goal;
        for (PackedCoord p = 0; p < board.size(); p++) {
            if (is_ball(target[p])) {
                goal[p] = cell_to_cs(target[p]);
                if (goal.size() == 2)
                    break;
            }
        }
        goal.erase(goal.begin()->first);

        State state(board, goal);
        state.show();
        Backtracker bt(state, 1, 11);
        debug(bt.solved);
        if (bt.solved) {
            for (auto move : bt.solution) {
                debug(unpack_move(move));
                state.apply_move(move);
                state.show();
            }

            auto sol = bt.solution;
            reverse(sol.begin(), sol.end());
            for (auto move : sol) {
                move = reversed_move(move);
                apply_move(board, move);
                result.push_back(format_move(move));
            }
        }
        ////////////

        /*
        int depth = 1;
        while (depth <= 6) {
            show_start_and_target(board, target);
            debug(depth);
            int num_improvements = 0;

            for (PackedCoord p = 0; p < board.size(); p++) {
                if (is_ball(target[p]) && target[p] != board[p]) {
                    map<PackedCoord, CellSet> goal;
                    goal[p] = cell_to_cs(target[p]);

                    // keep all already achieved ones
                    for (PackedCoord p2 = 0; p2 < board.size(); p2++) {
                        if (is_ball(target[p2]) && target[p2] == board[p2])
                            goal[p2] = cell_to_cs(target[p2]);
                    }

                    State state(board, goal);

                    Backtracker bt(state, 1, depth);
                    if (bt.solved) {
                        auto sol = bt.solution;
                        reverse(sol.begin(), sol.end());
                        for (auto move : sol) {
                            move = reversed_move(move);
                            apply_move(board, move);
                            result.push_back(format_move(move));
                        }
                        num_improvements++;
                    }
                }
            }
            debug(num_improvements);
            depth++;
        }*/

        int num_balls = 0;
        for (auto c : target)
            if (is_ball(c))
                num_balls++;
        debug(num_balls);
        debug(num_balls * 20 - result.size());
        debug(result.size());

        double score = 0.0;
        for (PackedCoord p = 0; p < board.size(); p++) {
            if (is_ball(target[p])) {
                if (board[p] == target[p])
                    score += 1.0;
                else if (is_ball(board[p]))
                    score += 0.5;
            }
        }
        if (num_balls > 0)
            score /= num_balls;
        debug(score);
        return result;
    }

private:
    static string format_move(Move move) {
        int dir = move_dir(move);
        if (dir == -::W) {
            dir = 3;
        } else if (dir == -1) {
            dir = 0;
        } else if (dir == 1) {
            dir = 2;
        } else if (dir == ::W) {
            dir = 1;
        } else {
            assert(false);
        }
        ostringstream out;
        out << unpack_y(move.first) - 1 << ' '
            << unpack_x(move.first) - 1 << ' '
            << dir;
        return out.str();
    }
};
