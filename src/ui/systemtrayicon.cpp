#include "systemtrayicon.h"
#include "../player/audioengine.h"
#include <QApplication>

SystemTrayIcon::SystemTrayIcon(QWidget *mainWindow, AudioEngine *engine, QObject *parent)
    : QSystemTrayIcon(parent), m_engine(engine)
{
    // Use a simple icon (colored square as placeholder)
    QPixmap pm(32, 32);
    pm.fill(QColor(0x7b, 0x68, 0xff));
    setIcon(QIcon(pm));
    setToolTip("Quantum Player");

    m_menu = new QMenu(mainWindow);

    m_trackInfo = m_menu->addAction("No track playing");
    m_trackInfo->setEnabled(false);
    m_menu->addSeparator();

    m_prev      = m_menu->addAction("⏮  Previous");
    m_playPause = m_menu->addAction("▶  Play");
    m_next      = m_menu->addAction("⏭  Next");
    m_menu->addSeparator();

    auto *showAct = m_menu->addAction("🪟  Show Window");
    auto *quitAct = m_menu->addAction("✕  Quit");

    setContextMenu(m_menu);

    connect(m_prev,      &QAction::triggered, engine, &AudioEngine::previous);
    connect(m_next,      &QAction::triggered, engine, &AudioEngine::next);
    connect(m_playPause, &QAction::triggered, this, [this]{
        m_engine->isPlaying() ? m_engine->pause() : m_engine->play();
    });
    connect(showAct, &QAction::triggered, this, &SystemTrayIcon::showWindow);
    connect(quitAct, &QAction::triggered, this, &SystemTrayIcon::quitRequested);

    // Double-click tray icon → show window
    connect(this, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r){
        if (r == QSystemTrayIcon::DoubleClick) emit showWindow();
    });

    show();
}

void SystemTrayIcon::updateTrack(const QString &title, const QString &artist) {
    QString info = artist.isEmpty() ? title : QString("%1 — %2").arg(title, artist);
    m_trackInfo->setText(info.isEmpty() ? "No track playing" : info);
    setToolTip("♫  " + info);
    showMessage("Now Playing", info, QSystemTrayIcon::NoIcon, 2000);
}

void SystemTrayIcon::updateState(bool playing) {
    m_playPause->setText(playing ? "⏸  Pause" : "▶  Play");
}
