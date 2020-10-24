#include "Expression.hpp"

using namespace std;

int main() {
    ExpressionManager &manager = ExpressionManager::getInstance();
    manager.transition(HAPPY, true);

    while (1) sleep(5);
}
