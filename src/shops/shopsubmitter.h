#ifndef SHOPSUBMITTER_H
#define SHOPSUBMITTER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>

enum ShopSubmissionData {
    SHOP_DATA_THREAD = QNetworkRequest::User,
};

enum ShopSubmissionState {
    SHOP_SUBMISSION_IDLE,
    SHOP_SUBMISSION_STARTED,
    SHOP_SUBMISSION_SUBMITTING,
    SHOP_SUBMISSION_BUMP_STARTED,
    SHOP_SUBMISSION_BUMPING,
    SHOP_SUBMISSION_FINISHED
};

struct ShopSubmission {
    QString threadId;
    QString shopData;
    bool bumpAfter;
    int timerId;
    QNetworkReply* currentReply;
    ShopSubmissionState state;

    QVariantHash data;
};

class ShopSubmitter : public QObject
{
    Q_OBJECT
public:
    explicit ShopSubmitter(QNetworkAccessManager* manager);

    bool IsSubmitting(const QString &threadId) {
        return submissions.contains(threadId) &&
               submissions.value(threadId)->state != SHOP_SUBMISSION_IDLE &&
               submissions.value(threadId)->state != SHOP_SUBMISSION_FINISHED;
    }

    int Count() {
        int count = 0;
        for (ShopSubmission* shop : submissions) {
            if (shop->state != SHOP_SUBMISSION_IDLE &&
                shop->state != SHOP_SUBMISSION_FINISHED) {
                count++;
            }
        }
        return count;
    }

    void SetTimeout(int t) { timeout = t; }
    int GetTimeout() { return timeout; }

protected:
    void timerEvent(QTimerEvent *event);
signals:
    void ShopSubmitted(const QString &threadId);
    void ShopBumped(const QString &threadId);
    void ShopSubmissionError(const QString &threadId, const QString &error);
public slots:
    void BeginShopSubmission(const QString &threadId, const QString &shopData, bool bumpAfter = false);
private slots:
    void OnSubmissionPageFinished();
    void OnShopSubmitted();

    void OnBumpPageFinished();
    void OnBumpFinished();
private:
    QString ShopEditUrl(const QString &threadId);
    QString ShopBumpUrl(const QString &threadId);
    ShopSubmission* ExtractResponse(QNetworkReply *reply);
    void BeginShopBump(ShopSubmission* submission);
    void SubmitShop(ShopSubmission* submission);
    void BumpShop(ShopSubmission* submission);
    void RemoveSubmission(ShopSubmission* submission);
    QNetworkAccessManager* network;
    QHash<QString, ShopSubmission*> submissions;

    int timeout;
};

#endif // SHOPSUBMITTER_H
