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

    explicit ShopTemplateManager(Application *parent);
    void LoadTemplate(const QString &temp) { shopTemplate = temp; }
    QString Generate(const Items &items);

    Items FindMatchingItems(const Items &items, QString keyword);
    QString FetchFromKey(const QString &key, const Items &items, QHash<QString, QString> *options);
    QString FetchFromEasyKey(const QString &key, QHash<QString, QString> *options);
    QString FetchFromItemsKey(const QString &key, const Items &items, QHash<QString, QString> *options);
    void WriteItems(const Items &items, QString *writer, bool includeBuyoutTag = true, bool includeNoBuyouts = false);
private:
    bool IsMatchingItem(const std::shared_ptr<Item> &item, QString keyword);
    void LoadTemplateMatchers();
    QString shopTemplate;
    QHash<QString, std::function<bool(const std::shared_ptr<Item>&)> > templateMatchers;

    Application* parent_;
};

#endif // SHOPTEMPLATEMANAGER_H
