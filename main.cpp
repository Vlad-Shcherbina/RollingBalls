#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace std;


#include "solution.h"


int main(int argc, char **argv) {
    (void)argc; (void)argv;  // suppress unused parameter warning

    int H;
    cin >> H;

    vector<string> start(H);
    for (auto &row : start)
        cin >> row;

    int H2;
    cin >> H2;
    assert(H == H2);

    vector<string> target(H);
    for (auto &row : target)
        cin >> row;

    auto result = RollingBalls().restorePattern(start, target);

    cout << result.size() << endl;
    for (auto s : result)
        cout << s.c_str() << endl;

    cout.flush();
    return 0;
}
