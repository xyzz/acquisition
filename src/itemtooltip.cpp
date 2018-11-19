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


static const int LINKH_HEIGHT = 16;
static const int LINKH_WIDTH = 38;
static const int LINKV_HEIGHT = LINKH_WIDTH;
static const int LINKV_WIDTH = LINKH_HEIGHT;
static const QImage link_h(":/sockets/linkH.png");
static const QImage link_v(":/sockets/linkV.png");
static const QImage elder_1x1(":/backgrounds/ElderBackground_1x1.png");
static const QImage elder_1x3(":/backgrounds/ElderBackground_1x3.png");
static const QImage elder_1x4(":/backgrounds/ElderBackground_1x4.png");
static const QImage elder_2x1(":/backgrounds/ElderBackground_2x1.png");
static const QImage elder_2x2(":/backgrounds/ElderBackground_2x2.png");
static const QImage elder_2x3(":/backgrounds/ElderBackground_2x3.png");
static const QImage elder_2x4(":/backgrounds/ElderBackground_2x4.png");
static const QImage shaper_1x1(":/backgrounds/ShaperBackground_1x1.png");
static const QImage shaper_1x3(":/backgrounds/ShaperBackground_1x3.png");
static const QImage shaper_1x4(":/backgrounds/ShaperBackground_1x4.png");
static const QImage shaper_2x1(":/backgrounds/ShaperBackground_2x1.png");
static const QImage shaper_2x2(":/backgrounds/ShaperBackground_2x2.png");
static const QImage shaper_2x3(":/backgrounds/ShaperBackground_2x3.png");
static const QImage shaper_2x4(":/backgrounds/ShaperBackground_2x4.png");
static const QImage shaper_icon(":/tooltip/ShaperItemSymbol.png");
static const QImage elder_icon(":/tooltip/ElderItemSymbol.png");
static const int HEADER_SINGLELINE_WIDTH = 29;
static const int HEADER_SINGLELINE_HEIGHT = 34;
static const int HEADER_DOUBLELINE_WIDTH = 44;
static const int HEADER_DOUBLELINE_HEIGHT = 54;
static const QSize HEADER_SINGLELINE_SIZE(HEADER_SINGLELINE_WIDTH, HEADER_SINGLELINE_HEIGHT);
static const QSize HEADER_DOUBLELINE_SIZE(HEADER_DOUBLELINE_WIDTH, HEADER_DOUBLELINE_HEIGHT);
static const QSize HEADER_OVERLAY_SIZE(27, 27);

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

void GenerateItemHeaderSide(QLabel *itemHeader, bool leftNotRight, std::string header_path_prefix, bool singleline, Item::BASE_TYPES base) {
    QImage header(QString::fromStdString(header_path_prefix + (leftNotRight ? "Left.png" : "Right.png")));
    QSize header_size = singleline ? HEADER_SINGLELINE_SIZE : HEADER_DOUBLELINE_SIZE;
    header = header.scaled(header_size);

    QPixmap header_pixmap(header_size);
    header_pixmap.fill(Qt::transparent);
    QPainter header_painter(&header_pixmap);
    header_painter.drawImage(0, 0, header);

    if(base == Item::BASE_SHAPER || base == Item::BASE_ELDER) {
        QImage overlay_image = base == Item::BASE_SHAPER ? shaper_icon : elder_icon;
        overlay_image = overlay_image.scaled(HEADER_OVERLAY_SIZE);
        int overlay_x = singleline ? (leftNotRight ? 2: 1) : (leftNotRight ? 2: 15);
        int overlay_y = (int) ( 0.5 * (header.height() - overlay_image.height()) );
        header_painter.drawImage(overlay_x, overlay_y, overlay_image);
    }

    itemHeader->setFixedSize(header_size);
    itemHeader->setPixmap(header_pixmap);
}

void UpdateItemTooltip(const Item &item, Ui::MainWindow *ui) {
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
        ui->itemNameContainerWidget->setFixedSize(16777215, HEADER_SINGLELINE_HEIGHT);
    } else {
        ui->itemNameFirstLine->show();
        ui->itemNameFirstLine->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        ui->itemNameSecondLine->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        ui->itemNameContainerWidget->setFixedSize(16777215, HEADER_DOUBLELINE_HEIGHT);
    }
    std::string suffix = (singleline && (frame == FRAME_TYPE_RARE || frame == FRAME_TYPE_UNIQUE)) ? "SingleLine" : "";
    std::string header_path_prefix = ":/tooltip/ItemsHeader" + key + suffix;

    GenerateItemHeaderSide(ui->itemHeaderLeft, true, header_path_prefix, singleline, item.baseType());
    GenerateItemHeaderSide(ui->itemHeaderRight, false, header_path_prefix, singleline, item.baseType());

    ui->itemNameContainerWidget->setStyleSheet(("border-radius: 0px; border: 0px; border-image: url(" + header_path_prefix + "Middle.png);").c_str());

    ui->itemNameFirstLine->setText(item.name().c_str());
    ui->itemNameSecondLine->setText(item.typeLine().c_str());

    std::string css = "border-image: none; background-color: transparent; font-size: 20px; color: " + FrameToColor[frame];
    ui->itemNameFirstLine->setStyleSheet(css.c_str());
    ui->itemNameSecondLine->setStyleSheet(css.c_str());
}

QPixmap GenerateItemSockets(const int width, const int height, const std::vector<ItemSocket> &sockets) {
    QPixmap pixmap(width * PIXELS_PER_SLOT, height * PIXELS_PER_SLOT);  // This will ensure we have enough room to draw the slots
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    int socket_rows = 0;
    int socket_columns = 0;
    ItemSocket prev = { 255, '-' };
    size_t i = 0;

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
                    switch(i){
                        case 1:
                        case 3:
                        case 5:
                            // horizontal link
                            painter.drawImage(
                                PIXELS_PER_SLOT - LINKH_WIDTH / 2,
                                row * PIXELS_PER_SLOT + PIXELS_PER_SLOT / 2 - LINKH_HEIGHT / 2,
                                link_h
                            );
                            break;
                        case 2:
                            painter.drawImage(
                                PIXELS_PER_SLOT * 1.5 - LINKV_WIDTH / 2,
                                row * PIXELS_PER_SLOT - LINKV_HEIGHT / 2,
                                link_v
                            );
                            break;
                        case 4:
                            painter.drawImage(
                                PIXELS_PER_SLOT / 2 - LINKV_WIDTH / 2,
                                row * PIXELS_PER_SLOT - LINKV_HEIGHT / 2,
                                link_v
                            );
                            break;
                        default:
                            QLOG_ERROR() << "No idea how to draw link for socket " << i;
                            break;
                    }
                }
            }

            prev = socket;
            ++i;
        }
    }

    return pixmap.copy(0, 0, PIXELS_PER_SLOT * socket_columns,
                                            PIXELS_PER_SLOT * socket_rows);
}

QPixmap GenerateItemIcon(const Item &item, const QImage &image) {
    const int height = item.h();
    const int width = item.w();

    QPixmap layered(image.width(), image.height());
    layered.fill(Qt::transparent);
    QPainter layered_painter(&layered);

    if(item.shaper() || item.elder()){
        // Assumes width <= 2
        const QImage *background_image = nullptr;
        if(item.shaper()) {
            switch(height) {
                case 1:
                    background_image = width == 1 ? &shaper_1x1 : &shaper_2x1;
                    break;
                case 2:
                    background_image = width == 1 ? nullptr : &shaper_2x2;
                    break;
                case 3:
                    background_image = width == 1 ? &shaper_1x3 : &shaper_2x3;
                    break;
                case 4:
                    background_image = width == 1 ? &shaper_1x4 : &shaper_2x4;
                    break;
                default:
                    break;
            }
        } else {    // Elder
            switch(height) {
                case 1:
                    background_image = width == 1 ? &elder_1x1 : &elder_2x1;
                    break;
                case 2:
                    background_image = width == 1 ? nullptr : &elder_2x2;
                    break;
                case 3:
                    background_image = width == 1 ? &elder_1x3 : &elder_2x3;
                    break;
                case 4:
                    background_image = width == 1 ? &elder_1x4 : &elder_2x4;
                    break;
                default:
                    break;
            }
        }
        if(background_image) {
            layered_painter.drawImage(0, 0, *background_image);
        } else {
            QLOG_ERROR() << "Problem drawing background for " << item.PrettyName().c_str();
        }
    }

    layered_painter.drawImage(0, 0, image);

    if(item.text_sockets().size() > 0) {
        QPixmap sockets = GenerateItemSockets(width, height, item.text_sockets());

        layered_painter.drawPixmap((int)(0.5*(image.width() - sockets.width())),
                       (int)(0.5*(image.height() - sockets.height())), sockets);    // Center sockets on overall image
    }

    return layered;
}

QString GenerateItemPOBText(const Item &item) {
    return QString::fromStdString(item.PrettyName());
}
