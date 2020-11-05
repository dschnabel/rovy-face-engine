#include "Expression.hpp"

#include <string.h>

using namespace std;

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY, true);

//    while (1) {
//        printf("before\n");
//        ad_play_audio_file(ad_wait_ready(), "../media/test.mp3", 1.0);
//        printf("after\n");
//        usleep(3000);
//    }

    while (1) sleep(5);
}
