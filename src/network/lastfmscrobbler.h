#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QDateTime>

class LastFmScrobbler : public QObject {
    Q_OBJECT
public:
    explicit LastFmScrobbler(QObject *parent = nullptr);

    bool isConfigured() const;
    void setCredentials(const QString &apiKey, const QString &apiSecret,
                        const QString &sessionKey);

    void updateNowPlaying(const QString &artist, const QString &title,
                          const QString &album, int durationSecs);
    void scrobble(const QString &artist, const QString &title,
                  const QString &album, int durationSecs);

    const QString& sessionKey() const { return m_sessionKey; }

private:
    QNetworkAccessManager *m_nam;
    QString m_apiKey;
    QString m_apiSecret;
    QString m_sessionKey;

    QString sign(const QMap<QString,QString> &params) const;
    void post(QMap<QString,QString> params);
};
