    #ifndef SHOPTEMPLATEMANAGER_H
#define SHOPTEMPLATEMANAGER_H

#include <QObject>

// Using Procurement-style templates
class ShopTemplateManager : public QObject
{
    Q_OBJECT
public:
    explicit ShopTemplateManager(QObject *parent = 0);

signals:

public slots:
};

#endif // SHOPTEMPLATEMANAGER_H
