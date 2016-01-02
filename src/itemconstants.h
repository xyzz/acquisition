#pragma once

#include <map>

const int PIXELS_PER_SLOT = 47;
const int INVENTORY_SLOTS = 12;

enum FRAME_TYPES {
    FRAME_TYPE_NORMAL = 0,
    FRAME_TYPE_MAGIC = 1,
    FRAME_TYPE_RARE = 2,
    FRAME_TYPE_UNIQUE = 3,
    FRAME_TYPE_GEM = 4,
    FRAME_TYPE_CURRENCY = 5,
};

enum ELEMENTAL_DAMAGE_TYPES {
    ED_FIRE = 4,
    ED_COLD = 5,
    ED_LIGHTNING = 6,
};

const int LINKH_HEIGHT = 16;
const int LINKH_WIDTH = 38;
const int LINKV_HEIGHT = LINKH_WIDTH;
const int LINKV_WIDTH = LINKH_HEIGHT;

const int PIXELS_PER_MINIMAP_SLOT = 10;
const int MINIMAP_SIZE = INVENTORY_SLOTS * PIXELS_PER_MINIMAP_SLOT;

struct position {
    double x;
    double y;
};

std::map<std::string, position> const POS_MAP{
    {"MainInventory", {0, 7}}, {"BodyArmour", {5, 2}}, {"Weapon", {2, 0}}, {"Weapon2", {2, 0}}, {"Offhand", {8, 0}},
    {"Offhand2", {8, 0}}, {"Boots", {7, 4}}, {"Ring", {4, 3}}, {"Ring2", {7, 3}}, {"Amulet", {7, 2}},
    {"Gloves", {3, 4}}, {"Belt", {5, 5}}, {"Helm", {5, 0}}, {"Flask", {3.5, 6}}
};
