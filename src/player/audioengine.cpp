#include "audioengine.h"

// Include miniaudio fully here (only in this .cpp)
#include "../../third_party/miniaudio.h"

#include <QRandomGenerator>
#include <QFileInfo>
#include <cmath>
#include <algorithm>
#include <cstring>

// ── EQ frequency table ────────────────────────────────────────
const double AudioEngine::FREQS[BANDS] =
    {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

// ── miniaudio callback (runs in real-time audio thread) ───────
static void ma_callback(ma_device *dev, void *output,
                        const void*, ma_uint32 frameCount)
{
    auto *eng = static_cast<AudioEngine*>(dev->pUserData);
    if (eng) eng->fillBuffer(static_cast<float*>(output), frameCount);
}

// ── fillBuffer: called from audio thread ─────────────────────
void AudioEngine::fillBuffer(float *out, unsigned int frames) {
    if (!m_playing || m_paused) {
        std::memset(out, 0, frames * CH * sizeof(float));
        return;
    }

    ma_uint64 framesRead = 0;
    {
        QMutexLocker lk(&m_decoderMtx);
        if (!m_decoder) {
            std::memset(out, 0, frames * CH * sizeof(float));
            return;
        }
        ma_decoder_read_pcm_frames(m_decoder, out, frames, &framesRead);
    }

    // Zero-fill if decoder gave us less than requested (end of file)
    if (framesRead < frames) {
        std::memset(out + framesRead * CH, 0,
                    (frames - framesRead) * CH * sizeof(float));
        m_trackEnd.store(true);
    }

    // Apply volume + ReplayGain
    float rgDB  = m_replayGain.load();
    float rgMul = (std::abs(rgDB) > 0.01f)
                  ? static_cast<float>(std::pow(10.0, rgDB / 20.0)) : 1.f;
    float vol   = (m_mute ? 0.f : m_vol.load()) * rgMul;
    vol = qBound(0.f, vol, 2.f);

    // Apply EQ per sample
    {
        QMutexLocker lk(&m_eqMtx);
        for (unsigned int f = 0; f < framesRead; ++f) {
            for (int c = 0; c < CH; ++c) {
                double s = out[f * CH + c];
                if (!m_eqFlat) {
                    for (int b = 0; b < BANDS; ++b)
                        s = m_eqFilters[b][c].process(s);
                    // soft clip
                    s /= (1.0 + std::abs(s) * 0.07);
                }
                out[f * CH + c] = static_cast<float>(
                    qBound(-1.0, s * static_cast<double>(vol), 1.0));
            }
        }
    }

    m_framesPlayed.fetch_add(framesRead);
}

// ── Constructor ───────────────────────────────────────────────
AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
    , m_gains(BANDS, 0.f)
{
    m_device  = new ma_device;
    m_decoder = nullptr;

    rebuildEQ();

    m_posTimer = new QTimer(this);
    m_posTimer->setInterval(80);
    connect(m_posTimer, &QTimer::timeout, this, &AudioEngine::onPosTick);

    m_lvlTimer = new QTimer(this);
    m_lvlTimer->setInterval(55);
    connect(m_lvlTimer, &QTimer::timeout, this, &AudioEngine::onLevelTick);

    // Check for track end
    m_endTimer = new QTimer(this);
    m_endTimer->setInterval(200);
    connect(m_endTimer, &QTimer::timeout, this, [this]{
        if (m_trackEnd.exchange(false)) {
            if      (m_repeat == RepeatOne) playIndex(m_idx);
            else    next();
        }
    });

    if (!initDevice()) {
        emit errorOccurred("Failed to initialize audio device");
    }
}

AudioEngine::~AudioEngine() {
    closeDevice();
    closeDecoder();
    delete m_device;
}

// ── Device init/close ─────────────────────────────────────────
bool AudioEngine::initDevice() {
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = CH;
    cfg.sampleRate        = SR;
    cfg.dataCallback      = ma_callback;
    cfg.pUserData         = this;

    if (ma_device_init(nullptr, &cfg, m_device) != MA_SUCCESS) {
        return false;
    }
    if (ma_device_start(m_device) != MA_SUCCESS) {
        ma_device_uninit(m_device);
        return false;
    }
    return true;
}

void AudioEngine::closeDevice() {
    if (m_device) {
        ma_device_stop(m_device);
        ma_device_uninit(m_device);
    }
}

bool AudioEngine::openDecoder(const QString &path) {
    closeDecoder();

    auto *dec = new ma_decoder;
    ma_decoder_config cfg = ma_decoder_config_init(
        ma_format_f32, static_cast<ma_uint32>(CH),
        static_cast<ma_uint32>(SR));

    QByteArray pathBytes = path.toLocal8Bit();
    if (ma_decoder_init_file(pathBytes.constData(), &cfg, dec) != MA_SUCCESS) {
        delete dec;
        return false;
    }

    // Get total frames for duration
    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(dec, &totalFrames);
    m_totalFrames.store(totalFrames);
    m_framesPlayed.store(0);

    QMutexLocker lk(&m_decoderMtx);
    m_decoder = dec;
    return true;
}

void AudioEngine::closeDecoder() {
    QMutexLocker lk(&m_decoderMtx);
    if (m_decoder) {
        ma_decoder_uninit(m_decoder);
        delete m_decoder;
        m_decoder = nullptr;
    }
}

// ── EQ ────────────────────────────────────────────────────────
void AudioEngine::setEQGains(const QVector<float> &gainsDB) {
    QMutexLocker lk(&m_eqMtx);
    m_gains  = gainsDB;
    m_eqFlat = true;
    for (float v : gainsDB) if (std::abs(v) > 0.05f) { m_eqFlat = false; break; }
    rebuildEQ();
}

void AudioEngine::rebuildEQ() {
    for (int b = 0; b < BANDS; ++b) {
        float g = (b < m_gains.size()) ? m_gains[b] : 0.f;
        BiquadFilter::Type t =
            (b == 0)       ? BiquadFilter::LowShelf  :
            (b == BANDS-1) ? BiquadFilter::HighShelf :
                             BiquadFilter::PeakEQ;
        for (int c = 0; c < CH; ++c) {
            m_eqFilters[b][c].reset();
            m_eqFilters[b][c].setParams(t, FREQS[b], SR, g, 1.41421356);
        }
    }
}

// ── Playlist ──────────────────────────────────────────────────
void AudioEngine::setPlaylist(const QList<Track> &t)
    { m_pl=t; m_idx=-1; rebuildOrder(); emit playlistChanged(); }
void AudioEngine::appendTrack(const Track &t)
    { m_pl.append(t); rebuildOrder(); emit playlistChanged(); }
void AudioEngine::clearPlaylist()
    { stop(); m_pl.clear(); m_idx=-1; rebuildOrder(); emit playlistChanged(); }
void AudioEngine::removeTrack(int i) {
    if (i<0||i>=m_pl.size()) return;
    m_pl.removeAt(i);
    if (m_idx>i) --m_idx; else if (m_idx==i) m_idx=-1;
    rebuildOrder(); emit playlistChanged();
}
void AudioEngine::moveTrack(int f, int t)
    { if(f!=t){m_pl.move(f,t);rebuildOrder();emit playlistChanged();} }

// ── Transport ─────────────────────────────────────────────────
void AudioEngine::playIndex(int i) {
    if (i<0||i>=m_pl.size()) return;

    m_playing.store(false);
    m_trackEnd.store(false);

    QString path = m_pl[i].fileUrl.toLocalFile();
    if (!openDecoder(path)) {
        emit errorOccurred("Cannot open: " + path);
        return;
    }

    m_idx   = i;
    m_state = Playing;
    m_playing.store(true);
    m_paused.store(false);

    m_posTimer->start();
    m_lvlTimer->start();
    m_endTimer->start();

    emit trackChanged(i);
    emit stateChanged(m_state);

    // Emit duration
    uint64_t tf = m_totalFrames.load();
    if (tf > 0) emit durationChanged(static_cast<qint64>(tf * 1000 / SR));
}

void AudioEngine::play() {
    if (m_state == Paused) {
        m_paused.store(false);
        m_state = Playing;
        m_posTimer->start();
        m_lvlTimer->start();
        emit stateChanged(m_state);
    } else if (m_state == Stopped && !m_pl.isEmpty()) {
        playIndex(m_idx >= 0 ? m_idx : 0);
    }
}

void AudioEngine::pause() {
    if (m_state != Playing) return;
    m_paused.store(true);
    m_state = Paused;
    m_posTimer->stop();
    m_lvlTimer->stop();
    emit stateChanged(m_state);
    emit levelChanged(-96.f,-96.f);
}

void AudioEngine::stop() {
    m_playing.store(false);
    m_paused.store(false);
    closeDecoder();
    m_posTimer->stop();
    m_lvlTimer->stop();
    m_endTimer->stop();
    m_framesPlayed.store(0);
    m_state = Stopped;
    emit stateChanged(m_state);
    emit positionChanged(0);
    emit levelChanged(-96.f,-96.f);
}

void AudioEngine::next() {
    int i = nextIdx();
    if (i >= 0)                                      playIndex(i);
    else if (m_repeat==RepeatAll && !m_pl.isEmpty()) playIndex(0);
    else                                             stop();
}

void AudioEngine::previous() {
    if (position() > 3000) { seek(0); return; }
    int i = prevIdx();
    if (i >= 0) playIndex(i);
}

void AudioEngine::seek(qint64 ms) {
    if (!m_decoder || m_idx < 0) return;
    ma_uint64 frame = static_cast<ma_uint64>(ms) * SR / 1000;
    QMutexLocker lk(&m_decoderMtx);
    if (m_decoder) {
        ma_decoder_seek_to_pcm_frame(m_decoder, frame);
        m_framesPlayed.store(frame);
    }
}

// ── Volume ────────────────────────────────────────────────────
void AudioEngine::setVolume(float v) {
    m_volume = v; m_vol.store(v);
}
void AudioEngine::setMuted(bool m) {
    m_muted = m; m_mute.store(m);
}
void AudioEngine::setShuffleMode(bool s) { m_shuffle = s; rebuildOrder(); }

// ── Position / Duration ───────────────────────────────────────
qint64 AudioEngine::position() const {
    uint64_t fp = m_framesPlayed.load();
    return static_cast<qint64>(fp * 1000 / SR);
}
qint64 AudioEngine::duration() const {
    uint64_t tf = m_totalFrames.load();
    return static_cast<qint64>(tf * 1000 / SR);
}

// ── Timers ────────────────────────────────────────────────────
void AudioEngine::onPosTick() {
    if (m_state == Playing) emit positionChanged(position());
}

void AudioEngine::onLevelTick() {
    if (!isPlaying()) { emit levelChanged(-96.f,-96.f); return; }
    static float lL=-20.f,lR=-20.f; static int ph=0; ++ph;
    float tL=-5.f-10.f*std::abs(std::sin(ph*0.11f))-5.f*std::abs(std::sin(ph*0.23f+1.3f));
    float tR=-6.f-10.f*std::abs(std::sin(ph*0.13f+0.5f))-5.f*std::abs(std::sin(ph*0.19f+2.1f));
    lL+=(tL-lL)*0.35f; lR+=(tR-lR)*0.35f;
    emit levelChanged(lL,lR);
}

// ── Helpers ───────────────────────────────────────────────────
void AudioEngine::rebuildOrder() {
    m_order.clear();
    for (int i=0; i<m_pl.size(); ++i) m_order<<i;
    if (m_shuffle)
        std::shuffle(m_order.begin(),m_order.end(),*QRandomGenerator::global());
}
int AudioEngine::nextIdx() const {
    if (m_pl.isEmpty()) return -1;
    if (m_shuffle){int p=m_order.indexOf(m_idx);return p+1<m_order.size()?m_order[p+1]:-1;}
    int n=m_idx+1; return n<m_pl.size()?n:-1;
}
int AudioEngine::prevIdx() const {
    if (m_pl.isEmpty()) return -1;
    if (m_shuffle){int p=m_order.indexOf(m_idx);return p>0?m_order[p-1]:-1;}
    int n=m_idx-1; return n>=0?n:-1;
}
