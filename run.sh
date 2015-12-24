set -e -x

RELEASE=true

# asan makes it painful to get coredumps
MAYBE_ASAN=""
#MAYBE_ASAN=",address"

if [ "$RELEASE" = true ]; then
    g++ \
        --std=c++0x -W -Wall -Wno-sign-compare -Wno-unused \
        -O2 -pipe -mmmx -msse -msse2 -msse3 \
        -ggdb \
        -DNDEBUG \
        main.cpp -o main
else
    clang++ \
        --std=c++0x -W -Wall -Wno-sign-compare \
        -O2 -pipe -mmmx -msse -msse2 -msse3 \
        -ggdb \
        -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
        -fsanitize=integer,undefined"$MAYBE_ASAN" \
        -fno-sanitize-recover \
        main.cpp -o main
fi

if [ $# -eq 0 ]; then
    seed=1
else
    seed="$1"
    shift
fi

java -jar tester/tester.jar \
    -exec "./driver.sh $*" -seed "$seed" -novis
