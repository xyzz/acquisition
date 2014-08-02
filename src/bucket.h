#pragma once

#include <string>

#include "item.h"

// A bucket holds set of filtered items.
// Items are "bucketed" by their location: stash tab / character.
class Bucket {
public:
    Bucket();
    explicit Bucket(const std::string &name);
    void AddItem(const std::shared_ptr<Item> item);
    const std::string &name() const { return name_; }
    const Items &items() const { return items_; }
private:
    std::string name_;
    Items items_;
};
