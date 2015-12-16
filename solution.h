#include <vector>
#include <string>
#include <iostream>
#include <cassert>

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


class RollingBalls {
public:
    vector<string> restorePattern(vector<string> start, vector<string> target) {
        ::H = start.size();
        ::W = start.front().size();
        assert(target.size() == ::H);
        assert(target.front().size() == ::W);

        for (int i = 0; i < H; i++) {
            cerr << start[i].c_str() << " " << target[i].c_str() << endl;
        }

        vector<string> result;
        return result;
    }
};
