//
// Created by shirotabi on 2026/03/05.
//

#ifndef CPLUS_ITEM_H
#define CPLUS_ITEM_H
#include <string>
#include <utility>

class Player;

class Item {

public:
    Item(std::string name, std::string description): name_(std::move(name)), description_(std::move(description)) {}
    virtual void use(Player &player) = 0;
    void showInfo() const;
    virtual ~Item() = default;

    const std::string& name() const;
    const std::string& description() const;

private:
    std::string name_;
    std::string description_;
};


#endif //CPLUS_ITEM_H
