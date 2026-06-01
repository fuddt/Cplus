#pragma once
#include <string>

#include "Item/Item.h"

class Key : public Item {
public:
    explicit Key(std::string keyId);
    void use(Player& player) override;

private:
    std::string keyId_;
};

