#pragma once
#include <QWidget>
#include <QVector>
#include <QThread>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>
#include <QUrl>

class WaveformWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget *parent = nullptr);
    void loadFile(const QUrl &url);
    void setPosition(qint64 ms, qint64 durationMs);
    void setAccentColor(const QColor &c);
    QSize minimumSizeHint() const override { return {200, 32}; }

signals:
    void seekRequested(qint64 ms);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private slots:
    void onSamplesReady(const QVector<float> &samples);

private:
    QVector<float> m_peaks;      // normalised 0-1 per pixel column
    qint64         m_posMs   = 0;
    qint64         m_durMs   = 0;
    QColor         m_accent  {0x7b,0x68,0xff};
    bool           m_loading = false;
    QString        m_currentFile;

    void seekFromX(int x);
};

// Background scanner
class WaveformScanner : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit WaveformScanner(const QString &path, int targetW)
        : m_path(path), m_w(targetW) { setAutoDelete(true); }
    void run() override;
signals:
    void done(const QVector<float> &peaks);
private:
    QString m_path;
    int     m_w;
};
