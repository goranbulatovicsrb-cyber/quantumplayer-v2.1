#include "albumartfetcher.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>

AlbumArtFetcher::AlbumArtFetcher(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void AlbumArtFetcher::fetch(const QString &artist, const QString &album) {
    if (artist.isEmpty() && album.isEmpty()) { emit notFound(); return; }
    m_step = StepMB;
    // Step 1: MusicBrainz search for release MBID
    QString query = QString("artist:%1 AND release:%2")
        .arg(artist.isEmpty() ? "*" : artist,
             album.isEmpty()  ? "*" : album);
    QUrl url("https://musicbrainz.org/ws/2/release/");
    QUrlQuery q;
    q.addQueryItem("query", query);
    q.addQueryItem("limit", "1");
    q.addQueryItem("fmt",   "json");
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "QuantumPlayer/2.0 (contact@example.com)");
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{ onMBReply(reply); });
}

void AlbumArtFetcher::onMBReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) { emit notFound(); return; }
    auto doc  = QJsonDocument::fromJson(reply->readAll());
    auto releases = doc.object()["releases"].toArray();
    if (releases.isEmpty()) { emit notFound(); return; }
    QString mbid = releases[0].toObject()["id"].toString();
    if (mbid.isEmpty()) { emit notFound(); return; }

    // Step 2: Cover Art Archive
    m_step = StepArt;
    QUrl artUrl(QString("https://coverartarchive.org/release/%1/front-250").arg(mbid));
    auto *artReply = m_nam->get(QNetworkRequest(artUrl));
    connect(artReply, &QNetworkReply::finished, this, [this, artReply]{ onArtReply(artReply); });
}

void AlbumArtFetcher::onArtReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) { emit notFound(); return; }
    QPixmap px;
    if (px.loadFromData(reply->readAll())) emit artFound(px);
    else emit notFound();
}
