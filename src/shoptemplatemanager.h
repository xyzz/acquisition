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
    explicit ShopTemplateManager(QObject *parent = 0);
    void LoadTemplate(const QString &temp) { shopTemplate = temp; }
    QString Generate(const Application &app, const Items &items);

    Items FindMatchingItems(const Items &items, QString keyword);
private:
    bool IsMatchingItem(const std::shared_ptr<Item> &item, QString keyword);
    void LoadTemplateMatchers();
    QString shopTemplate;
    QHash<QString, std::function<bool(const std::shared_ptr<Item>&)> > templateMatchers;
};

#endif // SHOPTEMPLATEMANAGER_H
