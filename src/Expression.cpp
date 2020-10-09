#include "Expression.hpp"

Expression::Expression(string stillPath) {
    stillVid_ = readFile(ANIMATION_PATH + stillPath);
}

void Expression::addTransition(ExpressionIndex e, string p) {
    transition_[e] = readFile(ANIMATION_PATH + p);
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

ExpressionManager::ExpressionManager() : current_(HAPPY) {
    expressions_[HAPPY] = new HappyExpression();
    expressions_[CONFUSED] = new ConfusedExpression();

    if (dd_init() != 0) {
        printf("Error during video driver init!\n");
    }
}

ExpressionManager::~ExpressionManager() {
    dd_destroy();

    for (auto e : expressions_) {
        delete e.second;
    }
}
