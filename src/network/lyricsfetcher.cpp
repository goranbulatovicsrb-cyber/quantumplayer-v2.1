#include "lyricsfetcher.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

LyricsFetcher::LyricsFetcher(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished,
            this,  &LyricsFetcher::onReply);
}

void LyricsFetcher::fetch(const QString &artist, const QString &title) {
    if (artist.isEmpty() || title.isEmpty()) { emit notFound(); return; }
    m_artist = artist; m_title = title;
    // lyrics.ovh free API
    QString url = QString("https://api.lyrics.ovh/v1/%1/%2")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(artist)),
             QString::fromUtf8(QUrl::toPercentEncoding(title)));
    m_nam->get(QNetworkRequest(QUrl(url)));
}

void LyricsFetcher::onReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) { emit notFound(); return; }
    auto doc = QJsonDocument::fromJson(reply->readAll());
    QString lyrics = doc.object()["lyrics"].toString().trimmed();
    if (lyrics.isEmpty()) emit notFound();
    else                  emit lyricsFound(lyrics);
}
