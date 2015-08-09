#include "shoptemplatemanager.h"
#include "buyoutmanager.h"

#include <QStringList>
#include <QRegularExpression>
#include <QDebug>
#include <QDateTime>

ShopTemplateManager::ShopTemplateManager(QObject *parent) : QObject(parent) {
    LoadTemplateMatchers();
}

// The actual generation engine
QString ShopTemplateManager::Generate(const Application &app, const Items &items) {
    QString temp = shopTemplate;
    QRegularExpression expr = QRegularExpression("{(?<key>.*?)}");
    QRegularExpressionMatchIterator matcher = expr.globalMatch(shopTemplate);
    int offset = 0;
    while (matcher.hasNext()) {
        QRegularExpressionMatch match = matcher.next();
        QString key = match.captured("key").toLower();
        int startPos = offset + match.capturedStart();
        int len = match.capturedLength();

        QString replacement;

        if (key == "ign") {
            replacement = "InsertIGNHere"; // todo(novynn): actually put preferred char name?
        }
        else if (key == "lastupdated") {
            replacement = QDateTime::currentDateTime().toString("MMMM dd, yyyy hh:mm A");
        }
        else if (items.size() > 0){
            Items result;

            if (key == "everything") {
                result = items;
            }
            else if (key.startsWith("stash:")) {
                QString name = key.split(":").last();
                for (const std::shared_ptr<Item> &item : items) {
                    if (QString::fromStdString(item->location().GetLabel()).compare(name, Qt::CaseInsensitive) == 0) {
                        result.push_back(item);
                    }
                }
            }
            else {
                Items pool = items;
                QStringList keyParts = key.split("+", QString::SkipEmptyParts);
                for (QString part : keyParts) {
                    if (templateMatchers.contains(part)) {
                        qDebug() << "\tMatched a template matcher!";
                        result = FindMatchingItems(pool, part);
                        qDebug() << "\tMatched " << result.size() << "/" << pool.size() << " item/s";
                    }
                    else {
                        // todo(novynn): phase these out? Prefer {Normal+Helmet} over {NormalHelmet}
                        bool matchedRarity = false;
                        const QStringList rarities = {"normal", "magic", "rare", "unique"};
                        for (QString rarity : rarities) {
                            if (part.startsWith(rarity)) {
                                QString type = part.mid(rarity.length());

                                result = FindMatchingItems(pool, rarity);
                                result = FindMatchingItems(result, type);
                                matchedRarity = true;
                            }
                        }

                        if (matchedRarity) continue;

                        if (part.endsWith("gems")) {
                            if (part == "gems" || part == "allgems") {
                                result = FindMatchingItems(pool, part);
                            }
                            else {
                                const QStringList gemTypes = {"AoE", "Attack", "Aura", "Bow", "Cast", "Chaining", "Chaos",
                                                              "Cold", "Curse", "Duration", "Fire", "Lightning", "Melee", "Mine", "Minion",
                                                              "Movement", "Projectile", "Spell", "Totem", "Trap", "Support"};
                                for (QString gemType : gemTypes) {
                                    if (!part.startsWith(gemType, Qt::CaseInsensitive)) continue;

                                    // This will never just be first key (?)
                                    QString firstKey = gemType.toLower() + "gems";
                                    result = FindMatchingItems(pool, firstKey);

                                    if (part != firstKey) {
                                        // supportlightninggems
                                        QString secondKey = part.mid(gemType.length());
                                        result = FindMatchingItems(result, secondKey);
                                    }
                                }
                            }
                        }
                    }
                    pool = result;
                }
            }

            for (auto &item : result) {
                if (item->location().socketed())
                    continue;
                Buyout bo;
                bo.type = BUYOUT_TYPE_NONE;

                std::string hash = item->location().GetUniqueHash();
                if (app.buyout_manager().ExistsTab(hash))
                    bo = app.buyout_manager().GetTab(hash);
                if (app.buyout_manager().Exists(*item))
                    bo = app.buyout_manager().Get(*item);
                if (bo.type == BUYOUT_TYPE_NONE)
                    continue;

                replacement += QString::fromStdString(item->location().GetForumCode(app.league()));

                replacement += QString::fromStdString(BuyoutTypeAsPrefix[bo.type]);

                if (bo.type == BUYOUT_TYPE_BUYOUT || bo.type == BUYOUT_TYPE_FIXED) {
                    replacement += QString::number(bo.value).toUtf8().constData();
                    replacement += " " + QString::fromStdString(CurrencyAsTag[bo.currency]);
                }
            }
        }

        temp.replace(startPos, len, replacement);
        offset += replacement.length() - len;
    }
    return temp;
}

Items ShopTemplateManager::FindMatchingItems(const Items &items, QString keyword) {
    Items matched;
    for (auto &item : items) {
        if (IsMatchingItem(item, keyword)) {
            matched.push_back(item);
        }
    }
    return matched;
}


bool ShopTemplateManager::IsMatchingItem(const std::shared_ptr<Item> &item, QString keyword) {
    QString key = keyword.toLower();
    if (templateMatchers.contains(key)) {
        auto func = templateMatchers.value(key);
        return func(item);
    }
    return false;
}

void ShopTemplateManager::LoadTemplateMatchers() {
    templateMatchers.insert("droponlygems", [](const std::shared_ptr<Item> &item) {
        // - This ones from the wiki(http://pathofexile.gamepedia.com/Drop_Only_Gems). This will exclude quality drop only gems
        const QStringList dropOnly = {"Added Chaos Damage", "Detonate Mines",
                                      "Empower", "Enhance", "Enlighten", "Portal"};
        QString name = QString::fromStdString(item->typeLine());
        return (item->frameType() == FRAME_TYPE_GEM &&
                (dropOnly.contains(name) ||
                 name.startsWith("Vaal")) &&
                item->properties().find("Quality") == item->properties().end());
    });

    for (int i = 1; i <= 20; i++) {
        templateMatchers.insert("quality" + QString::number(i) + "gems", [i](const std::shared_ptr<Item> &item) {
            auto quality = item->properties().find("Quality");
            QString q = QString::fromStdString(quality->second);
            return item->frameType() == FRAME_TYPE_GEM && !q.isEmpty() && q == QString("+%1%").arg(i);
        });
    }

    templateMatchers.insert("leveledgems", [](const std::shared_ptr<Item> &item) {
        auto level = item->properties().find("Level");
        int l = QString::fromStdString(level->second).toInt();
        return item->frameType() == FRAME_TYPE_GEM && l > 1;
    });

    templateMatchers.insert("corruptedgems", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_GEM && item->corrupted();
    });

    //    ???{PopularGems} - This is based on the Popular Gems list in settings.xml

    //    {MagicFind} - Any item with rarity, quantity or both stats will be included
    //    {DualRes} - Any item with 2 explicit resist stats
    //    {TripRes} - Any item with 3 or more explicit resist stats
    //    {OneHanders} - All one handed weapons
    //    {TwoHanders} - All two handed weapons
    //    {Life} - All items with +to maximum Life
    //    {LifeRegen} - All items with Life Regenerated per second
    //    {CritChance} - All items with crit chance
    //    {GlobalCritChance} - All items with global crit chance
    //    {GlobalCritMultiplier} - All items with global crit multiplier
    //    {SpellDamage} - All items with spell damage
    //    {ManaRegen} - Items with added mana regen
    //    {PhysicalDamage} - Items with added physical damage
    //    {IncreasedPhysicalDamage} - Items with Increased Physical Damage
    //    {EnergyShield} - Items with increased Energy Shield

    templateMatchers.insert("vaalfragments", [](const std::shared_ptr<Item> &item) {
        QStringList fragments = {"Sacrifice at Dusk", "Sacrifice at Midnight", "Sacrifice at Noon", "Sacrifice at Dawn"};
        return fragments.contains(QString::fromStdString(item->typeLine());
    });

    templateMatchers.insert("vaaluberfragments", [](const std::shared_ptr<Item> &item) {
        QStringList fragments = {"Mortal Grief", "Mortal Rage", "Mortal Hope", "Mortal Ignorance"};
        return fragments.contains(QString::fromStdString(item->typeLine());
    });

    //    {LvlMaps} - The keyword currently accepts a value for Lvl between 66 - 100. Some examples would be: {66Maps}, {67Maps}, {68Maps}
    for (int i = 68; i < 83; i++) {
        templateMatchers.insert(QString::number(i) + "maps", [i](const std::shared_ptr<Item> &item) {
            bool isMap = QString::fromStdString(item->typeLine()).contains("Map");
            if (!isMap) return false;
            int level = 0;
            for (auto &property : item->text_properties()) {
                if (property.name != "Level") continue;
                level = QString::fromStdString(QVector<std::string>::fromStdVector(property.values).first()).toInt();

            }
            return level == i;
        });
    }

    for (int i = 4; i < 7; i++) {
        templateMatchers.insert(QString::number(i) + "link", [i](const std::shared_ptr<Item> &item) {
            return item->links_cnt() == i;
        });
    }

    templateMatchers.insert("6socket", [](const std::shared_ptr<Item> &item) {
        return item->sockets_cnt() == 6;
    });

    templateMatchers.insert("normal", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_NORMAL;
    });
    templateMatchers.insert("normalgear", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_NORMAL;
    });
    templateMatchers.insert("magic", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_MAGIC;
    });
    templateMatchers.insert("rare", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_RARE;
    });
    templateMatchers.insert("unique", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_UNIQUE;
    });
    templateMatchers.insert("uniques", [](const std::shared_ptr<Item> &item) {
        return item->frameType() == FRAME_TYPE_UNIQUE;
    });

    // Ring, Amulet, Helmet, Chest, Belt, Gloves, Boots, Axe, Claw, Bow, Dagger, Mace, Quiver, Sceptre, Staff, Sword, Shield, Wand, Flask

    const QStringList gemTypes = {"AoE", "Attack", "Aura", "Bow", "Cast", "Chaining", "Chaos",
                                  "Cold", "Curse", "Duration", "Fire", "Lightning", "Melee", "Mine", "Minion",
                                  "Movement", "Projectile", "Spell", "Totem", "Trap", "Support"};

    for (QString gemType : gemTypes) {
        QString key = gemType.toLower() + "gems";
        templateMatchers.insert(key, [gemType](const std::shared_ptr<Item> &item) {
            if (item->frameType() != FRAME_TYPE_GEM) return false;
            // Is it safe to assume that the gem types will always be the top of the properties list?
            auto types = item->text_properties()[0];
            return QString::fromStdString(types.name).contains(gemType);
        });
    }

}

