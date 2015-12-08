#include "templatedialog.h"
#include "ui_templatedialog.h"
#include "external/syntaxhighlighter.h"
#include "application.h"
#include "shoptemplatemanager.h"
#include "shop.h"

TemplateDialog::TemplateDialog(Application *app, const QString &threadId, QWidget *parent) :
    QDialog(parent),
    app_(app),
    ui(new Ui::TemplateDialog)
{
    ui->setupUi(this);

    new SyntaxHighlighter(ui->plainTextEdit->document());

    QString text = app_->shop().GetShopTemplate(threadId);
    ui->plainTextEdit->setPlainText(text);

    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, [this] {
        QString text = ui->plainTextEdit->toPlainText();
        ShopTemplateManager manager(app_);
        manager.LoadTemplate(text);
        QString result = manager.Generate(app_->items()).join("\n");
        ui->textBrowser->setPlainText(result);
    });

    emit ui->plainTextEdit->textChanged();
}

QString TemplateDialog::GetTemplate() {
    return ui->plainTextEdit->toPlainText();
}

TemplateDialog::~TemplateDialog()
{
    delete ui;
}
