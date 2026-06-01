//
// Created by shirotabi on 2026/03/05.
//

#include "Item.h"

#include <iostream>

void Item::showInfo() const {
    std::cout << name_ << " - " << description_ << std::endl;
}

const std::string& Item::name() const {
    return name_;
}

const std::string& Item::description() const {
    return description_;
}
