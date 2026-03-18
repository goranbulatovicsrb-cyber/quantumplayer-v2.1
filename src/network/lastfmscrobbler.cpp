#include "lastfmscrobbler.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCryptographicHash>
#include <QUrlQuery>
#include <QUrl>
#include <QDateTime>

LastFmScrobbler::LastFmScrobbler(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

bool LastFmScrobbler::isConfigured() const {
    return !m_apiKey.isEmpty() && !m_sessionKey.isEmpty();
}

void LastFmScrobbler::setCredentials(const QString &ak, const QString &as, const QString &sk) {
    m_apiKey = ak; m_apiSecret = as; m_sessionKey = sk;
}

QString LastFmScrobbler::sign(const QMap<QString,QString> &params) const {
    QString str;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        str += it.key() + it.value();
    str += m_apiSecret;
    return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();
}

void LastFmScrobbler::post(QMap<QString,QString> params) {
    if (!isConfigured()) return;
    params["api_key"] = m_apiKey;
    params["sk"]      = m_sessionKey;
    params["api_sig"] = sign(params);
    params["format"]  = "json";

    QUrlQuery q;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        q.addQueryItem(it.key(), it.value());

    QNetworkRequest req(QUrl("https://ws.audioscrobbler.com/2.0/"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    auto *reply = m_nam->post(req, q.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void LastFmScrobbler::updateNowPlaying(const QString &artist, const QString &title,
                                        const QString &album, int dur) {
    post({{"method","track.updateNowPlaying"},
          {"artist", artist}, {"track", title},
          {"album",  album},  {"duration", QString::number(dur)}});
}

void LastFmScrobbler::scrobble(const QString &artist, const QString &title,
                                const QString &album, int dur) {
    post({{"method",    "track.scrobble"},
          {"artist[0]", artist}, {"track[0]", title},
          {"album[0]",  album},  {"duration[0]", QString::number(dur)},
          {"timestamp[0]", QString::number(QDateTime::currentSecsSinceEpoch())}});
}
