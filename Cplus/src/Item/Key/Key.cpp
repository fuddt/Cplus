#include "Key.h"

#include <iostream>
#include <utility>

Key::Key(std::string keyId)
    : Item("Key", "扉を開ける鍵"), keyId_(std::move(keyId)) {}

void Key::use(Player& /*player*/) {
    std::cout << "[" << keyId_ << "] を使った。扉が開いた。" << std::endl;
}

