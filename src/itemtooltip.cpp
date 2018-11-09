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

static const QImage link_h(":/sockets/linkH.png");
static const QImage link_v(":/sockets/linkV.png");
static const QImage elder_1x1(":/backgrounds/ElderBackground 1x1.png");
static const QImage elder_1x3(":/backgrounds/ElderBackground 1x3.png");
static const QImage elder_1x4(":/backgrounds/ElderBackground 1x4.png");
static const QImage elder_2x2(":/backgrounds/ElderBackground 2x2.png");
static const QImage elder_2x3(":/backgrounds/ElderBackground 2x3.png");
static const QImage elder_2x4(":/backgrounds/ElderBackground 2x4.png");
static const QImage shaper_1x1(":/backgrounds/ShaperBackground 1x1.png");
static const QImage shaper_1x3(":/backgrounds/ShaperBackground 1x3.png");
static const QImage shaper_1x4(":/backgrounds/ShaperBackground 1x4.png");
static const QImage shaper_2x2(":/backgrounds/ShaperBackground 2x2.png");
static const QImage shaper_2x3(":/backgrounds/ShaperBackground 2x3.png");
static const QImage shaper_2x4(":/backgrounds/ShaperBackground 2x4.png");

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

static std::vector<std::string> kPoEColors = {
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
    // Talisman level is not really a requirement but it lives in the requirements section
    if (item.talisman_tier())
        text += "Talisman Tier: " + std::to_string(item.talisman_tier()) + "<br>";
    for (auto &requirement : item.text_requirements()) {
        text += first ? "Requires " : ", ";
        first = false;
        text += requirement.name + " " + ColorPropertyValue(requirement.value);
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

static std::string GenerateItemInfo(const Item &item, const std::string &key, bool fancy) {
    std::vector<std::string> sections;

    std::string properties_text = GenerateProperties(item);
    if (properties_text.size() > 0)
        sections.push_back(properties_text);

    std::string requirements_text = GenerateRequirements(item);
    if (requirements_text.size() > 0)
        sections.push_back(requirements_text);

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
        if (!first) {
            text += "<br>";
            if (fancy)
                text += "<img src=':/tooltip/Separator" + key + ".png'><br>";
            else
                text += "<hr>";
        }
        first = false;
        text += s;
    }
    if (!fancy)
        text = ColorPropertyValue(ItemPropertyValue{ item.PrettyName(), 0 }) + "<hr>" + text;
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

void GenerateItemHeaderSide(const Item &item, Ui::MainWindow *ui, std::string header_path_prefix, bool singleline, bool leftNotRight) {
    QImage header(QString::fromStdString(header_path_prefix + (leftNotRight ? "Left.png" : "Right.png")));
    QSize header_size = singleline ? QSize(29, 34) : QSize(44, 54);
    header = header.scaled(header_size);

    QPixmap header_pixmap(header_size);
    header_pixmap.fill(Qt::transparent);
    QPainter header_painter(&header_pixmap);
    header_painter.drawImage(0, 0, header);

    if(item.shaper() || item.elder()){
        std::string overlay = ":/tooltip/";
        if(item.shaper()) {
            overlay += "Shaper";
        }
        if(item.elder()) {
            overlay += "Elder";
        }
        overlay += "ItemSymbol.png";

        QImage overlay_image(QString::fromStdString(overlay));
        overlay_image = overlay_image.scaled(27, 27);
        int overlay_x = singleline ? (leftNotRight ? 2: 1) : (leftNotRight ? 2: 15);
        int overlay_y = (int) ( 0.5 * (header.height() - overlay_image.height()) );
        header_painter.drawImage(overlay_x, overlay_y, overlay_image);
    }

    if(leftNotRight) {
        ui->itemHeaderLeft->setFixedSize(header_size);
        ui->itemHeaderLeft->setPixmap(header_pixmap);
    } else {
        ui->itemHeaderRight->setFixedSize(header_size);
        ui->itemHeaderRight->setPixmap(header_pixmap);
    }
}

void GenerateItemTooltip(const Item &item, Ui::MainWindow *ui) {
    size_t frame = item.frameType();
    if (frame >= FrameToKey.size())
        frame = 0;
    std::string key = FrameToKey[frame];

    ui->propertiesLabel->setText(GenerateItemInfo(item, key, true).c_str());
    ui->itemTextTooltip->setText(GenerateItemInfo(item, key, false).c_str());
    UpdateMinimap(item, ui);

    bool singleline = item.name().empty();
    if (singleline) {
        ui->itemNameFirstLine->hide();
        ui->itemNameSecondLine->setAlignment(Qt::AlignCenter);
        ui->itemNameContainerWidget->setFixedSize(16777215, 34);
    } else {
        ui->itemNameFirstLine->show();
        ui->itemNameFirstLine->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        ui->itemNameSecondLine->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        ui->itemNameContainerWidget->setFixedSize(16777215, 54);
    }
    std::string suffix = (singleline && (frame == FRAME_TYPE_RARE || frame == FRAME_TYPE_UNIQUE)) ? "SingleLine" : "";
    std::string header_path_prefix = ":/tooltip/ItemsHeader" + key + suffix;

    GenerateItemHeaderSide(item, ui, header_path_prefix, singleline, true);
    GenerateItemHeaderSide(item, ui, header_path_prefix, singleline, false);

    ui->itemNameContainerWidget->setStyleSheet(("border-radius: 0px; border: 0px; border-image: url(" + header_path_prefix + "Middle.png);").c_str());

    ui->itemNameFirstLine->setText(item.name().c_str());
    ui->itemNameSecondLine->setText(item.typeLine().c_str());

    std::string css = "border-image: none; background-color: transparent; font-size: 20px; color: " + FrameToColor[frame];
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

    if(item.shaper() || item.elder()){
        std::string background = ":/backgrounds/";
        if(item.shaper()) {
            background += "Shaper";
        }
        if(item.elder()) {
            background += "Elder";
        }
        background += "Background " + std::to_string(width) + "x" + std::to_string(height) + ".png";

        QImage background_image(QString::fromStdString(background));
        overlay.drawImage(0, 0, background_image);
    }

    overlay.drawImage(0, 0, image);

    overlay.drawPixmap((int)(0.5*(image.width() - cropped.width())),
                       (int)(0.5*(image.height() - cropped.height())), cropped);

    ui->imageLabel->setPixmap(base);
}
