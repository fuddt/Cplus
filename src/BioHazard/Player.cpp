#include "Player.h"

#include <utility>

void Player::heal(int amount) {
    hp += amount;
    if (hp > maxHp) {
        hp = maxHp;
    }
    updateCondition();
}

void Player::damage(int amount) {
    hp -= amount;
    if (hp < 0) {
        hp = 0;
    }
    updateCondition();
}

void Player::updateCondition() {
    float ratio = static_cast<float>(hp) / maxHp;

    if (ratio > 0.67f) {
        condition = Condition::Fine;
    } else if (ratio > 0.33f) {
        condition = Condition::Caution;
    } else {
        condition = Condition::Danger;
    }
}

int       Player::getHp()        const { return hp; }
int       Player::getMaxHp()     const { return maxHp; }
Condition Player::getCondition() const { return condition; }

void Player::addItem(std::unique_ptr<Item> item) {
    inventory.push_back(std::move(item));
}

void Player::useItem(int index) {
    if (index < 0 || index >= static_cast<int>(inventory.size())) {
        return;
    }

    inventory[index]->use(*this);
    inventory.erase(inventory.begin() + index);
}

int Player::itemCount() const {
    return static_cast<int>(inventory.size());
}
