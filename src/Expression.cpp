#include "Expression.hpp"

#include <random>

Expression::Expression(string stillPath, string blinkPath) {
    stillVid_ = readFile(EXPRESSIOS_PATH + stillPath);
    blinkVid_ = readFile(EXPRESSIOS_PATH + blinkPath);
}

void Expression::addTransition(ExpressionIndex e, string p) {
    transition_[e] = readFile(EXPRESSIOS_PATH + p);
}

bool Expression::transition(ExpressionIndex e, bool stay) {
    if (transition_.find(e) == transition_.end()) {
        cout << "Could not find transition " << e << endl;
        return false;
    }

    if (transition_[e].empty()) {
        cout << "Empty transition video " << e << endl;
        return false;
    }
    if (!play_video(transition_[e])) {
        cout << "Error during transition video " << e << endl;
        return false;
    }

    if (stay) {
        return still();
    } else {
        return true;
    }
}

bool Expression::still() {
    if (stillVid_.empty()) {
        cout << "Empty still video" << endl;
        return false;
    }
    if (!play_video(stillVid_)) {
        cout << "Error during still video" << endl;
        return false;
    }

    return true;
}

bool Expression::blink() {
    if (!blinkVid_.empty()) {
        if (!play_video(blinkVid_)) {
            cout << "Error during blink video" << endl;
            return false;
        }
    }

    return still();
}

string Expression::readFile(string path) {
    ifstream fin(path, ios::binary);
    if (!fin.good()) {
        cout << "Invalid path " << path << endl;
        return "";
    }

    ostringstream ostrm;
    ostrm << fin.rdbuf();
    return ostrm.str();
}

bool Expression::play_video(string &buffer) {
    if (dd_play_video((unsigned char *)buffer.c_str(), buffer.length()) != 0) {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////

bool ExpressionManager::transition(ExpressionIndex e, bool stay) {
    lock_guard<std::mutex> lock(blinkerMutex_);

    if (e != current_) {
        if (expressions_.find(current_) == expressions_.end()) {
            cout << "Could not find expression " << current_ << endl;
            return false;
        }

        bool ret = expressions_[e]->transition(current_, stay);
        current_ = e;
        return ret;
    } else {
        return expressions_[e]->still();
    }
}

void ExpressionManager::setQuiet(bool quiet) {
    quiet_ = quiet;
}

void ExpressionManager::blinkerThread() {
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> uni(2, 10);
    const char *blinkAudio = blinkAudio_.c_str();
    uint blinkAudioLen = blinkAudio_.length();

    while (1) {
        sleep(uni(rng));

        {
            lock_guard<std::mutex> lock(blinkerMutex_);

            if (!quiet_) {
                int id = ad_wait_ready();
                thread play_audio(ad_play_audio_buffer, id, blinkAudio, blinkAudioLen, 0.7);
                play_audio.detach();
            }

            expressions_[current_]->blink();
        }
    }
}

ExpressionManager::ExpressionManager() : current_(HAPPY), quiet_(false) {
    expressions_[HAPPY] = new HappyExpression();
//    expressions_[CONFUSED] = new ConfusedExpression();

    if (dd_init() != 0) {
        cout << "Error during video driver init!\n" << endl;
    }
    ad_init();

    blinkAudio_ = Expression::readFile(AUDIO_PATH + string("blink.mp3"));

    thread *t = new thread(&ExpressionManager::blinkerThread, this);
    t->detach();
}

ExpressionManager::~ExpressionManager() {
    ad_destroy();
    dd_destroy();

    for (auto e : expressions_) {
        delete e.second;
    }
}
