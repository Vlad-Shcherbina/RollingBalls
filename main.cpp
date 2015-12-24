#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <thread>

using namespace std;

#define LOCAL
#define DRAW_BOARDS

// 'cause topcoder requires single file submission
#include "solution.cpp"


int main(int argc, char **argv) {
    vector<string> args(argv + 1, argv + argc);
    for (auto arg : args) {
        int pos = arg.find('=');
        assert(pos != string::npos);
        auto key = arg.substr(0, pos);
        auto value = stoi(arg.substr(pos + 1));
        assert(knobs.count(key) > 0);
        assert(custom_knobs.count(key) == 0);
        custom_knobs[key] = value;
    }

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
