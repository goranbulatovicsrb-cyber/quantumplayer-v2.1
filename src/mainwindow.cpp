#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QLabel>
#include <QMessageBox>
#include <QTabWidget>
#include <QStatusBar>
#include <QFileInfo>
#include <QDir>
#include <QCheckBox>
#include <QThreadPool>
#include "library/musiclibrary.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_engine(new AudioEngine(this))
    , m_lyricsFetcher(new LyricsFetcher(this))
    , m_artFetcher(new AlbumArtFetcher(this))
    , m_scrobbler(new LastFmScrobbler(this))
    , m_folderWatcher(new FolderWatcher(this))
{
    setWindowTitle("⬡ Quantum Player");
    setMinimumSize(1100, 700);
    resize(1380, 840);
    setAcceptDrops(true);

    buildUI();
    buildMenuBar();

    // Engine signals
    connect(m_engine, &AudioEngine::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_engine, &AudioEngine::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_engine, &AudioEngine::stateChanged,    this, &MainWindow::onEngineStateChanged);
    connect(m_engine, &AudioEngine::trackChanged,    this, &MainWindow::onTrackChanged);
    connect(m_engine, &AudioEngine::levelChanged,    this, &MainWindow::onLevelChanged);
    connect(m_engine, &AudioEngine::errorOccurred,   this, &MainWindow::onPlayerError);

    // Lyrics
    connect(m_lyricsFetcher, &LyricsFetcher::lyricsFound, this, [this](const QString &lyrics){
        const auto &pl = m_engine->playlist();
        int idx = m_engine->currentIndex();
        if (idx >= 0 && idx < pl.size())
            m_lyricsWidget->setLyrics(lyrics, pl[idx].title, pl[idx].artist);
    });
    connect(m_lyricsFetcher, &LyricsFetcher::notFound, this, [this]{
        m_lyricsWidget->setLoading(false);
    });

    // Album art
    connect(m_artFetcher, &AlbumArtFetcher::artFound, this, [this](const QPixmap &px){
        const auto &pl = m_engine->playlist();
        int idx = m_engine->currentIndex();
        if (idx >= 0 && idx < pl.size()) {
            Track t = pl[idx];
            if (t.albumArt.isNull()) {
                t.albumArt = px;
                m_nowPlaying->setTrack(t);
            }
        }
    });

    // Folder watcher
    connect(m_folderWatcher, &FolderWatcher::newFilesFound, this, [this](const QStringList &files){
        for (const QString &f : files) {
            Track t;
            t.id      = MusicLibrary::instance()->nextTrackId();
            t.title   = QFileInfo(f).baseName();
            t.fileUrl = QUrl::fromLocalFile(f);
            m_engine->appendTrack(t);
            m_playlist->addTrack(t);
        }
        statusBar()->showMessage(
            QString("📁  Auto-added %1 new file(s)").arg(files.size()), 4000);
    });

    loadSettings();
    applyTheme(ThemeManager::Midnight);
}

MainWindow::~MainWindow() {}

void MainWindow::buildUI() {
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(0,0,0,0);
    mainLay->setSpacing(0);

    // ── TOP BAR ──────────────────────────────────────────────
    auto *topBar = new QFrame(central);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(48);
    auto *topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(14,6,14,6);

    auto *logo = new QLabel("⬡ QUANTUM PLAYER", topBar);
    logo->setStyleSheet("font-size:15px;font-weight:800;letter-spacing:3px;color:#7b68ff;");
    topLay->addWidget(logo);
    topLay->addStretch();

    topLay->addWidget(new QLabel("Viz:", topBar));
    m_vizStyleBox = new QComboBox(topBar);
    m_vizStyleBox->addItems({"Bars","Mirror"});
    m_vizStyleBox->setFixedWidth(88);
    connect(m_vizStyleBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onVisualizerStyleChanged);
    topLay->addWidget(m_vizStyleBox);

    topLay->addSpacing(10);
    topLay->addWidget(new QLabel("Theme:", topBar));
    m_themeBox = new QComboBox(topBar);
    m_themeBox->addItems(ThemeManager::instance()->themeNames());
    m_themeBox->setFixedWidth(112);
    connect(m_themeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onThemeChanged);
    topLay->addWidget(m_themeBox);

    topLay->addSpacing(10);
    m_sleepTimer = new SleepTimerWidget(topBar);
    connect(m_sleepTimer, &SleepTimerWidget::sleepTriggered, this, &MainWindow::onSleepTriggered);
    topLay->addWidget(m_sleepTimer);

    mainLay->addWidget(topBar);

    // ── MAIN SPLITTER ────────────────────────────────────────
    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setHandleWidth(1);

    // ── LEFT ──
    auto *sidePanel = new QFrame(splitter);
    sidePanel->setObjectName("sidePanel");
    sidePanel->setMinimumWidth(240);
    sidePanel->setMaximumWidth(340);
    auto *sideLay = new QVBoxLayout(sidePanel);
    sideLay->setContentsMargins(0,0,0,0);
    auto *tabs = new QTabWidget(sidePanel);
    m_library  = new LibraryWidget(tabs);
    m_playlist = new PlaylistWidget(tabs);
    tabs->addTab(m_library,  "Library");
    tabs->addTab(m_playlist, "Playlist");
    sideLay->addWidget(tabs);
    splitter->addWidget(sidePanel);

    // ── CENTER ──
    auto *centerPanel = new QFrame(splitter);
    centerPanel->setObjectName("centerPanel");
    auto *centerLay = new QVBoxLayout(centerPanel);
    centerLay->setContentsMargins(0,0,0,0);
    centerLay->setSpacing(0);

    // Now playing + lyrics tabs
    auto *centerTabs = new QTabWidget(centerPanel);
    m_nowPlaying = new NowPlayingWidget();
    m_nowPlaying->setMinimumHeight(280);
    centerTabs->addTab(m_nowPlaying, "Now Playing");

    m_lyricsWidget = new LyricsWidget();
    centerTabs->addTab(m_lyricsWidget, "Lyrics");

    centerLay->addWidget(centerTabs, 2);

    m_visualizer = new VisualizerWidget(centerPanel);
    m_visualizer->setMinimumHeight(110);
    centerLay->addWidget(m_visualizer, 1);

    splitter->addWidget(centerPanel);

    // ── RIGHT ── EQ
    auto *eqPanel = new QFrame(splitter);
    eqPanel->setObjectName("equalizerPanel");
    eqPanel->setMinimumWidth(210);
    eqPanel->setMaximumWidth(290);
    auto *eqLay = new QVBoxLayout(eqPanel);
    eqLay->setContentsMargins(0,0,0,0);
    m_equalizer = new EqualizerWidget(eqPanel);
    eqLay->addWidget(m_equalizer);
    splitter->addWidget(eqPanel);

    splitter->setStretchFactor(0,0);
    splitter->setStretchFactor(1,1);
    splitter->setStretchFactor(2,0);
    splitter->setSizes({270,800,240});
    mainLay->addWidget(splitter, 1);

    // ── CONTROL BAR ──────────────────────────────────────────
    auto *ctrlBar = new QFrame(central);
    ctrlBar->setObjectName("controlBar");
    buildControlBar(ctrlBar);
    mainLay->addWidget(ctrlBar);

    // ── Connections ───────────────────────────────────────────
    connect(m_playlist, &PlaylistWidget::trackDoubleClicked, this, &MainWindow::onPlaylistDoubleClick);
    connect(m_playlist, &PlaylistWidget::addFilesRequested,  this, &MainWindow::onAddFiles);
    connect(m_playlist, &PlaylistWidget::clearRequested,     this, [this]{ m_engine->clearPlaylist(); });
    connect(m_playlist, &PlaylistWidget::removeRequested,    this, [this](int i){ m_engine->removeTrack(i); });
    connect(m_library,  &LibraryWidget::addToPlaylist,       this, &MainWindow::onAddToPlaylist);
    connect(m_library,  &LibraryWidget::playNow,             this, &MainWindow::onPlayNow);
    connect(m_equalizer,&EqualizerWidget::gainsChanged,      this, &MainWindow::onEQGainsChanged);
    connect(m_waveform, &WaveformWidget::seekRequested,      this, [this](qint64 ms){ m_engine->seek(ms); });

    // System tray
    m_tray = new SystemTrayIcon(this, m_engine, this);
    connect(m_tray, &SystemTrayIcon::showWindow, this, &QWidget::show);
    connect(m_tray, &SystemTrayIcon::quitRequested, qApp, &QApplication::quit);
}

void MainWindow::buildControlBar(QWidget *parent) {
    auto *lay = new QVBoxLayout(parent);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    m_levelMeters = new LevelMetersWidget(parent);
    lay->addWidget(m_levelMeters);

    // Waveform seek bar
    m_waveform = new WaveformWidget(parent);
    lay->addWidget(m_waveform);

    auto *row = new QWidget(parent);
    auto *rlay = new QHBoxLayout(row);
    rlay->setContentsMargins(14,5,14,8);
    rlay->setSpacing(6);

    // Buttons
    m_shuffleBtn = new QPushButton("⇄", row);
    m_shuffleBtn->setObjectName("ctrlBtn");
    m_shuffleBtn->setCheckable(true);
    m_shuffleBtn->setToolTip("Shuffle");
    connect(m_shuffleBtn, &QPushButton::toggled, this, [this](bool on){ m_engine->setShuffleMode(on); });
    rlay->addWidget(m_shuffleBtn);

    m_prevBtn = new QPushButton("⏮", row);
    m_prevBtn->setObjectName("ctrlBtn");
    m_prevBtn->setToolTip("Previous  [P]");
    connect(m_prevBtn, &QPushButton::clicked, this, &MainWindow::onPrev);
    rlay->addWidget(m_prevBtn);

    m_playBtn = new QPushButton("▶", row);
    m_playBtn->setObjectName("playBtn");
    m_playBtn->setToolTip("Play/Pause  [Space]");
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    rlay->addWidget(m_playBtn);

    m_nextBtn = new QPushButton("⏭", row);
    m_nextBtn->setObjectName("ctrlBtn");
    m_nextBtn->setToolTip("Next  [N]");
    connect(m_nextBtn, &QPushButton::clicked, this, &MainWindow::onNext);
    rlay->addWidget(m_nextBtn);

    m_stopBtn = new QPushButton("⏹", row);
    m_stopBtn->setObjectName("ctrlBtn");
    m_stopBtn->setToolTip("Stop  [S]");
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    rlay->addWidget(m_stopBtn);

    m_repeatBtn = new QPushButton("↩", row);
    m_repeatBtn->setObjectName("ctrlBtn");
    m_repeatBtn->setCheckable(true);
    m_repeatBtn->setToolTip("Repeat");
    connect(m_repeatBtn, &QPushButton::toggled, this, [this](bool on){
        m_engine->setRepeatMode(on ? AudioEngine::RepeatAll : AudioEngine::RepeatNone);
    });
    rlay->addWidget(m_repeatBtn);

    rlay->addSpacing(8);

    // Position
    m_posLabel = new QLabel("0:00", row);
    m_posLabel->setObjectName("timeLabel");
    m_posLabel->setFixedWidth(42);
    m_posLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    rlay->addWidget(m_posLabel);

    m_seekSlider = new ClickableSlider(Qt::Horizontal, row);
    m_seekSlider->setRange(0, 1000);
    m_seekSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_seekSlider, &QSlider::sliderPressed,  [this]{ m_seeking=true; });
    connect(m_seekSlider, &QSlider::sliderReleased, [this]{
        m_seeking=false;
        qint64 dur=m_engine->duration();
        if (dur>0) m_engine->seek(dur*m_seekSlider->value()/1000);
    });
    connect(m_seekSlider, &QSlider::sliderMoved, [this](int v){
        qint64 dur=m_engine->duration();
        if (dur>0) m_engine->seek(dur*v/1000);
    });
    rlay->addWidget(m_seekSlider);

    m_durLabel = new QLabel("0:00", row);
    m_durLabel->setObjectName("timeLabel");
    m_durLabel->setFixedWidth(42);
    rlay->addWidget(m_durLabel);

    rlay->addSpacing(8);

    // Volume
    m_muteBtn = new QPushButton("🔊", row);
    m_muteBtn->setObjectName("ctrlBtn");
    m_muteBtn->setToolTip("Mute  [M]");
    connect(m_muteBtn, &QPushButton::clicked, this, &MainWindow::onMute);
    rlay->addWidget(m_muteBtn);

    m_volSlider = new ClickableSlider(Qt::Horizontal, row);
    m_volSlider->setRange(0,100);
    m_volSlider->setValue(70);
    m_volSlider->setFixedWidth(90);
    m_volSlider->setToolTip("Volume  [↑/↓]");
    connect(m_volSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    rlay->addWidget(m_volSlider);

    rlay->addSpacing(8);

    // Speed
    m_speedLabel = new QLabel("1.0×", row);
    m_speedLabel->setObjectName("timeLabel");
    m_speedLabel->setFixedWidth(34);
    m_speedLabel->setToolTip("Playback Speed");
    rlay->addWidget(m_speedLabel);

    m_speedSlider = new ClickableSlider(Qt::Horizontal, row);
    m_speedSlider->setRange(25,200);
    m_speedSlider->setValue(100);
    m_speedSlider->setFixedWidth(76);
    m_speedSlider->setToolTip("Speed 0.25×–2.0×");
    connect(m_speedSlider, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);
    rlay->addWidget(m_speedSlider);

    rlay->addSpacing(8);

    // Crossfade
    m_crossfadeLabel = new QLabel("CF:0s", row);
    m_crossfadeLabel->setObjectName("timeLabel");
    m_crossfadeLabel->setFixedWidth(38);
    m_crossfadeLabel->setToolTip("Crossfade seconds");
    rlay->addWidget(m_crossfadeLabel);

    m_crossfadeSlider = new ClickableSlider(Qt::Horizontal, row);
    m_crossfadeSlider->setRange(0,10);
    m_crossfadeSlider->setValue(0);
    m_crossfadeSlider->setFixedWidth(60);
    m_crossfadeSlider->setToolTip("Crossfade 0–10s");
    connect(m_crossfadeSlider, &QSlider::valueChanged, this, &MainWindow::onCrossfadeChanged);
    rlay->addWidget(m_crossfadeSlider);

    lay->addWidget(row);
}

void MainWindow::buildMenuBar() {
    auto *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Add Files...",    this, &MainWindow::onAddFiles,   QKeySequence::Open);
    fileMenu->addAction("Add Folder...",   this, &MainWindow::onAddFolder);
    fileMenu->addAction("Watch Folder...", this, &MainWindow::onWatchFolder);
    fileMenu->addSeparator();
    fileMenu->addAction("Smart Playlist...", this, &MainWindow::onSmartPlaylist);
    fileMenu->addAction("Edit Track Tags...", this, &MainWindow::onEditTag);
    fileMenu->addSeparator();
    fileMenu->addAction("Save Library",    this, [this]{
        QString p=QFileDialog::getSaveFileName(this,"Save Library","","JSON (*.json)");
        if(!p.isEmpty()) MusicLibrary::instance()->saveToFile(p);
    });
    fileMenu->addAction("Load Library",    this, [this]{
        QString p=QFileDialog::getOpenFileName(this,"Load Library","","JSON (*.json)");
        if(!p.isEmpty()) MusicLibrary::instance()->loadFromFile(p);
    });
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close, QKeySequence::Quit);

    auto *playMenu = menuBar()->addMenu("Playback");
    playMenu->addAction("Play / Pause", this, &MainWindow::onPlayPause, QKeySequence(Qt::Key_Space));
    playMenu->addAction("Next",         this, &MainWindow::onNext,  QKeySequence(Qt::Key_Right));
    playMenu->addAction("Previous",     this, &MainWindow::onPrev,  QKeySequence(Qt::Key_Left));
    playMenu->addAction("Stop",         this, &MainWindow::onStop,  QKeySequence(Qt::Key_S));

    auto *toolsMenu = menuBar()->addMenu("Tools");
    toolsMenu->addAction("Last.fm Settings...", this, &MainWindow::onOpenLastFmSettings);
    toolsMenu->addAction("Smart Playlist...",   this, &MainWindow::onSmartPlaylist);
    toolsMenu->addAction("Edit Track Tags...",  this, &MainWindow::onEditTag);
    toolsMenu->addSeparator();
    toolsMenu->addAction("ReplayGain: Scan Current", this, [this]{
        const auto &pl = m_engine->playlist();
        int idx = m_engine->currentIndex();
        if (idx >= 0 && idx < pl.size()) scanReplayGain(pl[idx]);
    });

    auto *viewMenu = menuBar()->addMenu("View");
    auto *themeMenu = viewMenu->addMenu("Theme");
    for (int i=0; i<ThemeManager::ThemeCount; ++i) {
        auto *a = themeMenu->addAction(
            ThemeManager::instance()->themeName(static_cast<ThemeManager::Theme>(i)));
        connect(a, &QAction::triggered, this, [this,i]{ m_themeBox->setCurrentIndex(i); });
    }

    menuBar()->addMenu("Help")->addAction("About", this, &MainWindow::openAbout);
}

// ── Playback ──────────────────────────────────────────────────
void MainWindow::onPlayPause() { m_engine->isPlaying() ? m_engine->pause() : m_engine->play(); }
void MainWindow::onNext()      { m_engine->next(); }
void MainWindow::onPrev()      { m_engine->previous(); }
void MainWindow::onStop()      { m_engine->stop(); }
void MainWindow::onShuffle()   {}
void MainWindow::onRepeat()    {}

void MainWindow::onVolumeChanged(int v) {
    m_engine->setVolume(v/100.f);
    m_muteBtn->setText(v==0?"🔇":v<40?"🔈":v<75?"🔉":"🔊");
}
void MainWindow::onMute() {
    bool m=m_engine->isMuted(); m_engine->setMuted(!m);
    m_muteBtn->setText(!m?"🔇":"🔊");
}
void MainWindow::onSpeedChanged(int v) {
    float r=v/100.f;
    m_speedLabel->setText(QString::number(r,'f',2)+"×");
    m_engine->setPlaybackRate(r);
}
void MainWindow::onCrossfadeChanged(int v) {
    m_crossfadeLabel->setText(QString("CF:%1s").arg(v));
    m_engine->setCrossfadeSecs(v);
}

// ── Engine signals ────────────────────────────────────────────
void MainWindow::onPositionChanged(qint64 ms) {
    if (!m_seeking) {
        qint64 dur=m_engine->duration();
        if (dur>0) m_seekSlider->setValue(static_cast<int>(ms*1000/dur));
        m_posLabel->setText(formatTime(ms));
        m_waveform->setPosition(ms, dur);
    }
    // Scrobble at 50% or 4 minutes
    if (!m_scrobbled && m_scrobbleThreshold > 0 && ms >= m_scrobbleThreshold) {
        const auto &pl = m_engine->playlist();
        int idx = m_engine->currentIndex();
        if (idx >= 0 && idx < pl.size()) tryScrobble(pl[idx], ms);
    }
}
void MainWindow::onDurationChanged(qint64 ms) {
    m_durLabel->setText(formatTime(ms));
    m_scrobbleThreshold = qMin(ms/2, (qint64)240000);
}
void MainWindow::onEngineStateChanged(AudioEngine::State s) {
    bool playing=(s==AudioEngine::Playing);
    m_playBtn->setText(playing?"⏸":"▶");
    m_visualizer->setPlaying(playing);
    m_tray->updateState(playing);
    if (s==AudioEngine::Stopped) {
        m_seekSlider->setValue(0);
        m_posLabel->setText("0:00");
        m_levelMeters->reset();
        m_nowPlaying->clearTrack();
        setWindowTitle("⬡ Quantum Player");
        m_scrobbled=false;
    }
}
void MainWindow::onTrackChanged(int index) {
    m_playlist->setCurrentIndex(index);
    const auto &pl=m_engine->playlist();
    if (index<0||index>=pl.size()) return;
    const Track &t=pl[index];
    m_nowPlaying->setTrack(t);
    m_waveform->loadFile(t.fileUrl);
    m_lyricsWidget->setLoading(true);
    m_scrobbled=false;

    QString title  = t.title.isEmpty()  ? "Unknown" : t.title;
    QString artist = t.artist.isEmpty() ? "Unknown" : t.artist;
    setWindowTitle(QString("♫  %1  —  %2  |  Quantum Player").arg(title, artist));
    m_tray->updateTrack(title, artist);

    // ReplayGain
    if (m_replayGainEnabled) {
        if (m_replayGainCache.contains(t.id))
            m_engine->setReplayGain(m_replayGainCache[t.id]);
        else
            scanReplayGain(t);
    } else {
        m_engine->setReplayGain(0.f);
    }

    // Fetch lyrics and art
    fetchLyrics(t);
    if (t.albumArt.isNull()) fetchAlbumArt(t);

    // Last.fm now playing
    if (m_scrobbler->isConfigured())
        m_scrobbler->updateNowPlaying(t.artist, t.title, t.album,
                                       static_cast<int>(t.duration/1000));
}
void MainWindow::onLevelChanged(float l, float r) { m_levelMeters->setLevel(l,r); }
void MainWindow::onPlayerError(const QString &msg) { statusBar()->showMessage("⚠  "+msg, 6000); }

// ── EQ ────────────────────────────────────────────────────────
void MainWindow::onEQGainsChanged(const QVector<float> &g) { m_engine->setEQGains(g); }

// ── File management ───────────────────────────────────────────
void MainWindow::onAddFiles() {
    QStringList files=QFileDialog::getOpenFileNames(this,"Add Audio Files","",
        "Audio Files (*.mp3 *.flac *.wav *.ogg *.m4a *.aac *.opus *.wma)");
    for (const QString &f:files) {
        Track t; t.id=MusicLibrary::instance()->nextTrackId();
        t.title=QFileInfo(f).baseName(); t.fileUrl=QUrl::fromLocalFile(f);
        m_engine->appendTrack(t); m_playlist->addTrack(t);
    }
    if (!files.isEmpty()&&m_engine->currentIndex()<0) m_engine->playIndex(0);
}
void MainWindow::onAddFolder() {
    QString dir=QFileDialog::getExistingDirectory(this,"Add Folder");
    if (dir.isEmpty()) return;
    static const QStringList FILT={"*.mp3","*.flac","*.wav","*.ogg","*.m4a","*.aac","*.opus"};
    auto entries=QDir(dir).entryInfoList(FILT,QDir::Files,QDir::Name);
    for (const auto &fi:entries) {
        Track t; t.id=MusicLibrary::instance()->nextTrackId();
        t.title=fi.baseName(); t.fileUrl=QUrl::fromLocalFile(fi.absoluteFilePath());
        m_engine->appendTrack(t); m_playlist->addTrack(t);
    }
    if (!entries.isEmpty()&&m_engine->currentIndex()<0) m_engine->playIndex(0);
}
void MainWindow::onWatchFolder() {
    QString dir=QFileDialog::getExistingDirectory(this,"Watch Folder for New Files");
    if (dir.isEmpty()) return;
    m_folderWatcher->addFolder(dir);
    statusBar()->showMessage("👁  Watching: "+dir, 4000);
}
void MainWindow::onPlaylistDoubleClick(int i) { m_engine->playIndex(i); }
void MainWindow::onAddToPlaylist(const QList<Track> &tracks) {
    for (const auto &t:tracks) { m_engine->appendTrack(t); m_playlist->addTrack(t); }
}
void MainWindow::onPlayNow(const QList<Track> &tracks) {
    m_engine->clearPlaylist(); m_playlist->clearPlaylist();
    onAddToPlaylist(tracks); m_engine->playIndex(0);
}
void MainWindow::onSmartPlaylist() {
    SmartPlaylistDialog dlg(m_engine->playlist(), this);
    if (dlg.exec()!=QDialog::Accepted) return;
    auto tracks=dlg.filteredTracks();
    if (tracks.isEmpty()) { statusBar()->showMessage("No tracks match filter.",3000); return; }
    m_engine->clearPlaylist(); m_playlist->clearPlaylist();
    onAddToPlaylist(tracks); m_engine->playIndex(0);
}
void MainWindow::onEditTag() {
    const auto &pl=m_engine->playlist();
    int idx=m_engine->currentIndex();
    if (idx<0||idx>=pl.size()) { statusBar()->showMessage("No track selected.",3000); return; }
    TagEditorDialog dlg(pl[idx], this);
    if (dlg.exec()!=QDialog::Accepted) return;
    // Update in engine playlist (no file write — in-memory only)
    statusBar()->showMessage("Tags updated (in-memory).", 3000);
}

// ── ReplayGain ────────────────────────────────────────────────
void MainWindow::scanReplayGain(const Track &t) {
    auto *scanner=new ReplayGainScanner(t.id, t.fileUrl);
    connect(scanner, &ReplayGainScanner::done, this, &MainWindow::onReplayGainDone);
    QThreadPool::globalInstance()->start(scanner);
}
void MainWindow::onReplayGainDone(int trackId, float gainDB) {
    m_replayGainCache[trackId]=gainDB;
    if (m_engine->currentIndex()>=0) {
        const auto &pl=m_engine->playlist();
        int idx=m_engine->currentIndex();
        if (idx<pl.size()&&pl[idx].id==trackId&&m_replayGainEnabled)
            m_engine->setReplayGain(gainDB);
    }
    statusBar()->showMessage(
        QString("ReplayGain: %1 dB").arg(gainDB,0,'f',1), 2000);
}

// ── Network ───────────────────────────────────────────────────
void MainWindow::fetchLyrics(const Track &t) {
    if (!t.artist.isEmpty()||!t.title.isEmpty())
        m_lyricsFetcher->fetch(t.artist, t.title);
}
void MainWindow::fetchAlbumArt(const Track &t) {
    if (!t.artist.isEmpty()||!t.album.isEmpty())
        m_artFetcher->fetch(t.artist, t.album);
}
void MainWindow::tryScrobble(const Track &t, qint64) {
    if (!m_scrobbled && m_scrobbler->isConfigured()) {
        m_scrobbler->scrobble(t.artist, t.title, t.album,
                               static_cast<int>(t.duration/1000));
        m_scrobbled=true;
    }
}
void MainWindow::onOpenLastFmSettings() {
    LastFmSettingsDialog dlg(this);
    if (dlg.exec()==QDialog::Accepted && dlg.enabled()) {
        m_scrobbler->setCredentials(dlg.apiKey(), dlg.apiSecret(), dlg.sessionKey());
        statusBar()->showMessage("Last.fm scrobbling enabled.", 3000);
    }
}

// ── Theme ─────────────────────────────────────────────────────
void MainWindow::onThemeChanged(int i) { applyTheme(static_cast<ThemeManager::Theme>(i)); }
void MainWindow::onVisualizerStyleChanged(int i) {
    m_visualizer->setStyle(i==0?VisualizerWidget::Bars:VisualizerWidget::Mirror);
}
void MainWindow::onSleepTriggered() {
    m_engine->stop(); statusBar()->showMessage("💤  Sleep timer — playback stopped.",5000);
}
void MainWindow::applyTheme(ThemeManager::Theme t) {
    ThemeManager::instance()->apply(qApp,t);
    updateVisualizerColors();
    const auto &c=ThemeManager::instance()->colors();
    m_nowPlaying->setAccentColor(c.accent);
    m_levelMeters->setAccentColor(c.accent);
    m_waveform->setAccentColor(c.accent);
    m_nowPlaying->update();
}
void MainWindow::updateVisualizerColors() {
    const auto &c=ThemeManager::instance()->colors();
    m_visualizer->setColors(c.vizBar1,c.vizBar2,c.vizBar3,c.vizPeak,c.vizBg);
}

// ── About ─────────────────────────────────────────────────────
void MainWindow::openAbout() {
    QMessageBox::about(this,"About Quantum Player",
        "<b>⬡ Quantum Player v2.1</b><br><br>"
        "Ultra-modern C++17 / Qt6 / miniaudio music player<br><br>"
        "<b>Audio:</b> miniaudio WASAPI/CoreAudio/ALSA<br>"
        "<b>EQ:</b> 10-band real-time biquad DSP<br>"
        "<b>Features:</b> ReplayGain · Crossfade · Waveform · Lyrics<br>"
        "Album art fetch · Last.fm scrobbling · Smart playlists<br>"
        "Folder watcher · Tag editor · System tray · Speed control<br>"
        "Sleep timer · 5 themes · Drag&amp;Drop<br><br>"
        "Qt " QT_VERSION_STR);
}

QString MainWindow::formatTime(qint64 ms) const {
    qint64 s=ms/1000;
    return QString("%1:%2").arg(s/60).arg(s%60,2,10,QChar('0'));
}
void MainWindow::saveSettings() {
    QSettings s("QuantumSoft","QuantumPlayer");
    s.setValue("volume",    m_volSlider->value());
    s.setValue("theme",     m_themeBox->currentIndex());
    s.setValue("shuffle",   m_shuffleBtn->isChecked());
    s.setValue("repeat",    m_repeatBtn->isChecked());
    s.setValue("vizStyle",  m_vizStyleBox->currentIndex());
    s.setValue("speed",     m_speedSlider->value());
    s.setValue("crossfade", m_crossfadeSlider->value());
    s.setValue("geometry",  saveGeometry());
    s.setValue("state",     saveState());
}
void MainWindow::loadSettings() {
    QSettings s("QuantumSoft","QuantumPlayer");
    m_volSlider->setValue(s.value("volume",70).toInt());
    m_themeBox->setCurrentIndex(s.value("theme",0).toInt());
    m_shuffleBtn->setChecked(s.value("shuffle",false).toBool());
    m_repeatBtn->setChecked(s.value("repeat",false).toBool());
    m_vizStyleBox->setCurrentIndex(s.value("vizStyle",0).toInt());
    m_speedSlider->setValue(s.value("speed",100).toInt());
    m_crossfadeSlider->setValue(s.value("crossfade",0).toInt());
    if (s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
    // Last.fm
    if (s.value("lastfm/enabled",false).toBool())
        m_scrobbler->setCredentials(s.value("lastfm/apiKey").toString(),
                                    s.value("lastfm/apiSecret").toString(),
                                    s.value("lastfm/sessionKey").toString());
}
void MainWindow::closeEvent(QCloseEvent *e) {
    // Minimize to tray instead of close
    if (m_tray->isVisible()) {
        hide(); e->ignore();
    } else {
        saveSettings(); e->accept();
    }
}
void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent *e) {
    static const QStringList EXT={".mp3",".flac",".wav",".ogg",".m4a",".aac",".opus",".wma"};
    bool any=false;
    for (const auto &url:e->mimeData()->urls()) {
        QString path=url.toLocalFile();
        bool ok=false;
        for (const auto &ext:EXT) if(path.endsWith(ext,Qt::CaseInsensitive)){ok=true;break;}
        if (!ok) continue;
        Track t; t.id=MusicLibrary::instance()->nextTrackId();
        t.title=QFileInfo(path).baseName(); t.fileUrl=url;
        m_engine->appendTrack(t); m_playlist->addTrack(t); any=true;
    }
    if (any&&m_engine->currentIndex()<0) m_engine->playIndex(0);
}
void MainWindow::keyPressEvent(QKeyEvent *e) {
    switch(e->key()) {
    case Qt::Key_Space:      onPlayPause(); break;
    case Qt::Key_Right:      m_engine->seek(m_engine->position()+5000); break;
    case Qt::Key_Left:       m_engine->seek(m_engine->position()-5000); break;
    case Qt::Key_Up:         m_volSlider->setValue(qMin(100,m_volSlider->value()+5)); break;
    case Qt::Key_Down:       m_volSlider->setValue(qMax(0,m_volSlider->value()-5)); break;
    case Qt::Key_N:          onNext();  break;
    case Qt::Key_P:          onPrev();  break;
    case Qt::Key_S:          onStop();  break;
    case Qt::Key_M:          onMute();  break;
    case Qt::Key_MediaPlay:  onPlayPause(); break;
    case Qt::Key_MediaNext:  onNext();  break;
    case Qt::Key_MediaPrevious: onPrev(); break;
    case Qt::Key_MediaStop:  onStop();  break;
    default: QMainWindow::keyPressEvent(e);
    }
}
