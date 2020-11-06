#include "Expression.hpp"

#include <string.h>
#include <map>

using namespace std;

void nextAnimation(map<int, string> &timing, string &mp3Path) {
    // init structure
    viseme_timing_t vt = (viseme_timing_t){PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, NULL};
    vt.timing_size = timing.size();
    vt.timing = (int *)calloc(vt.timing_size, sizeof(int));

    // copy each timing to the structure array
    int i = 0;
    for (auto t : timing) vt.timing[i++] = t.first;

    // start playing audio in a separate thread
    thread play_audio(ad_play_audio_file, ad_wait_ready(), mp3Path.c_str(), 0.7, &vt);

    // receive notifications from the audio thread based on the given timings
    while (vt.next_timing < vt.timing_size) {
        pthread_mutex_lock(&vt.lock);
        pthread_cond_wait(&vt.cond, &vt.lock);
        pthread_mutex_unlock(&vt.lock);

        if (vt.next_timing <= vt.timing_size) {
            // TODO put transition expression here
            cout << "timing: " << timing[vt.timing[vt.next_timing-1]] << endl;
        }
    }

    free(vt.timing);
    play_audio.join();
}

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY, true);

    //////////////
    map<int, string> t;
    t[0] = "60";t[1000] = "59";t[2000] = "58";t[3000] = "57";t[4000] = "56";t[5000] = "55";
    t[6000] = "54";t[7000] = "53";t[8000] = "52";t[9000] = "51";t[10000] = "50";
    t[11000] = "49";t[12000] = "48";t[13000] = "47";t[14000] = "46";t[15000] = "45";
    t[16000] = "44";t[17000] = "43";t[18000] = "42";t[19000] = "41";t[20000] = "40";
    t[25000] = "35";t[30000] = "30";t[35000] = "25";t[40000] = "20";t[45000] = "15";
    t[50000] = "10";t[55000] = "05";t[59000] = "01";t[60000] = "00";
    t[11500] = "49.5";t[12500] = "48.5";t[13500] = "47.5";

    string p = "../media/test2.mp3";
    //////////////

    while (1) {
        thread animate(nextAnimation, ref(t), ref(p));
        animate.detach();
        sleep(15);
    }

    while (1) sleep(5);
}
