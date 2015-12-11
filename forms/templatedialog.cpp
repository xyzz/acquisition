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

    emit ui->plainTextEdit->textChanged();
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, [this] {
        ui->generateButton->setEnabled(true);
        ui->previewWidget->setEnabled(false);
        qDeleteAll(previews);
        previews.clear();
    });
}

QString TemplateDialog::GetTemplate() {
    return ui->plainTextEdit->toPlainText();
}

TemplateDialog::~TemplateDialog()
{
    delete ui;
}

void TemplateDialog::on_generateButton_clicked() {
    QString text = ui->plainTextEdit->toPlainText();
    ShopTemplateManager manager(app_);
    manager.LoadTemplate(text);
    QStringList results = manager.Generate(app_->items());
    for (int i = 0; i < results.size(); i++) {
        QTextBrowser* preview = new QTextBrowser(ui->previewWidget);
        ui->previewWidgetLayout->addWidget(preview);
        previews.append(preview);
        preview->setPlainText(results.at(i));
    }

    ui->generateButton->setEnabled(false);
    ui->previewWidget->setEnabled(true);
}
