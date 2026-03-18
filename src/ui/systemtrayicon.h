#pragma once
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class AudioEngine;

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTrayIcon(QWidget *mainWindow, AudioEngine *engine, QObject *parent = nullptr);
    void updateTrack(const QString &title, const QString &artist);
    void updateState(bool playing);

signals:
    void showWindow();
    void quitRequested();

private:
    QMenu   *m_menu;
    QAction *m_playPause;
    QAction *m_next;
    QAction *m_prev;
    QAction *m_trackInfo;
    AudioEngine *m_engine;
};
