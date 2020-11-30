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
    A,
    I,
    O,
    P,
};

class Expression {
public:
    Expression(string stillPath, string blinkPath = "");
    void addTransition(ExpressionIndex e, string p);
    bool transition(ExpressionIndex e, bool stay);
    bool still();
    bool blink();
    bool hasBlink();

    static string readFile(string path);
private:
    bool play_video(string &buffer);

    string stillVid_, blinkVid_;
    map<ExpressionIndex, string> transition_;
};

class HappyExpression: public Expression {
public:
    HappyExpression() : Expression("happy.h264", "happy-blink.h264") {
        addTransition(A, "a-happy.h264");
        addTransition(I, "i-happy.h264");
        addTransition(O, "o-happy.h264");
        addTransition(P, "p-happy.h264");
    }
};

class AExpression: public Expression {
public:
    AExpression() : Expression("a.h264") {
        addTransition(HAPPY, "happy-a.h264");
        addTransition(I, "i-a.h264");
        addTransition(O, "o-a.h264");
        addTransition(P, "p-a.h264");
    }
};

class IExpression: public Expression {
public:
    IExpression() : Expression("i.h264") {
        addTransition(HAPPY, "happy-i.h264");
        addTransition(A, "a-i.h264");
        addTransition(O, "o-i.h264");
        addTransition(P, "p-i.h264");
    }
};

class OExpression: public Expression {
public:
    OExpression() : Expression("o.h264") {
        addTransition(HAPPY, "happy-o.h264");
        addTransition(A, "a-o.h264");
        addTransition(I, "i-o.h264");
        addTransition(P, "p-o.h264");
    }
};

class PExpression: public Expression {
public:
    PExpression() : Expression("p.h264") {
        addTransition(HAPPY, "happy-p.h264");
        addTransition(A, "a-p.h264");
        addTransition(I, "i-p.h264");
        addTransition(O, "o-p.h264");
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
    void pauseBlink(bool pause);
    int getPausedBlinkCount();
private:
    ExpressionManager();
    ~ExpressionManager();
    void blinkerThread();

    map<ExpressionIndex, Expression*> expressions_;
    ExpressionIndex current_;
    mutex blinkerMutex_;
    string blinkAudio_;
    bool quiet_;
    int pauseBlink_;
};

#endif /* SRC_EXPRESSION_HPP_ */
