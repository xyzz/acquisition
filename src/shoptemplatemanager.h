#ifndef SHOPTEMPLATEMANAGER_H
#define SHOPTEMPLATEMANAGER_H

#include <QObject>
#include <QHash>
#include "application.h"
#include "item.h"

// Using Procurement-style templates
class ShopTemplateManager : public QObject
{
    Q_OBJECT
public:

    enum ShopTemplateContainType {
        CONTAIN_TYPE_NONE,
        CONTAIN_TYPE_WRAP,
        CONTAIN_TYPE_GROUP
    };

    enum ShopTemplateSectionType {
        SECTION_TYPE_NONE,
        SECTION_TYPE_SPOILER
    };

    class ShopTemplateSection {
    public:
        ShopTemplateSection(QStringList contents, ShopTemplateSectionType type = SECTION_TYPE_NONE, QString name = QString());
        ShopTemplateSection(QString content, ShopTemplateSectionType type = SECTION_TYPE_NONE, QString name = QString());

        QString generate(int start = 0, int items = -1);

        int size(int start = 0, int items = -1);
        int items() {
            return contents.size();
        }

        bool isEmpty();
        ShopTemplateSectionType getType();
    private:
        QStringList contents;
        ShopTemplateSectionType type;
        QString header;
        QString footer;

        QStringList spliced(int start, int length) {
            return contents.mid(start, length);
        }
    };

    struct ShopTemplateRecord {
        QString key;
        QHash<QString, QString> options;
        int templateIndex;
    };

    explicit ShopTemplateManager(Application *parent);

    void SetSharedItems(bool shared = true) {
        shared_ = shared;
    }

    void LoadTemplate(const QString &temp) { shopTemplate = temp; }
    QStringList Generate(const Items &items);

    Items FindMatchingItems(const Items &items, QString keyword);
    QList<ShopTemplateSection> FetchFromKey(const QString &key, const Items &items, QHash<QString, QString> *options);
    QList<ShopTemplateSection> FetchFromEasyKey(const QString &key, QHash<QString, QString> *options);
    QList<ShopTemplateSection> FetchFromItemsKey(const QString &key, const Items &items, QHash<QString, QString> *options);
    QStringList WriteItems(const Items &items, bool includeBuyoutTag = true, bool includeNoBuyouts = false);
private:
    bool IsMatchingItem(const std::shared_ptr<Item> &item, QString keyword);
    void LoadTemplateMatchers();
    QString shopTemplate;
    QHash<QString, std::function<bool(const std::shared_ptr<Item>&)> > templateMatchers;

    Application* parent_;
    bool shared_;
};

#endif // SHOPTEMPLATEMANAGER_H
