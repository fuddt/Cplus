//
// Created by shirotabi on 2026/04/19.
//

#ifndef CPLUS_BAIOHAZARD_H
#define CPLUS_BAIOHAZARD_H
#include "Player.h"

#endif //CPLUS_BAIOHAZARD_H

class BioHazard {
public:
    static std::string conditionName(Condition c);
    static void printStatus(const Player& p);
    static void execution();
};