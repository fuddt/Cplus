//
// Created by shirotabi on 2026/03/05.
//

#ifndef CPLUS_HERB_H
#define CPLUS_HERB_H
#include <string>
#include <utility>

#include "Item/Item.h"


class Herb: public Item
{
public:
    Herb(std::string name, std::string description, int healAmount): Item(std::move(name),std::move(description)), healAmount_(healAmount) {}
    void use(Player& player) override;
private:
    int healAmount_;
};

class GreenHerb: public Herb {
    public:
        GreenHerb(): Herb("Green Herb", "少量の体力を回復",  30) {}

};

class RedHerb: public Herb {
    public:
        RedHerb(): Herb("Red Herb", "中程度の体力を回復",  60) {}
};




#endif //CPLUS_HERB_H
