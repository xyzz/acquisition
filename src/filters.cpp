/*
    Copyright 2014 Ilya Zhuravlev

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

#include <memory>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>

#include "buyoutmanager.h"
#include "filters.h"
#include "util.h"
#include "porting.h"

std::unique_ptr<FilterData> Filter::CreateData() {
    return std::make_unique<FilterData>(this);
}

FilterData::FilterData(Filter *filter):
    text_query(""),
    min_filled(false),
    max_filled(false),
    r_filled(false),
    g_filled(false),
    b_filled(false),
    checked(false),
    filter_(filter)
{}

bool FilterData::Matches(const std::shared_ptr<Item> item) {
    return filter_->Matches(item, this);
}

void FilterData::FromForm() {
    filter_->FromForm(this);
}

void FilterData::ToForm() {
    filter_->ToForm(this);
}

NameSearchFilter::NameSearchFilter(QLayout *parent) {
    Initialize(parent);
}

void NameSearchFilter::FromForm(FilterData *data) {
    data->text_query = textbox_->text().toUtf8().constData();
}

void NameSearchFilter::ToForm(FilterData *data) {
    textbox_->setText(data->text_query.c_str());
}

void NameSearchFilter::ResetForm() {
    textbox_->setText("");
}

bool NameSearchFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    std::string query = data->text_query;
    std::string name = item->PrettyName();
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return name.find(query) != std::string::npos;
}

void NameSearchFilter::Initialize(QLayout *parent) {
    textbox_ = new QLineEdit;
    parent->addWidget(textbox_);
    QObject::connect(textbox_, SIGNAL(textEdited(const QString&)),
                     parent->parentWidget()->window(), SLOT(OnDelayedSearchFormChange()));
}

MinMaxFilter::MinMaxFilter(QLayout *parent, std::string property):
    property_(property),
    caption_(property)
{
    Initialize(parent);
}

MinMaxFilter::MinMaxFilter(QLayout *parent, std::string property, std::string caption):
    property_(property),
    caption_(caption)
{
    Initialize(parent);
}

void MinMaxFilter::Initialize(QLayout *parent) {
    QWidget *group = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    QLabel *label = new QLabel(caption_.c_str());
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    textbox_min_ = new QLineEdit;
    textbox_max_ = new QLineEdit;
    layout->addWidget(label);
    layout->addWidget(textbox_min_);
    layout->addWidget(textbox_max_);
    group->setLayout(layout);
    parent->addWidget(group);
    textbox_min_->setPlaceholderText("min");
    textbox_max_->setPlaceholderText("max");
    textbox_min_->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_MIN_MAX));
    textbox_max_->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_MIN_MAX));
    label->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_LABEL));
    QObject::connect(textbox_min_, SIGNAL(textEdited(const QString&)),
                     parent->parentWidget()->window(), SLOT(OnDelayedSearchFormChange()));
    QObject::connect(textbox_max_, SIGNAL(textEdited(const QString&)),
                     parent->parentWidget()->window(), SLOT(OnDelayedSearchFormChange()));
}

void MinMaxFilter::FromForm(FilterData *data) {
    data->min_filled = textbox_min_->text().size() > 0;
    data->min = textbox_min_->text().toDouble();
    data->max_filled = textbox_max_->text().size() > 0;
    data->max = textbox_max_->text().toDouble();
}

void MinMaxFilter::ToForm(FilterData *data) {
    if (data->min_filled)
        textbox_min_->setText(QString::number(data->min));
    else
        textbox_min_->setText("");
    if (data->max_filled)
        textbox_max_->setText(QString::number(data->max));
    else
        textbox_max_->setText("");
}

void MinMaxFilter::ResetForm() {
    textbox_min_->setText("");
    textbox_max_->setText("");
}

bool MinMaxFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    if (IsValuePresent(item)) {
        double value = GetValue(item);
        if (data->min_filled && data->min > value)
            return false;
        if (data->max_filled && data->max < value)
            return false;
        return true;
    } else {
        return !data->max_filled && !data->min_filled;
    }
}

bool SimplePropertyFilter::IsValuePresent(const std::shared_ptr<Item> &item) {
    return item->properties().count(property_);
}

double SimplePropertyFilter::GetValue(const std::shared_ptr<Item> &item) {
    return std::stod(item->properties().at(property_));
}

double DefaultPropertyFilter::GetValue(const std::shared_ptr<Item> &item) {
    if (!item->properties().count(property_))
        return default_value_;
    return SimplePropertyFilter::GetValue(item);
}

double RequiredStatFilter::GetValue(const std::shared_ptr<Item> &item) {
    auto &requirements = item->requirements();
    if (requirements.count(property_))
        return requirements.at(property_);
    return 0;
}

ItemMethodFilter::ItemMethodFilter(QLayout *parent, std::function<double (Item *)> func, std::string caption):
    MinMaxFilter(parent, caption, caption),
    func_(func)
{}

double ItemMethodFilter::GetValue(const std::shared_ptr<Item> &item) {
    return func_(&*item);
}

double SocketsFilter::GetValue(const std::shared_ptr<Item> &item) {
    return item->sockets_cnt();
}

double LinksFilter::GetValue(const std::shared_ptr<Item> &item) {
    return item->links_cnt();
}

SocketsColorsFilter::SocketsColorsFilter(QLayout *parent) {
    Initialize(parent, "Colors");
}

// TODO(xyz): ugh, a lot of copypasta below, perhaps this could be done
// in a nice way?
void SocketsColorsFilter::Initialize(QLayout *parent, const char* caption) {
    QWidget *group = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    QLabel *label = new QLabel(caption);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    textbox_r_ = new QLineEdit;
    textbox_r_->setPlaceholderText("R");
    textbox_g_ = new QLineEdit;
    textbox_g_->setPlaceholderText("G");
    textbox_b_ = new QLineEdit;
    textbox_b_->setPlaceholderText("B");
    layout->addWidget(label);
    layout->addWidget(textbox_r_);
    layout->addWidget(textbox_g_);
    layout->addWidget(textbox_b_);
    group->setLayout(layout);
    parent->addWidget(group);
    textbox_r_->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_RGB));
    textbox_g_->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_RGB));
    textbox_b_->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_RGB));
    label->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_LABEL));
    QObject::connect(textbox_r_, SIGNAL(textEdited(const QString&)),
                     parent->parentWidget()->window(), SLOT(OnSearchFormChange()));
    QObject::connect(textbox_g_, SIGNAL(textEdited(const QString&)),
                     parent->parentWidget()->window(), SLOT(OnSearchFormChange()));
    QObject::connect(textbox_b_, SIGNAL(textEdited(const QString&)),
                     parent->parentWidget()->window(), SLOT(OnSearchFormChange()));
}

void SocketsColorsFilter::FromForm(FilterData *data) {
    data->r_filled = textbox_r_->text().size() > 0;
    data->g_filled = textbox_g_->text().size() > 0;
    data->b_filled = textbox_b_->text().size() > 0;
    data->r = textbox_r_->text().toInt();
    data->g = textbox_g_->text().toInt();
    data->b = textbox_b_->text().toInt();
}

void SocketsColorsFilter::ToForm(FilterData *data) {
    if (data->r_filled)
        textbox_r_->setText(QString::number(data->r));
    if (data->g_filled)
        textbox_g_->setText(QString::number(data->g));
    if (data->b_filled)
        textbox_b_->setText(QString::number(data->b));
}

void SocketsColorsFilter::ResetForm() {
    textbox_r_->setText("");
    textbox_g_->setText("");
    textbox_b_->setText("");
}

bool SocketsColorsFilter::Check(int need_r, int need_g, int need_b, int got_r, int got_g, int got_b, int got_w) {
    int diff = std::max(0, need_r - got_r) + std::max(0, need_g - got_g)
        + std::max(0, need_b - got_b);
    return diff <= got_w;
}

bool SocketsColorsFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    if (!data->r_filled && !data->g_filled && !data->b_filled)
        return true;
    int need_r = 0, need_g = 0, need_b = 0;
    if (data->r_filled)
        need_r = data->r;
    if (data->g_filled)
        need_g = data->g;
    if (data->b_filled)
        need_b = data->b;
    const ItemSocketGroup &sockets = item->sockets();
    return Check(need_r, need_g, need_b, sockets.r, sockets.g, sockets.b, sockets.w);
}

LinksColorsFilter::LinksColorsFilter(QLayout *parent) {
    Initialize(parent, "Linked");
}

bool LinksColorsFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    if (!data->r_filled && !data->g_filled && !data->b_filled)
        return true;
    int need_r = 0, need_g = 0, need_b = 0;
    if (data->r_filled)
        need_r = data->r;
    if (data->g_filled)
        need_g = data->g;
    if (data->b_filled)
        need_b = data->b;
    for (auto &group : item->socket_groups()) {
        if (Check(need_r, need_g, need_b, group.r, group.g, group.b, group.w))
            return true;
    }
    return false;
}

BooleanFilter::BooleanFilter(QLayout *parent, std::string property, std::string caption):
    property_(property),
    caption_(caption)
{
    Initialize(parent);
}

void BooleanFilter::Initialize(QLayout *parent) {
    QWidget *group = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    QLabel *label = new QLabel(caption_.c_str());
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    checkbox_ = new QCheckBox;
    layout->addWidget(label);
    layout->addWidget(checkbox_);
    group->setLayout(layout);
    parent->addWidget(group);
    label->setFixedWidth(Util::TextWidth(TextWidthId::WIDTH_LABEL));

    QObject::connect(checkbox_, SIGNAL(clicked(bool)),
                     parent->parentWidget()->window(), SLOT(OnSearchFormChange()));
}

void BooleanFilter::FromForm(FilterData *data) {
    data->checked = checkbox_->isChecked();
}

void BooleanFilter::ToForm(FilterData *data) {
    checkbox_->setChecked(data->checked);
}

void BooleanFilter::ResetForm() {
    checkbox_->setChecked(false);
}

bool BooleanFilter::Matches(const std::shared_ptr<Item> & /* item */, FilterData * /* data */) {
    return true;
}

bool MTXFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    return !data->checked || item->has_mtx();
}

bool AltartFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    static std::vector<std::string> altart = {
        // season 1
        "RedBeak2.png", "Wanderlust2.png", "Ring2b.png", "Goldrim2.png", "FaceBreaker2.png", "Atzirismirror2.png",
        // season 2
        "KaruiWardAlt.png", "ShiverstingAlt.png", "QuillRainAlt.png", "OnyxAmuletAlt.png", "DeathsharpAlt.png", "CarnageHeartAlt.png",
        "TabulaRasaAlt.png", "andvariusAlt.png", "AstramentisAlt.png",
        // season 3
        "BlackheartAlt.png", "SinTrekAlt.png", "ShavronnesPaceAlt.png", "Belt3Alt.png", "EyeofChayulaAlt.png", "SundanceAlt.png",
        "ReapersPursuitAlt.png", "WindscreamAlt.png", "RainbowStrideAlt.png", "TarynsShiverAlt.png",
        // season 4
        "BrightbeakAlt.png", "RubyRingAlt.png", "TheSearingTouchAlt.png", "CloakofFlameAlt.png", "AtzirisFoibleAlt.png", "DivinariusAlt.png",
        "HrimnorsResolveAlt.png", "CarcassJackAlt.png", "TheIgnomonAlt.png", "HeatShiverAlt.png",
        // season 5
        "KaomsSignAlt.png", "StormcloudAlt.png", "FairgravesTricorneAlt.png", "MoonstoneRingAlt.png", "GiftsfromAboveAlt.png", "LeHeupofAllAlt.png",
        "QueensDecreeAlt.png", "PerandusSignetAlt.png", "AuxiumAlt.png", "dGlsbGF0ZUFsdCI7czoy",
        // season 6
        "PerandusBlazonAlt.png", "AurumvoraxAlt.png", "GoldwyrmAlt.png", "AmethystAlt.png", "DeathRushAlt.png", "RingUnique1.png", "MeginordsGirdleAlt.png",
        "SidhebreathAlt.png", "MingsHeartAlt.png", "VoidBatteryAlt.png",
        // season 7
        "Empty-Socket2.png", "PrismaticEclipseAlt.png", "ThiefsTorment2.png", "Amulet5Unique2.png", "FurryheadofstarkonjaAlt.png", "Headhunter2.png",
        "Belt6Unique2.png", "BlackgleamAlt.png", "ThousandribbonsAlt.png", "IjtzOjI6InNwIjtkOjAu",
        // season 8
        "TheThreeDragonsAlt.png", "ImmortalFleshAlt.png", "DreamFragmentsAlt2.png", "BereksGripAlt.png", "SaffellsFrameAlt.png", "BereksRespiteAlt.png",
        "LifesprigAlt.png", "PillaroftheCagedGodAlt.png", "BereksPassAlt.png", "PrismaticRingAlt.png",
        // season 9
        "Fencoil.png", "TopazRing.png", "Cherufe2.png", "cy9CbG9ja0ZsYXNrMiI7", "BringerOfRain.png", "AgateAmuletUnique2.png",
        // season 10
        "StoneofLazhwarAlt.png", "SapphireRingAlt.png", "CybilsClawAlt.png", "DoedresDamningAlt.png", "AlphasHowlAlt.png", "dCI7czoyOiJzcCI7ZDow",
        // season 11
        "MalachaisArtificeAlt.png", "MokousEmbraceAlt.png", "RusticSashAlt2.png", "MaligarosVirtuosityAlt.png", "BinosKitchenKnifeAlt.png", "WarpedTimepieceAlt.png",
        // emberwake season
        "UngilsHarmonyAlt.png", "LightningColdTwoStoneRingAlt.png", "EdgeOfMadnessAlt.png", "RashkaldorsPatienceAlt.png", "RathpithGlobeAlt.png",
        "EmberwakeAlt.png",
        // bloodgrip season
        "GoreFrenzyAlt.png", "BloodGloves.png", "BloodAmuletALT.png", "TheBloodThornALT.png", "BloodJewel.png", "BloodRIng.png",
        // soulthirst season
        "ThePrincessAlt.png", "EclipseStaff.png", "Perandus.png", "SoultakerAlt.png", "SoulthirstALT.png", "bHQiO3M6Mjoic3AiO2Q6",
        // winterheart season
        "AsphyxiasWrathRaceAlt.png", "SapphireRingRaceAlt.png", "TheWhisperingIceRaceAlt.png", "DyadianDawnRaceAlt.png", "CallOfTheBrotherhoodRaceAlt.png",
        "WinterHeart.png",
    };

    if (!data->checked)
        return true;
    for (auto &needle : altart)
        if (item->icon().find(needle) != std::string::npos)
            return true;
    return false;
}

bool PricedFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    if (!data->checked)
        return true;
    return bm_.Get(*item).IsActive();
}

double ItemlevelFilter::GetValue(const std::shared_ptr<Item> &item) {
    return item->ilvl();
}
