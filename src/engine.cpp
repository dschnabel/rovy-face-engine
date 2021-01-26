#include "Expression.hpp"
#include "json.hpp"

#include <string.h>
#include <map>
#include <sys/inotify.h>

using namespace std;
using json = nlohmann::json;

typedef shared_ptr<map<int, ExpressionIndex>> Marks;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16 ))
#define ACTION_PATH "/tmp/action/"
#define ACTION_PREFIX "sound_"
#define MEDIA_PATH "/opt/face-engine/media/"

static int notifyFd;
static char notifyBuffer[EVENT_BUF_LEN];
static map<string, ExpressionIndex> visemeMapping =
    {
            {"sil", HAPPY},
            {"p", SPEAK_CLOSE},
            {"t", SPEAK_OPEN},
            {"S", SPEAK_CLOSE},
            {"T", SPEAK_CLOSE},
            {"f", SPEAK_CLOSE},
            {"k", SPEAK_OPEN},
            {"i", SPEAK_OPEN},
            {"r", SPEAK_CLOSE},
            {"s", SPEAK_OPEN},
            {"u", SPEAK_OPEN},
            {"@", SPEAK_OPEN},
            {"a", SPEAK_OPEN},
            {"e", SPEAK_OPEN},
            {"E", SPEAK_OPEN},
            {"o", SPEAK_OPEN},
            {"O", SPEAK_OPEN}
    };

bool hasSuffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool exists(string path) {
    return (access(path.c_str(), F_OK) != -1);
}

void nextAnimation(Marks marks, string soundPath) {
    string fullPath = MEDIA_PATH + soundPath;
    if (!exists(fullPath)) {
        cout << "Invalid path: " << fullPath << endl;
        return;
    }

    ExpressionManager &manager = ExpressionManager::getInstance();
    manager.pauseBlink(true);

    // init structure
    viseme_timing_t vt = (viseme_timing_t){PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, NULL};
    vt.timing_size = marks->size();
    vt.timing = (int *)calloc(vt.timing_size, sizeof(int));

    // copy each timing to the structure array
    int tCount = 0;
    for (auto t : *marks) vt.timing[tCount++] = t.first;

    // start playing audio in a separate thread
    thread play_audio;
    if (hasSuffix(fullPath, ".mp3")) {
        play_audio = thread(ad_play_mp3_file, ad_wait_ready(), fullPath.c_str(), 1.0, &vt);
    } else if (hasSuffix(fullPath, ".ogg")) {
        play_audio = thread(ad_play_ogg_file, ad_wait_ready(), fullPath.c_str(), 1.0, &vt);
    } else {
        cout << "Unknown file type: " << fullPath << endl;
        manager.pauseBlink(false);
        return;
    }

    // receive notifications from the audio thread based on the given timings
    while (vt.next_timing < vt.timing_size) {
        pthread_mutex_lock(&vt.lock);
        pthread_cond_wait(&vt.cond, &vt.lock);
        pthread_mutex_unlock(&vt.lock);

        int timing_index = -1;
        while (vt.next_timing <= vt.timing_size && timing_index != vt.next_timing) {
            timing_index = vt.next_timing;
            ExpressionIndex exp = (*marks)[vt.timing[timing_index-1]];
            cout << "mark: " << exp << ", index: " << timing_index-1 << endl;
            manager.transition(exp);
        }
    }

    if (manager.getPausedBlinkCount() == 1) {
        ExpressionIndex exp = (*marks)[vt.timing[tCount-1]];
        cout << "last mark: " << exp << ", index: " << tCount-1 << endl;
        manager.transition(exp);
        usleep(300000);
        manager.transition(HAPPY);
    }

    free(vt.timing);
    play_audio.join();

    manager.pauseBlink(false);
}

Marks getSpeechMarks(string marksPath) {
    string fullPath = MEDIA_PATH + marksPath;
    if (!exists(fullPath)) {
        throw string("Invalid path: ") + fullPath;
    }

    ifstream input(fullPath);
    string str;

    json speechMarks;
    while(getline(input, str)) {
        speechMarks["marks"].push_back(json::parse(str));
    }

    int prevTime = 0;
    map<int, ExpressionIndex> marks;
    for (json::iterator it = speechMarks["marks"].begin(); it != speechMarks["marks"].end(); ++it) {
        int time = (*it)["time"];
        string value = (*it)["value"];

        // set viseme
        ExpressionIndex viseme = visemeMapping[value];
        if (viseme == UNDEF) {
            if (marks[prevTime] != UNDEF) {
                viseme = marks[prevTime];
            } else {
                viseme = HAPPY;
            }
        }

        marks[time] = viseme;

        prevTime = time;
    }

    return make_shared<map<int, ExpressionIndex>>(marks);
}

json getNextAction() {
    // blocking read
    int length = read(notifyFd, notifyBuffer, EVENT_BUF_LEN);
    if (length < 0) return "";

    string filename;
    int i = 0;
    while (i < length) {
        struct inotify_event *event = (struct inotify_event *)&notifyBuffer[i];
        if (event->len) {
            if ((event->mask & IN_CREATE) && (strncmp(event->name, ACTION_PREFIX, strlen(ACTION_PREFIX)) == 0)) {
                unlink(string(ACTION_PATH + filename).c_str());
                filename = event->name;
            }
        }
        i += EVENT_SIZE + event->len;
    }

    if (!filename.empty()) {
        ifstream input(string(ACTION_PATH) + filename);
        json action;

        try {
            input >> action;
        } catch (json::exception &ex) {
            cout << ex.what() << endl;
        }

        input.close();
        unlink(string(ACTION_PATH + filename).c_str());

        return action;
    }

    return json();
}

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY);

    notifyFd = inotify_init();
    if ( notifyFd < 0 ) {
      perror("inotify_init");
    }
    int watch = inotify_add_watch(notifyFd, ACTION_PATH, IN_CREATE);

    while (1) {
        try {
            // expected action format:
            // echo '{"marks":"audio/hello-rovy.marks", "sound":"audio/hello-rovy.mp3"}' > /tmp/action/sound_a
            json action = getNextAction();
            if (action.empty()) continue;

            if (action.contains("marks") && action.contains("sound")) {
                Marks marks = getSpeechMarks(action["marks"]);
                string sound = action["sound"];

                thread animate(nextAnimation, marks, sound);
                animate.detach();
            } else {
                cout << "Invalid or incomplete action file" << endl;
            }
        } catch (string &ex) {
            cout << ex << endl;
        }
    }

    inotify_rm_watch(notifyFd, watch);
    close(notifyFd);
}
