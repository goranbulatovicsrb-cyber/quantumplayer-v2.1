#pragma once
#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QAction>
#include <QComboBox>
#include <QSplitter>
#include <QTabWidget>
#include <QMap>

#include "player/audioengine.h"
#include "player/replaygain.h"
#include "ui/visualizerwidget.h"
#include "ui/equalizerwidget.h"
#include "ui/playlistwidget.h"
#include "ui/librarywidget.h"
#include "ui/nowplayingwidget.h"
#include "ui/thememanager.h"
#include "ui/levelmeterswidget.h"
#include "ui/sleeptimerwidget.h"
#include "ui/clickableslider.h"
#include "ui/systemtrayicon.h"
#include "ui/waveformwidget.h"
#include "ui/lyricswidget.h"
#include "ui/smartplaylistdialog.h"
#include "ui/tageditordialog.h"
#include "ui/lastfmsettingsdialog.h"
#include "network/lyricsfetcher.h"
#include "network/albumartfetcher.h"
#include "network/lastfmscrobbler.h"
#include "library/folderwatcher.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void onPlayPause();
    void onNext();
    void onPrev();
    void onStop();
    void onVolumeChanged(int v);
    void onMute();
    void onShuffle();
    void onRepeat();
    void onSpeedChanged(int v);
    void onCrossfadeChanged(int v);

    void onPositionChanged(qint64 ms);
    void onDurationChanged(qint64 ms);
    void onEngineStateChanged(AudioEngine::State s);
    void onTrackChanged(int index);
    void onLevelChanged(float l, float r);
    void onPlayerError(const QString &msg);

    void onAddFiles();
    void onAddFolder();
    void onWatchFolder();
    void onPlaylistDoubleClick(int index);
    void onAddToPlaylist(const QList<Track> &tracks);
    void onPlayNow(const QList<Track> &tracks);
    void onSmartPlaylist();
    void onEditTag();

    void onEQGainsChanged(const QVector<float> &gains);
    void onThemeChanged(int index);
    void onVisualizerStyleChanged(int index);
    void onSleepTriggered();
    void onReplayGainDone(int trackId, float gainDB);

    void onOpenLastFmSettings();
    void openAbout();

private:
    // Core
    AudioEngine      *m_engine;

    // Widgets
    NowPlayingWidget *m_nowPlaying;
    VisualizerWidget *m_visualizer;
    EqualizerWidget  *m_equalizer;
    PlaylistWidget   *m_playlist;
    LibraryWidget    *m_library;
    LevelMetersWidget*m_levelMeters;
    SleepTimerWidget *m_sleepTimer;
    WaveformWidget   *m_waveform;
    LyricsWidget     *m_lyricsWidget;
    SystemTrayIcon   *m_tray;

    // Controls
    QPushButton     *m_playBtn;
    QPushButton     *m_prevBtn;
    QPushButton     *m_nextBtn;
    QPushButton     *m_stopBtn;
    QPushButton     *m_shuffleBtn;
    QPushButton     *m_repeatBtn;
    QPushButton     *m_muteBtn;
    ClickableSlider *m_seekSlider;
    ClickableSlider *m_volSlider;
    ClickableSlider *m_speedSlider;
    ClickableSlider *m_crossfadeSlider;
    QLabel          *m_posLabel;
    QLabel          *m_durLabel;
    QLabel          *m_speedLabel;
    QLabel          *m_crossfadeLabel;
    QComboBox       *m_themeBox;
    QComboBox       *m_vizStyleBox;
    QCheckBox       *m_replayGainCheck = nullptr;

    // Network
    LyricsFetcher    *m_lyricsFetcher;
    AlbumArtFetcher  *m_artFetcher;
    LastFmScrobbler  *m_scrobbler;

    // Library extras
    FolderWatcher    *m_folderWatcher;

    // ReplayGain cache
    QMap<int,float>   m_replayGainCache;  // trackId → gainDB
    bool              m_replayGainEnabled = false;
    qint64            m_scrobbleThreshold = 0;
    bool              m_scrobbled = false;

    bool m_seeking = false;

    void buildUI();
    void buildMenuBar();
    void buildControlBar(QWidget *parent);
    void applyTheme(ThemeManager::Theme theme);
    void updateVisualizerColors();
    void scanReplayGain(const Track &t);
    void fetchLyrics(const Track &t);
    void fetchAlbumArt(const Track &t);
    void tryScrobble(const Track &t, qint64 posMs);
    QString formatTime(qint64 ms) const;
    void saveSettings();
    void loadSettings();
};
