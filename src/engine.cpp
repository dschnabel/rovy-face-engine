#include "Expression.hpp"

int main() {
    ExpressionManager &manager = ExpressionManager::getInstance();
    manager.transition(CONFUSED);
    manager.transition(HAPPY);
    manager.transition(CONFUSED);
    manager.transition(HAPPY);
    manager.transition(CONFUSED);
    manager.transition(HAPPY);
    manager.transition(CONFUSED);
    manager.transition(HAPPY, true);
    sleep(5);
}
