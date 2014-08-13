#pragma once

#include <QRectF>
#include "jsoncpp/json.h"

#include "itemconstants.h"

enum class ItemLocationType {
    STASH,
    CHARACTER
};

class ItemLocation {
public:
    ItemLocation();
    explicit ItemLocation(const Json::Value &root);
    void ToItemJson(Json::Value *root);
    void FromItemJson(const Json::Value &root);
    std::string GetHeader() const;
    QRectF GetRect() const;
    std::string GetForumCode(const std::string &league) const;
    std::string GetUniqueHash() const;
    bool operator<(const ItemLocation &other) const;
    void set_type(const ItemLocationType type) { type_ = type; }
    void set_character(const std::string &character) { character_ = character; }
    void set_tab_id(int tab_id) { tab_id_ = tab_id; }
    void set_tab_label(const std::string &tab_label) { tab_label_ = tab_label; }
private:
    ItemLocationType type_;
    int tab_id_;
    std::string tab_label_;
    int x_, y_, w_, h_;
    std::string character_;
    std::string inventory_id_;
};
