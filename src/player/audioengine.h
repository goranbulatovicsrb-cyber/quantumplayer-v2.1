#pragma once
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QList>
#include <QVector>
#include <QElapsedTimer>
#include <atomic>
#include "../dsp/biquadfilter.h"
#include "../library/track.h"

// Forward declare miniaudio types to avoid including the huge header in .h
typedef struct ma_device   ma_device;
typedef struct ma_decoder  ma_decoder;

// ─── AudioEngine using miniaudio ─────────────────────────────
// Real-time audio thread: ma_device callback → DSP EQ → speakers
class AudioEngine : public QObject {
    Q_OBJECT
public:
    enum RepeatMode { RepeatNone=0, RepeatOne, RepeatAll };
    enum State      { Stopped=0, Playing, Paused };

    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    void setPlaylist(const QList<Track> &tracks);
    void appendTrack(const Track &t);
    void clearPlaylist();
    void removeTrack(int i);
    void moveTrack(int from, int to);
    const QList<Track>& playlist() const { return m_pl; }

    void playIndex(int i);
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(qint64 ms);

    // REAL DSP EQ — applied per-sample in audio callback
    void setEQGains(const QVector<float> &gainsDB);

    // ReplayGain — set per-track gain offset (dB)
    void setReplayGain(float gainDB);

    // Crossfade duration (0 = disabled)
    void setCrossfadeSecs(int secs);

    void  setVolume(float v);
    float volume()       const { return m_volume; }
    void  setMuted(bool m);
    bool  isMuted()      const { return m_muted; }
    void  setShuffleMode(bool s);
    bool  shuffleMode()  const { return m_shuffle; }
    void  setRepeatMode(RepeatMode r) { m_repeat = r; }
    RepeatMode repeatMode() const     { return m_repeat; }
    void  setPlaybackRate(float r)    { m_rate = qBound(0.25f,r,3.f); }
    float playbackRate() const        { return m_rate; }

    State  state()        const { return m_state; }
    bool   isPlaying()    const { return m_state == Playing; }
    bool   isPaused()     const { return m_state == Paused;  }
    bool   isStopped()    const { return m_state == Stopped; }
    int    currentIndex() const { return m_idx; }
    qint64 position()     const;
    qint64 duration()     const;

    // Called from miniaudio audio thread — DO NOT call directly
    void fillBuffer(float *output, unsigned int frames);

signals:
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void stateChanged(AudioEngine::State s);
    void trackChanged(int index);
    void playlistChanged();
    void errorOccurred(const QString &msg);
    void levelChanged(float l, float r);

private slots:
    void onPosTick();
    void onLevelTick();

private:
    static const int    SR    = 44100;
    static const int    CH    = 2;
    static const int    BANDS = 10;
    static const double FREQS[BANDS];

    // miniaudio objects
    ma_device  *m_device  = nullptr;
    ma_decoder *m_decoder = nullptr;
    QMutex      m_decoderMtx;

    // EQ state — accessed from audio thread, protected by mutex
    QMutex        m_eqMtx;
    QVector<float> m_gains;
    BiquadFilter   m_eqFilters[BANDS][CH];
    bool           m_eqFlat = true;

    // Playback state
    std::atomic<bool>    m_playing  {false};
    std::atomic<bool>    m_paused   {false};
    std::atomic<float>   m_vol      {0.70f};
    std::atomic<bool>    m_mute     {false};
    std::atomic<bool>    m_trackEnd {false};
    std::atomic<float>   m_replayGain {0.f};   // dB offset
    std::atomic<float>   m_crossGain  {1.f};   // 0→1 crossfade multiplier
    int                  m_crossfadeSecs = 0;
    std::atomic<uint64_t> m_framesPlayed {0};
    std::atomic<uint64_t> m_totalFrames  {0};

    QList<Track>   m_pl;
    int            m_idx     = -1;
    State          m_state   = Stopped;
    bool           m_shuffle = false;
    RepeatMode     m_repeat  = RepeatNone;
    QList<int>     m_order;
    float          m_volume  = 0.70f;
    bool           m_muted   = false;
    float          m_rate    = 1.0f;

    QTimer  *m_posTimer = nullptr;
    QTimer  *m_lvlTimer = nullptr;
    QTimer  *m_endTimer = nullptr;

    bool initDevice();
    void closeDevice();
    bool openDecoder(const QString &path);
    void closeDecoder();
    void rebuildEQ();
    void rebuildOrder();
    int  nextIdx() const;
    int  prevIdx() const;
    void scheduleEndCheck();
};
