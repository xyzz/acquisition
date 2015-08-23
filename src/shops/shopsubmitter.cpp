#include "shopsubmitter.h"
#include "util.h"
#include <QEvent>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QDebug>

const QString POE_EDIT_THREAD = "https://www.pathofexile.com/forum/edit-thread/";
const QString POE_BUMP_THREAD = "https://www.pathofexile.com/forum/post-reply/";
const QString BUMP_MESSAGE = "[url=https://github.com/Novynn/acquisitionplus/releases]Bumped with Acquisition Plus![/url]";

ShopSubmitter::ShopSubmitter(QNetworkAccessManager *manager)
    : QObject()
    , network(manager)
    , timeout(60000)
{
}

void ShopSubmitter::timerEvent(QTimerEvent *event) {
    int index = event->timerId();
    for (ShopSubmission* shop : submissions) {
        if (shop->timerId == index) {
            // Uh oh, a submission timed out...
            if (shop->currentReply) {
                shop->currentReply->abort();
            }
            break;
        }
    }
    event->accept();
}

void ShopSubmitter::BeginShopSubmission(const QString &threadId, const QString &shopData, bool bumpAfter) {
    if (threadId.isEmpty() || shopData.isEmpty()) {
        emit ShopSubmissionError(threadId, "Invalid shop submitted.");
        return;
    }

    if (submissions.contains(threadId)) {
        emit ShopSubmissionError(threadId, "This shop is already being submitted.");
        return;
    }

    // Setup Queue
    ShopSubmission* submission = new ShopSubmission;
    submission->threadId = threadId;
    submission->shopData = shopData;
    submission->bumpAfter = bumpAfter;
    submissions.insert(threadId, submission);

    submission->timerId = startTimer(GetTimeout());

    // Submit!
    QNetworkRequest request(QUrl(ShopEditUrl(threadId)));
    request.setAttribute((QNetworkRequest::Attribute) SHOP_DATA_THREAD, threadId);

    QNetworkReply *fetched = network->get(request);
    submission->state = SHOP_SUBMISSION_STARTED;
    submission->currentReply = fetched;
    connect(fetched, SIGNAL(finished()), this, SLOT(OnSubmissionPageFinished()));
}

ShopSubmission* ShopSubmitter::ExtractResponse(QNetworkReply *reply) {
    QString threadId = reply->request().attribute((QNetworkRequest::Attribute) SHOP_DATA_THREAD).toString();
    QString error;
    ShopSubmission* submission = 0;

    if (threadId.isEmpty()) {
        error = "An empty shop was submitted.";
    }
    else {
        submission = submissions.value(threadId);

        if (reply->error() != QNetworkReply::NoError) {
            error = "A network error occured: " + reply->errorString();
            if (reply->error() == QNetworkReply::OperationCanceledError) {
                error = "Timed out while submitting to the server.";
            }
        }
        else {
            QByteArray bytes = reply->readAll();
            switch (submission->state) {
            case SHOP_SUBMISSION_STARTED: {
                std::string hash = Util::GetCsrfToken(bytes.toStdString(), "forum_thread");
                if (hash.empty()) {
                    error = "Could not extract CSRF token.";
                    break;
                }

                std::string title = Util::FindTextBetween(bytes.toStdString(),
                                                          "<input type=\"text\" name=\"title\" id=\"title\" value=\"", "\" class=\"textInput\">");
                if (title.empty()) {
                    error = "Could not extract title from forum thread.";
                    break;
                }

                submission->data.insert("forum_thread", QString::fromStdString(hash));
                submission->data.insert("forum_title", QString::fromStdString(title));
            } break;
            case SHOP_SUBMISSION_BUMP_STARTED: {
                std::string hash = Util::GetCsrfToken(bytes.toStdString(), "forum_post");
                if (hash.empty()) {
                    error = "Could not extract CSRF token.";
                    break;
                }
                submission->data.insert("forum_post", QString::fromStdString(hash));
            } break;
            case SHOP_SUBMISSION_BUMPING:
            case SHOP_SUBMISSION_SUBMITTING: {
                std::string forumError = Util::FindTextBetween(bytes.toStdString(), "<ul class=\"errors\"><li>", "</li></ul>");
                if (!forumError.empty()) {
                    error = "The forum responded with an error: " + QString::fromStdString(forumError);
                    break;
                }
            } break;
            case SHOP_SUBMISSION_IDLE:
            case SHOP_SUBMISSION_FINISHED:
            default:
                error = "Internal error";
            }
        }

        if (submission->timerId != -1) {
            killTimer(submission->timerId);
            submission->timerId = -1;
        }
    }

    reply->disconnect();
    reply->deleteLater();

    if (!error.isEmpty()) {
        if (submission) RemoveSubmission(submission);
        emit ShopSubmissionError(threadId, error);
        return 0;
    }
    return submission;
}

void ShopSubmitter::OnSubmissionPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    ShopSubmission* shop = ExtractResponse(reply);
    if (!shop) return;

    SubmitShop(shop);
}

void ShopSubmitter::OnBumpPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    ShopSubmission* shop = ExtractResponse(reply);
    if (!shop) return;

    BumpShop(shop);
}

void ShopSubmitter::SubmitShop(ShopSubmission* submission) {
    QUrlQuery query;
    query.addQueryItem("forum_thread", submission->data.value("forum_thread").toString());
    query.addQueryItem("title", submission->data.value("forum_title").toString());
    query.addQueryItem("content", submission->shopData);
    query.addQueryItem("submit", "Submit");

    submission->timerId = startTimer(GetTimeout());

    QNetworkRequest request(QUrl(ShopEditUrl(submission->threadId)));
    request.setAttribute((QNetworkRequest::Attribute) SHOP_DATA_THREAD, submission->threadId);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = network->post(request, query.query().toUtf8());
    submission->state = SHOP_SUBMISSION_SUBMITTING;
    submission->currentReply = submitted;
    connect(submitted, SIGNAL(finished()), this, SLOT(OnShopSubmitted()));
}

void ShopSubmitter::BumpShop(ShopSubmission *submission) {
    QUrlQuery query;
    query.addQueryItem("forum_post", submission->data.value("forum_post").toString());
    query.addQueryItem("content", BUMP_MESSAGE);
    query.addQueryItem("post_submit", "Submit");

    submission->timerId = startTimer(GetTimeout());

    QNetworkRequest request(QUrl(ShopBumpUrl(submission->threadId)));
    request.setAttribute((QNetworkRequest::Attribute) SHOP_DATA_THREAD, submission->threadId);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = network->post(request, query.query().toUtf8());
    submission->state = SHOP_SUBMISSION_BUMPING;
    submission->currentReply = submitted;
    connect(submitted, SIGNAL(finished()), this, SLOT(OnBumpFinished()));
}

void ShopSubmitter::RemoveSubmission(ShopSubmission *submission) {
    if (submission->timerId != -1) {
        killTimer(submission->timerId);
    }

    submissions.remove(submission->threadId);

    delete submission;
}

void ShopSubmitter::OnShopSubmitted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    ShopSubmission* shop = ExtractResponse(reply);
    if (!shop) return;

    shop->state = SHOP_SUBMISSION_FINISHED;
    shop->currentReply = 0;
    emit ShopSubmitted(shop->threadId);

    if (shop->bumpAfter) {
        BeginShopBump(shop);
    }
    else {
        RemoveSubmission(shop);
    }
}

void ShopSubmitter::OnBumpFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    ShopSubmission* shop = ExtractResponse(reply);
    if (!shop) return;

    shop->state = SHOP_SUBMISSION_FINISHED;
    shop->currentReply = 0;
    emit ShopBumped(shop->threadId);

    RemoveSubmission(shop);
}

void ShopSubmitter::BeginShopBump(ShopSubmission *submission) {
    QNetworkRequest request(QUrl(ShopBumpUrl(submission->threadId)));
    request.setAttribute((QNetworkRequest::Attribute) SHOP_DATA_THREAD, submission->threadId);

    QNetworkReply *fetched = network->get(request);
    submission->state = SHOP_SUBMISSION_BUMP_STARTED;
    submission->currentReply = fetched;
    connect(fetched, SIGNAL(finished()), this, SLOT(OnBumpPageFinished()));
}

QString ShopSubmitter::ShopEditUrl(const QString &threadId) {
    return POE_EDIT_THREAD + threadId;
}

QString ShopSubmitter::ShopBumpUrl(const QString &threadId) {
    return POE_BUMP_THREAD + threadId;
}
