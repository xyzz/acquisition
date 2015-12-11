#ifndef TEMPLATEDIALOG_H
#define TEMPLATEDIALOG_H

#include <QDialog>
#include <QTextBrowser>

class Application;

namespace Ui {
class TemplateDialog;
}

class TemplateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TemplateDialog(Application* app, const QString &threadId, QWidget *parent = 0);
    QString GetTemplate();
    ~TemplateDialog();

private slots:
    void on_generateButton_clicked();

private:
    Application* app_;

    Ui::TemplateDialog *ui;
    QList<QTextBrowser*> previews;
};

#endif // TEMPLATEDIALOG_H
