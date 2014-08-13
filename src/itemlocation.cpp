#include "itemlocation.h"

#include <QString>
#include "jsoncpp/json.h"

ItemLocation::ItemLocation()
{}

ItemLocation::ItemLocation(const Json::Value &root) {
    FromItemJson(root);
}

void ItemLocation::FromItemJson(const Json::Value &root) {
    if (root.isMember("_type")) {
        type_ = static_cast<ItemLocationType>(root["_type"].asInt());
        if (type_ == ItemLocationType::STASH) {
            tab_label_ = root["_tab_label"].asString();
            tab_id_ = root["_tab"].asInt();
        } else {
            character_ = root["_character"].asString();
        }
    }
    x_ = root["x"].asInt();
    y_ = root["y"].asInt();
    w_ = root["w"].asInt();
    h_ = root["h"].asInt();
    inventory_id_ = root["inventoryId"].asString();
}

void ItemLocation::ToItemJson(Json::Value *root) {
    (*root)["_type"] = static_cast<int>(type_);
    if (type_ == ItemLocationType::STASH) {
        (*root)["_tab"] = tab_id_;
        (*root)["_tab_label"] = tab_label_;
    } else {
        (*root)["_character"] = character_;
    }
}

std::string ItemLocation::GetHeader() const {
    if (type_ == ItemLocationType::STASH) {
        QString format("#%1, \"%2\"");
        return format.arg(tab_id_ + 1).arg(tab_label_.c_str()).toStdString();
    } else {
        return character_;
    }
}

QRectF ItemLocation::GetRect() const {
    QRectF result;
    result.setX(MINIMAP_SIZE * x_ / INVENTORY_SLOTS);
    result.setY(MINIMAP_SIZE * y_ / INVENTORY_SLOTS);
    result.setWidth(MINIMAP_SIZE * w_ / INVENTORY_SLOTS);
    result.setHeight(MINIMAP_SIZE * h_ / INVENTORY_SLOTS);
    return result;
}

std::string ItemLocation::GetForumCode(const std::string &league) const {
    if (type_ == ItemLocationType::STASH) {
        QString format("[linkItem location=\"Stash%1\" league=\"%2\" x=\"%3\" y=\"%4\"]");
        return format.arg(QString::number(tab_id_ + 1), league.c_str(), QString::number(x_), QString::number(y_)).toStdString();
    } else {
        QString format("[linkItem location=\"%1\" character=\"%2\" x=\"%3\" y=\"%4\"]");
        return format.arg(inventory_id_.c_str(), character_.c_str(), QString::number(x_), QString::number(y_)).toStdString();
    }
}

std::string ItemLocation::GetUniqueHash() const {
    if (type_ == ItemLocationType::STASH)
        return "stash:" + tab_label_;
    else
        return "character:" + character_;
}

bool ItemLocation::operator<(const ItemLocation &other) const {
    if (type_ != other.type_)
        return type_ < other.type_;
    if (type_ == ItemLocationType::STASH)
        return tab_id_ < other.tab_id_;
    else
        return character_ < other.character_;
}
