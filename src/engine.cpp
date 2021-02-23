#include "Expression.hpp"
#include "json.hpp"

#include <string.h>
#include <map>

#include <ros/ros.h>
#include <rovy_msgs/AudioPlay.h>
#include <rovy_msgs/AudioDone.h>

using namespace std;
using json = nlohmann::json;

typedef shared_ptr<map<int, ExpressionIndex>> Marks;

#define MEDIA_PATH "/opt/face-engine/media/"

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

static ros::Publisher audioDonePub;

bool hasSuffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool exists(string path) {
    return (access(path.c_str(), F_OK) != -1);
}

void publishDoneMsg(bool status) {
    // publish audio done message
    rovy_msgs::AudioDone msg;
    msg.status = status;
    audioDonePub.publish(msg);
}

void nextAnimation(Marks marks, string soundPath) {
    string fullPath = MEDIA_PATH + soundPath;
    if (!exists(fullPath)) {
        cout << "Invalid path: " << fullPath << endl;
        publishDoneMsg(false);
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
        free(vt.timing);
        publishDoneMsg(false);
        return;
    }

    // receive notifications from the audio thread based on the given timings
    while (vt.next_timing < vt.timing_size) {
        pthread_mutex_lock(&vt.lock);

        struct timespec timeToWait;
        clock_gettime(CLOCK_REALTIME, &timeToWait);
        timeToWait.tv_sec += 3;
        int err = pthread_cond_timedwait(&vt.cond, &vt.lock, &timeToWait);
        // give up after 3 seconds and clean up
        if (err == ETIMEDOUT) {
            //cout<<"ETIMEDOUT: "<<this_thread::get_id()<<endl;
            manager.pauseBlink(false);
            free(vt.timing);
            play_audio.join();
            publishDoneMsg(false);
            return;
        }

        pthread_mutex_unlock(&vt.lock);

        int timing_index = -1;
        while (vt.next_timing <= vt.timing_size && timing_index != vt.next_timing) {
            timing_index = vt.next_timing;
            ExpressionIndex exp = (*marks)[vt.timing[timing_index-1]];
            //cout << "mark: " << exp << ", index: " << timing_index-1 << endl;
            manager.transition(exp);
        }
    }

    if (manager.getPausedBlinkCount() == 1) {
        ExpressionIndex exp = (*marks)[vt.timing[tCount-1]];
        //cout << "last mark: " << exp << ", index: " << tCount-1 << endl;
        manager.transition(exp);
        usleep(300000);
        manager.transition(HAPPY);
    }

    free(vt.timing);
    play_audio.join();

    manager.pauseBlink(false);

    publishDoneMsg(true);
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

void audioPlayCallback(const rovy_msgs::AudioPlay& msg) {
    try {
        if (!msg.audioPath.empty() && !msg.marksPath.empty()) {
            Marks marks = getSpeechMarks(msg.marksPath);
            string sound = msg.audioPath;

            thread animate(nextAnimation, marks, sound);
            animate.detach();
        } else {
            cout << "Empty audioPath and/or marksPath" << endl;
        }
    } catch (string &ex) {
        cout << ex << endl;
    }
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "rovy_face_engine");
    ExpressionManager &manager = ExpressionManager::getInstance();

    if (argc == 2 && strcmp(argv[1], "quiet") == 0) {
        manager.setQuiet(true);
    }

    manager.transition(HAPPY);

    ros::NodeHandle nh;
    ros::Subscriber audioPlaySub = nh.subscribe("rovy/action/audio_play", 1, audioPlayCallback);
    audioDonePub = nh.advertise<rovy_msgs::AudioDone>("rovy/action/audio_done", 1);
    ros::spin();
}
