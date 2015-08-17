#ifndef TEMPLATEDIALOG_H
#define TEMPLATEDIALOG_H

#include <QDialog>

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

private:
    Application* app_;

    Ui::TemplateDialog *ui;
};

#endif // TEMPLATEDIALOG_H
