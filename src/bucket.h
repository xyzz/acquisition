#pragma once

#include <string>

#include "item.h"

// A bucket holds set of filtered items.
// Items are "bucketed" by their location: stash tab / character.
class Bucket {
public:
    Bucket();
    explicit Bucket(const ItemLocation &location);
    void AddItem(const std::shared_ptr<Item> item);
    const Items &items() const { return items_; }
    const ItemLocation &location() const { return location_; }
private:
    Items items_;
    ItemLocation location_;
};
