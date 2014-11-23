#include "itemlocation.h"

#include <QString>
#include "rapidjson/document.h"
#include "jsoncpp/json.h"

ItemLocation::ItemLocation():
    socketed_(false)
{}

ItemLocation::ItemLocation(const rapidjson::Value &root) {
    FromItemJson(root);
}

void ItemLocation::FromItemJson(const rapidjson::Value &root) {
    if (root.HasMember("_type")) {
        type_ = static_cast<ItemLocationType>(root["_type"].GetInt());
        if (type_ == ItemLocationType::STASH) {
            tab_label_ = root["_tab_label"].GetString();
            tab_id_ = root["_tab"].GetInt();
        } else {
            character_ = root["_character"].GetString();
        }
        socketed_ = false;
        if (root.HasMember("_socketed"))
            socketed_ = root["_socketed"].GetBool();
        // socketed items have x/y pointing to parent
        if (socketed_) {
            x_ = root["_x"].GetInt();
            y_ = root["_y"].GetInt();
        }
    }
    if (root.HasMember("x") && root.HasMember("y")) {
        x_ = root["x"].GetInt();
        y_ = root["y"].GetInt();
    }
    w_ = root["w"].GetInt();
    h_ = root["h"].GetInt();
    //inventory_id_ = root["inventoryId"].GetString();
}

void ItemLocation::ToItemJson(rapidjson::Value *root) {
#if 0
    (*root)["_type"] = static_cast<int>(type_);
    if (type_ == ItemLocationType::STASH) {
        (*root)["_tab"] = tab_id_;
        (*root)["_tab_label"] = tab_label_;
    } else {
        (*root)["_character"] = character_;
    }
    if (socketed_) {
        (*root)["_x"] = x_;
        (*root)["_y"] = y_;
    }
    (*root)["_socketed"] = socketed_;
#endif
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
