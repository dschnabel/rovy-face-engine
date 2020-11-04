#include "Expression.hpp"

#include <string.h>

using namespace std;

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY, true);

    while (1) sleep(5);
}
