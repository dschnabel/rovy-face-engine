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
    HAPPY,
    CONFUSED,
};

class Expression {
public:
    Expression(string stillPath, string blinkPath = "");
    void addTransition(ExpressionIndex e, string p);
    bool transition(ExpressionIndex e, bool stay);
    bool still();
    bool blink();

    static string readFile(string path);
private:
    bool play_video(string &buffer);

    string stillVid_, blinkVid_;
    map<ExpressionIndex, string> transition_;
};

class HappyExpression: public Expression {
public:
    HappyExpression() : Expression("happy.h264", "happy-blink.h264") {
//        addTransition(CONFUSED, "confused-happy.h264");
    }
};

class ConfusedExpression: public Expression {
public:
    ConfusedExpression() : Expression("confused.h264") {
        addTransition(HAPPY, "happy-confused.h264");
    }
};

class ExpressionManager {
public:
    static ExpressionManager& getInstance() {
        static ExpressionManager instance;
        return instance;
    }
    ExpressionManager(ExpressionManager const&) = delete;
    void operator=(ExpressionManager const&) = delete;

    bool transition(ExpressionIndex e, bool stay = false);
    void setQuiet(bool quiet);
private:
    ExpressionManager();
    ~ExpressionManager();
    void blinkerThread();

    map<ExpressionIndex, Expression*> expressions_;
    ExpressionIndex current_;
    mutex blinkerMutex_;
    string blinkAudio_;
    bool quiet_;
};

#endif /* SRC_EXPRESSION_HPP_ */
