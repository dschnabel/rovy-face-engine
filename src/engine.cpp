#include "Expression.hpp"
#include "json.hpp"

#include <string.h>
#include <map>
#include <sys/inotify.h>

using namespace std;
using json = nlohmann::json;

typedef shared_ptr<map<int, string>> Marks;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16 ))
#define ACTION_PATH "/tmp/action/"
#define MEDIA_PATH "/opt/face-engine/media/"

static int notifyFd;
static char notifyBuffer[EVENT_BUF_LEN];

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
    int i = 0;
    for (auto t : *marks) vt.timing[i++] = t.first;

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

    map<int, string> marks;
    for (json::iterator it = speechMarks["marks"].begin(); it != speechMarks["marks"].end(); ++it) {
        marks[(*it)["time"]] = (*it)["value"];
    }

    return make_shared<map<int, string>>(marks);
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
            if (event->mask & IN_CREATE) {
                filename = event->name;
            }
        }
        i += EVENT_SIZE + event->len;
    }

    if (!filename.empty()) {
        ifstream input(string(ACTION_PATH) + filename);
        json action;
        input >> action;
        return action;
    }

    return json();
}

int main(int argc, char **argv) {
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY, true);

    notifyFd = inotify_init();
    if ( notifyFd < 0 ) {
      perror("inotify_init");
    }
    int watch = inotify_add_watch(notifyFd, ACTION_PATH, IN_CREATE);

    while (1) {
        try {
            // expected action format:
            // {"marks":"olympics.marks", "sound":"olympics.ogg"}
            json action = getNextAction();
            if (!action.empty() && action.contains("marks") && action.contains("sound")) {
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
