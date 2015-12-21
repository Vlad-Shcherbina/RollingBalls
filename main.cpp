#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <thread>

using namespace std;

#define DRAW_BOARDS

// 'cause topcoder requires single file submission
#include "solution.cpp"


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

    // To sort of ensure that ErrorReader thread in tester will get a chance
    // to pick all stderr up.
    cerr.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    cout << result.size() << endl;
    for (auto s : result)
        cout << s.c_str() << endl;

    cout.flush();
    return 0;
}
