#pragma once

#include <QRectF>
#include "rapidjson/document.h"

#include "itemconstants.h"
#include "rapidjson_util.h"

enum class ItemLocationType {
    STASH,
    CHARACTER
};

class ItemLocation {
public:
    ItemLocation();
    explicit ItemLocation(const rapidjson::Value &root);
    void ToItemJson(rapidjson::Value *root, rapidjson_allocator &alloc);
    void FromItemJson(const rapidjson::Value &root);
    std::string GetHeader() const;
    QRectF GetRect() const;
    std::string GetForumCode(const std::string &league) const;
    std::string GetGeneralHash() const;
    std::string GetUniqueHash() const;
    std::string GetLabel() const { return tab_label_; }
    bool operator<(const ItemLocation &other) const;
    ItemLocationType type() const { return type_; }
    void set_type(const ItemLocationType type) { type_ = type; }
    void set_character(const std::string &character) { character_ = character; }
    void set_tab_id(int tab_id) { tab_id_ = tab_id; }
    void set_tab_label(const std::string &tab_label) { tab_label_ = tab_label; }
    bool socketed() const { return socketed_; }
    void set_socketed(bool socketed) { socketed_ = socketed; }
private:
    ItemLocationType type_;
    int tab_id_;
    std::string tab_label_;
    int x_, y_, w_, h_;
    std::string character_;
    std::string inventory_id_;
    bool socketed_;
};
