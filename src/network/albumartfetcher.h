#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>

class AlbumArtFetcher : public QObject {
    Q_OBJECT
public:
    explicit AlbumArtFetcher(QObject *parent = nullptr);
    void fetch(const QString &artist, const QString &album);

signals:
    void artFound(const QPixmap &pixmap);
    void notFound();

private slots:
    void onMBReply(QNetworkReply *reply);
    void onArtReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    enum Step { StepMB, StepArt } m_step = StepMB;
};
