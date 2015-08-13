#ifndef RECIPEPANE_H
#define RECIPEPANE_H

#include <QWidget>

namespace Ui {
class RecipePane;
}

class RecipePane : public QWidget
{
    Q_OBJECT

public:
    explicit RecipePane(QWidget *parent = 0);
    ~RecipePane();

private:
    Ui::RecipePane *ui;
};

#endif // RECIPEPANE_H
