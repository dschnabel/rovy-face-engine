#ifndef SRC_EXPRESSION_HPP_
#define SRC_EXPRESSION_HPP_

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <mutex>

#include "display.h"
#include "audio.h"

using namespace std;

#define EXPRESSIOS_PATH "../media/expressions/"
#define AUDIO_PATH "../media/audio/"

enum ExpressionIndex {
    UNDEF,
    HAPPY,
    SPEAK_OPEN,
    SPEAK_CLOSE,
};

class Expression {
public:
    Expression(string stillPath, string blinkPath = "");
    bool still();
    bool blink();
    bool hasBlink();

    static string readFile(string path);
private:
    bool play_video(string &buffer);

    string stillVid_, blinkVid_;
};

class HappyExpression: public Expression {
public:
    HappyExpression() : Expression("happy.h264", "happy-blink.h264") {}
};

class SpeakOpenExpression: public Expression {
public:
    SpeakOpenExpression() : Expression("speak-open.h264") {}
};

class SpeakCloseExpression: public Expression {
public:
    SpeakCloseExpression() : Expression("speak-close.h264") {}
};

class ExpressionManager {
public:
    static ExpressionManager& getInstance() {
        static ExpressionManager instance;
        return instance;
    }
    ExpressionManager(ExpressionManager const&) = delete;
    void operator=(ExpressionManager const&) = delete;

    bool transition(ExpressionIndex e);
    void setQuiet(bool quiet);
    void pauseBlink(bool pause);
    int getPausedBlinkCount();
private:
    ExpressionManager();
    ~ExpressionManager();
    void blinkerThread();

    map<ExpressionIndex, Expression*> expressions_;
    ExpressionIndex current_;
    mutex blinkerMutex_, transitionMutex_;
    string blinkAudio_;
    bool quiet_;
    int pauseBlink_;
};

#endif /* SRC_EXPRESSION_HPP_ */
