#pragma once

#include <QRectF>
#include <QString>
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
    QString GetHeader() const;
    QRectF GetRect() const;
    QString GetForumCode(const QString &league) const;
    QString GetUniqueHash() const;
    bool operator<(const ItemLocation &other) const;
    void set_type(const ItemLocationType type) { type_ = type; }
    void set_character(const QString &character) { character_ = character; }
    void set_tab_id(int tab_id) { tab_id_ = tab_id; }
    void set_tab_label(const QString &tab_label) { tab_label_ = tab_label; }
    bool socketed() const { return socketed_; }
    void set_socketed(bool socketed) { socketed_ = socketed; }
private:
    ItemLocationType type_;
    int tab_id_;
    QString tab_label_;
    int x_, y_, w_, h_;
    QString character_;
    QString inventory_id_;
    bool socketed_;
};
