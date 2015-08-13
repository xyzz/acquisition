#include "recipepane.h"
#include "ui_recipepane.h"

RecipePane::RecipePane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RecipePane)
{
    ui->setupUi(this);
}

RecipePane::~RecipePane()
{
    delete ui;
}
