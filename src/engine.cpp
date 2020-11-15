#include "Expression.hpp"
#include "json.hpp"

#include <string.h>
#include <map>

using namespace std;
using json = nlohmann::json;

void nextAnimation(map<int, string> &marks, string &mp3Path) {
    ExpressionManager &manager = ExpressionManager::getInstance();
    manager.pauseBlink(true);

    // init structure
    viseme_timing_t vt = (viseme_timing_t){PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, NULL};
    vt.timing_size = marks.size();
    vt.timing = (int *)calloc(vt.timing_size, sizeof(int));

    // copy each timing to the structure array
    int i = 0;
    for (auto t : marks) vt.timing[i++] = t.first;

    // start playing audio in a separate thread
    thread play_audio(ad_play_mp3_file, ad_wait_ready(), mp3Path.c_str(), 1.0, &vt);

    // receive notifications from the audio thread based on the given timings
    while (vt.next_timing < vt.timing_size) {
        pthread_mutex_lock(&vt.lock);
        pthread_cond_wait(&vt.cond, &vt.lock);
        pthread_mutex_unlock(&vt.lock);

        if (vt.next_timing <= vt.timing_size) {
            // TODO put transition expression here
            cout << "timing: " << marks[vt.timing[vt.next_timing-1]] << endl;
        }
    }

    free(vt.timing);
    play_audio.join();

    manager.pauseBlink(false);
}

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY, true);

    ifstream i("../media/speech.marks");
    string str;

    json speechMarks;
    while(getline(i, str)) {
        speechMarks["marks"].push_back(json::parse(str));
    }

    map<int, string> marks;
    for (json::iterator it = speechMarks["marks"].begin(); it != speechMarks["marks"].end(); ++it) {
      marks[(*it)["time"]] = (*it)["value"];
    }

    string speech = "../media/hello-rovy.mp3";
    thread animate(nextAnimation, ref(marks), ref(speech));
//    animate.detach();
    animate.join();

    ad_play_ogg_file_pitched(0, "../media/poem.ogg", 1.0, NULL);

    while (1) sleep(5);
}
