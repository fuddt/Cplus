//
// Created by shirotabi on 2026/03/05.
//

#include "Herb.h"

#include "Player.h"

void Herb::use(Player& player) {
    player.heal(healAmount_);
}
