#include <iostream>
#include <memory>
#include <string>
#include "Player.h"
#include "Item/Herb/Herb.h"
#include "Item/Key/Key.h"


std::string conditionName(Condition c) {
    switch (c) {
        case Condition::Fine:   return "Fine";
        case Condition::Caution: return "Caution";
        case Condition::Danger: return "Danger";
    }
    return "Unknown";
}

void printStatus(const Player& p) {
    std::cout << "HP: " << p.getHp() << "/" << p.getMaxHp()
              << "  [" << conditionName(p.getCondition()) << "]"
              << "  Items: " << p.itemCount()
              << std::endl;
}

int main() {
    Player player;

    player.addItem(std::make_unique<GreenHerb>());
    player.addItem(std::make_unique<RedHerb>());
    player.addItem(std::make_unique<Key>("ボスルームの鍵"));

    player.damage(80);
    printStatus(player);

    player.useItem(0);
    printStatus(player);

    player.useItem(0);
    printStatus(player);

    player.useItem(0);
    printStatus(player);

    return 0;
}
