#pragma once
#include <QObject>
#include <QRunnable>
#include <QUrl>

// Background RMS scan → returns gain adjustment in dB
class ReplayGainScanner : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit ReplayGainScanner(int trackId, const QUrl &url)
        : m_id(trackId), m_url(url) { setAutoDelete(true); }
    void run() override;
signals:
    void done(int trackId, float gainDB);
private:
    int  m_id;
    QUrl m_url;
};
