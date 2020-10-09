#ifndef SRC_EXPRESSION_HPP_
#define SRC_EXPRESSION_HPP_

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <map>

#include "display.h"

using namespace std;

#define ANIMATION_PATH "../animation/"

enum ExpressionIndex {
    HAPPY,
    CONFUSED,
};

class Expression {
public:
    Expression(string stillPath);
    void addTransition(ExpressionIndex e, string p);
    bool transition(ExpressionIndex e, bool stay);
    bool still();
private:
    string readFile(string path);
    bool play_video(string &buffer);

    string stillVid_;
    map<ExpressionIndex, string> transition_;
};

class HappyExpression: public Expression {
public:
    HappyExpression() : Expression("happy.h264") {
        addTransition(CONFUSED, "confused-happy.h264");
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
private:
    ExpressionManager();
    ~ExpressionManager();

    map<ExpressionIndex, Expression*> expressions_;
    ExpressionIndex current_;
};

#endif /* SRC_EXPRESSION_HPP_ */
