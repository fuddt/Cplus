#pragma once
#include <memory>
#include <vector>

#include "Item/Item.h"

enum class Condition {
    Fine,    // HP 67% 超
    Caution, // HP 34〜67%
    Danger,  // HP 33% 以下
};

class Player {
public:
    void heal(int amount);
    void damage(int amount);
    void updateCondition();
    void addItem(std::unique_ptr<Item> item);
    void useItem(int index);
    int itemCount() const;
    int       getHp()        const;
    int       getMaxHp()     const;
    Condition getCondition() const;

private:
    int       hp        = 100;
    int       maxHp     = 100;
    Condition condition = Condition::Fine;
    std::vector<std::unique_ptr<Item>> inventory;
};
