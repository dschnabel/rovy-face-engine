#include "Expression.hpp"
#include "json.hpp"

#include <string.h>
#include <map>

using namespace std;
using json = nlohmann::json;

typedef shared_ptr<map<int, string>> Marks;

bool hasSuffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void nextAnimation(Marks &marks, string &soundPath) {
    ExpressionManager &manager = ExpressionManager::getInstance();
    manager.pauseBlink(true);

    // init structure
    viseme_timing_t vt = (viseme_timing_t){PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, NULL};
    vt.timing_size = marks->size();
    vt.timing = (int *)calloc(vt.timing_size, sizeof(int));

    // copy each timing to the structure array
    int i = 0;
    for (auto t : *marks) vt.timing[i++] = t.first;

    // start playing audio in a separate thread
    thread play_audio;
    if (hasSuffix(soundPath, ".mp3")) {
        play_audio = thread(ad_play_mp3_file, ad_wait_ready(), soundPath.c_str(), 1.0, &vt);
    } else if (hasSuffix(soundPath, ".ogg")) {
        play_audio = thread(ad_play_ogg_file, ad_wait_ready(), soundPath.c_str(), 1.0, &vt);
    } else {
        cout << "Unknown file type: " << soundPath << endl;
        manager.pauseBlink(false);
        return;
    }

    // receive notifications from the audio thread based on the given timings
    while (vt.next_timing < vt.timing_size) {
        pthread_mutex_lock(&vt.lock);
        pthread_cond_wait(&vt.cond, &vt.lock);
        pthread_mutex_unlock(&vt.lock);

        if (vt.next_timing <= vt.timing_size) {
            // TODO put transition expression here
            cout << "mark: " << (*marks)[vt.timing[vt.next_timing-1]] << endl;
        }
    }

    if (manager.getPausedBlinkCount() == 1) {
        // TODO put transition expression here
        cout << "last mark: " << (*marks)[vt.timing[vt.timing_size-1]] << endl;
    }

    free(vt.timing);
    play_audio.join();

    manager.pauseBlink(false);
}

Marks getSpeechMarks(string marksPath) {
    ifstream i(marksPath);
    string str;

    json speechMarks;
    while(getline(i, str)) {
        speechMarks["marks"].push_back(json::parse(str));
    }

    map<int, string> marks;
    for (json::iterator it = speechMarks["marks"].begin(); it != speechMarks["marks"].end(); ++it) {
        marks[(*it)["time"]] = (*it)["value"];
    }

    return make_shared<map<int, string>>(marks);
}

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY, true);

    Marks marks = getSpeechMarks("../media/olympics.marks");
    while (1) {
        string speech = "../media/olympics.ogg";
        thread animate(nextAnimation, ref(marks), ref(speech));
        animate.detach();
        sleep(7);cout<<endl;

        speech = "../media/poem.ogg";
        thread animate1(nextAnimation, ref(marks), ref(speech));
        animate1.detach();
        sleep(4);cout<<endl;

        speech = "../media/hello-rovy.mp3";
        thread animate2(nextAnimation, ref(marks), ref(speech));
        animate2.detach();
        sleep(2);cout<<endl;

        sleep(15);
    }

    while (1) sleep(5);
}
