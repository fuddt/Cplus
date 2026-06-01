//
// Created by shirotabi on 2026/04/19.
//

#ifndef CPLUS_APPCONTEXT_H
#define CPLUS_APPCONTEXT_H
#include <string>


struct Course {
    std::string place;
    int distance;
};

struct Jockey {
    int name;
    int age;
    float weight;
    float win_rate;
    struct Equipment {
        std::string whipType;
        std::string blinkerType;
    }equipment;
};

struct Horse {
    int gateNumber = 0;
    int speedRating = 0;
};

struct Context {
    Course cource;
    Jockey jockey;
    Horse horse;
};


#endif //CPLUS_APPCONTEXT_H