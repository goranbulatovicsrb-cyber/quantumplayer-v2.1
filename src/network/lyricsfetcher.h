#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class LyricsFetcher : public QObject {
    Q_OBJECT
public:
    explicit LyricsFetcher(QObject *parent = nullptr);
    void fetch(const QString &artist, const QString &title);

signals:
    void lyricsFound(const QString &lyrics);
    void notFound();

private slots:
    void onReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    QString m_artist, m_title;
};
