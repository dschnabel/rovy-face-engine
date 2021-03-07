#include "Expression.hpp"

#include <random>

Expression::Expression(string stillPath, string blinkPath) {
    stillVid_ = readFile(EXPRESSIOS_PATH + stillPath);
    blinkVid_ = readFile(EXPRESSIOS_PATH + blinkPath);
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

        return still();
    }

    return true;
}

bool Expression::hasBlink() {
    return !blinkVid_.empty();
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

bool ExpressionManager::transition(ExpressionIndex e) {
    lock_guard<std::mutex> lock(transitionMutex_);

    if (!isHappy_) {
        switch (e) {
        case HAPPY: e = FROWN; break;
        case SPEAK_OPEN: e = FROWN_SPEAK_OPEN; break;
        case SPEAK_CLOSE: e = FROWN_SPEAK_CLOSE; break;
        }
    }

    current_ = e;

    bool ret = expressions_[e]->still();

	// in case of SPEAK_OPEN we need to close lips (SPEAK_CLOSE) again afterward
    if (e == SPEAK_OPEN ||e == FROWN_SPEAK_OPEN) {
        // add some randomized timing to make lip sync appear less predictable
        static random_device rd;
        static mt19937 rng(rd());
        static uniform_int_distribution<int> uni(70000, 90000);

        usleep(uni(rng));
        expressions_[e == SPEAK_OPEN ? SPEAK_CLOSE : FROWN_SPEAK_CLOSE]->still();
        usleep(uni(rng) - 40000);
    }

    return ret;
}

void ExpressionManager::setQuiet(bool quiet) {
    quiet_ = quiet;
}

void ExpressionManager::pauseBlink(bool pause) {
    lock_guard<std::mutex> lock(blinkerMutex_);

    if (pause) pauseBlink_++;
    else pauseBlink_--;
}

int ExpressionManager::getPausedBlinkCount() {
    return pauseBlink_;
}

void ExpressionManager::changeMood(bool isHappy) {
    isHappy_ = isHappy;
    transition(HAPPY);
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

            if (pauseBlink_ == 0) {
                lock_guard<std::mutex> lock2(transitionMutex_);

                if (!quiet_ && expressions_[current_]->hasBlink()) {
                    thread play_audio(ad_play_mp3_buffer, ad_wait_ready(), blinkAudio, blinkAudioLen, 0.2, nullptr);
                    play_audio.detach();
                }

                expressions_[current_]->blink();
            }
        }
    }
}

ExpressionManager::ExpressionManager() : current_(HAPPY), quiet_(false), pauseBlink_(0), isHappy_(true) {
    expressions_[HAPPY] = new HappyExpression();
    expressions_[SPEAK_OPEN] = new SpeakOpenExpression();
    expressions_[SPEAK_CLOSE] = new SpeakCloseExpression();
    expressions_[FROWN] = new FrownExpression();
    expressions_[FROWN_SPEAK_OPEN] = new FrownSpeakOpenExpression();
    expressions_[FROWN_SPEAK_CLOSE] = new FrownSpeakCloseExpression();

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
