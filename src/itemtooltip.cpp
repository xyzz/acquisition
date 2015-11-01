/*
    Copyright 2015 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "itemtooltip.h"

#include <QPainter>
#include <QString>
#include <string>
#include <vector>
#include "QsLog.h"

#include "item.h"
#include "util.h"

/*
    PoE colors:
    Default: 0
    Augmented: 1
    Unmet: 2
    PhysicalDamage: 3
    FireDamage: 4
    ColdDamage: 5
    LightningDamage: 6
    ChaosDamage: 7
    MagicItem: 8
    RareItem: 9
    UniqueItem: 10
*/

std::vector<std::string> kPoEColors = {
    "#fff",
    "#88f",
    "#d20000",
    "#fff",
    "#960000",
    "#366492",
    "gold",
    "#d02090"
};

static std::string ColorPropertyValue(const ItemPropertyValue &value) {
    size_t type = value.type;
    if (type >= kPoEColors.size())
        type = 0;
    return "<font color='" + kPoEColors[type] + "'>" + value.str + "</font>";
}

static std::string FormatProperty(const ItemProperty &prop) {
    if (prop.display_mode == 3) {
        QString format(prop.name.c_str());
        for (auto &value : prop.values)
            format = format.arg(ColorPropertyValue(value).c_str());
        return format.toStdString();
    }
    std::string text = prop.name;
    if (prop.values.size()) {
        if (prop.name.size() > 0)
            text += ": ";
        bool first = true;
        for (auto &value : prop.values) {
            if (!first)
                text += ", ";
            first = false;
            text += ColorPropertyValue(value);
        }
    }
    return text;
}

static std::string GenerateProperties(const Item &item) {
    std::string text;
    bool first = true;
    for (auto &property : item.text_properties()) {
        if (!first)
            text += "<br>";
        first = false;
        text += FormatProperty(property);
    }

    return text;
}

static std::string GenerateRequirements(const Item &item) {
    std::string text;
    bool first = true;
    for (auto &requirement : item.text_requirements()) {
        if (!first)
            text += ", ";
        first = false;
        text += requirement.name + ": " + ColorPropertyValue(requirement.value);
    }
    return text;
}

static std::string ModListAsString(const ItemMods &list) {
    std::string mods;
    bool first = true;
    for (auto &mod : list) {
        mods += (first ? "" : "<br>") + mod;
        first = false;
    }
    if (mods.empty())
        return "";
    return ColorPropertyValue(ItemPropertyValue{ mods, 1 });
}

static std::vector<std::string> GenerateMods(const Item &item) {
    std::vector<std::string> out;
    auto &mods = item.text_mods();
    for (auto &mod_type : ITEM_MOD_TYPES) {
        std::string mod_list = ModListAsString(mods.at(mod_type));
        if (!mod_list.empty())
            out.push_back(mod_list);
    }
    return out;
}

static std::string GenerateItemInfo(const Item &item, const std::string &key) {
    std::vector<std::string> sections;

    std::string properties_text = GenerateProperties(item);
    if (properties_text.size() > 0)
        sections.push_back(properties_text);

    std::string requirements_text = GenerateRequirements(item);
    if (requirements_text.size() > 0)
        sections.push_back("Requires " + requirements_text);

    std::vector<std::string> mods = GenerateMods(item);
    sections.insert(sections.end(), mods.begin(), mods.end());

    std::string unmet;
    if (!item.identified())
        unmet += "Unidentified";
    if (item.corrupted())
        unmet += (unmet.empty() ? "" : "<br>") + std::string("Corrupted");
    if (!unmet.empty())
        sections.push_back(ColorPropertyValue(ItemPropertyValue{ unmet, 2 }));

    std::string text;
    bool first = true;
    for (auto &s : sections) {
        if (!first)
            text += "<br><img src=':/tooltip/Separator" + key + ".png'><br>";
        first = false;
        text += s;
    }
    return "<center>" + text + "</center>";
}

static std::vector<std::string> FrameToKey = {
    "White",
    "Magic",
    "Rare",
    "Unique",
    "Gem",
    "Currency"
};

static std::vector<std::string> FrameToColor = {
    "#c8c8c8",
    "#88f",
    "#ff7",
    "#af6025",
    "#1ba29b",
    "#aa9e82"
};

static void UpdateMinimap(const Item &item, Ui::MainWindow *ui) {
    QPixmap pixmap(MINIMAP_SIZE, MINIMAP_SIZE);
    pixmap.fill(QColor("transparent"));

    QPainter painter(&pixmap);
    painter.setBrush(QBrush(QColor(0x0c, 0x0b, 0x0b)));
    painter.drawRect(0, 0, MINIMAP_SIZE, MINIMAP_SIZE);
    const ItemLocation &location = item.location();
    painter.setBrush(QBrush(location.socketed() ? Qt::blue : Qt::red));
    QRectF rect = location.GetRect();
    painter.drawRect(rect);

    ui->minimapLabel->setPixmap(pixmap);
}

void GenerateItemTooltip(const Item &item, Ui::MainWindow *ui) {
    size_t frame = item.frameType();
    if (frame >= FrameToKey.size())
        frame = 0;
    std::string key = FrameToKey[frame];

    ui->propertiesLabel->setText(GenerateItemInfo(item, key).c_str());
    UpdateMinimap(item, ui);

    bool singleline = item.name().empty();
    std::string suffix = "";
    if (singleline && (frame == FRAME_TYPE_RARE || frame == FRAME_TYPE_UNIQUE))
        suffix = "SingleLine";

    if (singleline) {
        ui->itemNameFirstLine->hide();
        ui->itemNameSecondLine->setAlignment(Qt::AlignCenter);
        ui->itemNameContainerWidget->setFixedSize(16777215, 34);
        ui->itemHeaderLeft->setFixedSize(29, 34);
        ui->itemHeaderRight->setFixedSize(29, 34);
    } else {
        ui->itemNameFirstLine->show();
        ui->itemNameFirstLine->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        ui->itemNameSecondLine->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        ui->itemNameContainerWidget->setFixedSize(16777215, 54);
        ui->itemHeaderLeft->setFixedSize(44, 54);
        ui->itemHeaderRight->setFixedSize(44, 54);
    }

    ui->itemHeaderLeft->setStyleSheet(("border-image: url(:/tooltip/ItemHeader" + key + suffix + "Left.png);").c_str());
    ui->itemNameContainerWidget->setStyleSheet(("border-image: url(:/tooltip/ItemHeader" + key + suffix + "Middle.png);").c_str());
    ui->itemHeaderRight->setStyleSheet(("border-image: url(:/tooltip/ItemHeader" + key + suffix + "Right.png);").c_str());

    ui->itemNameFirstLine->setText(item.name().c_str());
    ui->itemNameSecondLine->setText(item.typeLine().c_str());

    std::string css = "border-image: none; font-size: 20px; color: " + FrameToColor[frame];
    ui->itemNameFirstLine->setStyleSheet(css.c_str());
    ui->itemNameSecondLine->setStyleSheet(css.c_str());
}

void GenerateItemIcon(const Item &item, const QImage &image, Ui::MainWindow *ui) {
    int height = item.h();
    int width = item.w();
    int socket_rows = 0;
    int socket_columns = 0;
    // this will ensure we have enough room to draw the slots
    QPixmap pixmap(width * PIXELS_PER_SLOT, height * PIXELS_PER_SLOT);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    static const QImage link_h(":/sockets/linkH.png");
    static const QImage link_v(":/sockets/linkV.png");
    ItemSocket prev = { 255, '-' };
    size_t i = 0;

    auto &sockets = item.text_sockets();
    if (sockets.size() == 0) {
        // Do nothing
    } else if (sockets.size() == 1) {
        auto &socket = sockets.front();
        QImage socket_image(":/sockets/" + QString(socket.attr) + ".png");
        painter.drawImage(0, PIXELS_PER_SLOT * i, socket_image);
        socket_rows = 1;
        socket_columns = 1;
    } else {
        for (auto &socket : sockets) {
            bool link = socket.group == prev.group;
            QImage socket_image(":/sockets/" + QString(socket.attr) + ".png");
            if (width == 1) {
                painter.drawImage(0, PIXELS_PER_SLOT * i, socket_image);
                if (link)
                    painter.drawImage(16, PIXELS_PER_SLOT * i - 19, link_v);
                socket_columns = 1;
                socket_rows = i + 1;
            } else /* w == 2 */ {
                int row = i / 2;
                int column = i % 2;
                if (row % 2 == 1)
                    column = 1 - column;
                socket_columns = qMax(column + 1, socket_columns);
                socket_rows = qMax(row + 1, socket_rows);
                painter.drawImage(PIXELS_PER_SLOT * column, PIXELS_PER_SLOT * row, socket_image);
                if (link) {
                    if (i == 1 || i == 3 || i == 5) {
                        // horizontal link
                        painter.drawImage(
                            PIXELS_PER_SLOT - LINKH_WIDTH / 2,
                            row * PIXELS_PER_SLOT + PIXELS_PER_SLOT / 2 - LINKH_HEIGHT / 2,
                            link_h
                        );
                    } else if (i == 2) {
                        painter.drawImage(
                            PIXELS_PER_SLOT * 1.5 - LINKV_WIDTH / 2,
                            row * PIXELS_PER_SLOT - LINKV_HEIGHT / 2,
                            link_v
                        );
                    } else if (i == 4) {
                        painter.drawImage(
                            PIXELS_PER_SLOT / 2 - LINKV_WIDTH / 2,
                            row * PIXELS_PER_SLOT - LINKV_HEIGHT / 2,
                            link_v
                        );
                    } else {
                        QLOG_ERROR() << "No idea how to draw link for" << item.PrettyName().c_str();
                    }
                }
            }

            prev = socket;
            ++i;
        }
    }

    QPixmap cropped = pixmap.copy(0, 0, PIXELS_PER_SLOT * socket_columns,
                                        PIXELS_PER_SLOT * socket_rows);

    QPixmap base(image.width(), image.height());
    base.fill(Qt::transparent);
    QPainter overlay(&base);
    overlay.drawImage(0, 0, image);

    overlay.drawPixmap((int)(0.5*(image.width() - cropped.width())),
                       (int)(0.5*(image.height() - cropped.height())), cropped);

    ui->imageLabel->setPixmap(base);
}
