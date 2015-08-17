#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QDebug>

class SyntaxHighlighter : public QSyntaxHighlighter
{
public:
    SyntaxHighlighter(QTextDocument* doc)
        : QSyntaxHighlighter(doc)
    {

    }
protected:
    void highlightBlock(const QString &text)
    {
        QTextCharFormat format;
        format.setFontWeight(75);
        format.setFontUnderline(true);
        format.setForeground(Qt::lightGray);

        QTextCharFormat optionFormat;
        optionFormat.setFontUnderline(true);
        optionFormat.setForeground(Qt::yellow);

        QString pattern = "{(?<key>.+?)(?<options>(\\|(.+?))*?)}";


        QRegularExpression expression(pattern);
        QRegularExpressionMatchIterator iter = expression.globalMatch(text);
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            int keyStart = match.capturedStart("key");
            int keyLen = match.capturedLength("key");
            setFormat(keyStart, keyLen, format);

            int start = match.capturedStart("options");
            int len = match.capturedLength("options");
            setFormat(start, len, optionFormat);
        }
    }
};

#endif // SYNTAXHIGHLIGHTER_H
