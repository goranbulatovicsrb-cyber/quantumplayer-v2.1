#include "waveformwidget.h"
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>
#include <cmath>
#include <cstring>

// Include miniaudio for decoding waveform
#define MINIAUDIO_IMPLEMENTATION
// Already implemented in miniaudio_impl.cpp — just include header
#undef MINIAUDIO_IMPLEMENTATION
#include "../../third_party/miniaudio.h"

// ── WaveformScanner ───────────────────────────────────────────
void WaveformScanner::run() {
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 1, 22050);
    ma_decoder dec;
    QByteArray pathBytes = m_path.toLocal8Bit();
    if (ma_decoder_init_file(pathBytes.constData(), &cfg, &dec) != MA_SUCCESS) {
        emit done({});
        return;
    }

    ma_uint64 total = 0;
    ma_decoder_get_length_in_pcm_frames(&dec, &total);
    if (total == 0) { ma_decoder_uninit(&dec); emit done({}); return; }

    // Read all samples
    const int CHUNK = 4096;
    QVector<float> all;
    all.reserve(static_cast<int>(total));
    float buf[CHUNK];
    ma_uint64 read = 0;
    while (true) {
        ma_uint64 got = 0;
        ma_decoder_read_pcm_frames(&dec, buf, CHUNK, &got);
        if (got == 0) break;
        for (ma_uint64 i = 0; i < got; ++i) all.append(std::abs(buf[i]));
        read += got;
    }
    ma_decoder_uninit(&dec);

    if (all.isEmpty()) { emit done({}); return; }

    // Downsample to m_w peaks
    QVector<float> peaks(m_w, 0.f);
    int samplesPerBin = qMax(1, all.size() / m_w);
    for (int i = 0; i < m_w; ++i) {
        int start = i * samplesPerBin;
        int end   = qMin(start + samplesPerBin, all.size());
        float mx  = 0;
        for (int j = start; j < end; ++j)
            if (all[j] > mx) mx = all[j];
        peaks[i] = mx;
    }

    // Normalize
    float maxPeak = *std::max_element(peaks.begin(), peaks.end());
    if (maxPeak > 0.001f)
        for (auto &p : peaks) p /= maxPeak;

    emit done(peaks);
}

// ── WaveformWidget ────────────────────────────────────────────
WaveformWidget::WaveformWidget(QWidget *parent) : QWidget(parent) {
    setFixedHeight(36);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

void WaveformWidget::loadFile(const QUrl &url) {
    QString path = url.toLocalFile();
    if (path == m_currentFile) return;
    m_currentFile = path;
    m_peaks.clear();
    m_loading = true;
    update();

    auto *scanner = new WaveformScanner(path, width() > 0 ? width() : 600);
    connect(scanner, &WaveformScanner::done, this, &WaveformWidget::onSamplesReady);
    QThreadPool::globalInstance()->start(scanner);
}

void WaveformWidget::setPosition(qint64 ms, qint64 durMs) {
    m_posMs = ms; m_durMs = durMs; update();
}

void WaveformWidget::setAccentColor(const QColor &c) { m_accent = c; }

void WaveformWidget::onSamplesReady(const QVector<float> &peaks) {
    m_peaks   = peaks;
    m_loading = false;
    update();
}

void WaveformWidget::seekFromX(int x) {
    if (m_durMs <= 0) return;
    float ratio = static_cast<float>(x) / width();
    ratio = qBound(0.f, ratio, 1.f);
    emit seekRequested(static_cast<qint64>(ratio * m_durMs));
}

void WaveformWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) seekFromX(e->pos().x());
}
void WaveformWidget::mouseMoveEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::LeftButton) seekFromX(e->pos().x());
}

void WaveformWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W = width(), H = height();
    int mid = H / 2;

    // Background
    p.fillRect(rect(), QColor(0,0,0,60));

    if (m_loading) {
        p.setPen(QColor(255,255,255,60));
        p.drawText(rect(), Qt::AlignCenter, "Analysing waveform...");
        return;
    }

    if (m_peaks.isEmpty()) {
        // Plain progress bar fallback
        if (m_durMs > 0) {
            float prog = static_cast<float>(m_posMs) / m_durMs;
            p.fillRect(0, 0, static_cast<int>(prog * W), H,
                       QColor(m_accent.red(), m_accent.green(), m_accent.blue(), 80));
        }
        p.setPen(QColor(255,255,255,30));
        p.drawText(rect(), Qt::AlignCenter, "—");
        return;
    }

    // Played position in pixels
    int playedX = 0;
    if (m_durMs > 0)
        playedX = static_cast<int>(static_cast<float>(m_posMs) / m_durMs * W);

    int bins = m_peaks.size();
    for (int x = 0; x < W; ++x) {
        int binIdx = x * bins / W;
        if (binIdx >= bins) binIdx = bins - 1;
        float peak = m_peaks[binIdx];
        int   h    = qMax(2, static_cast<int>(peak * (mid - 2)));

        bool played = (x <= playedX);
        QColor col  = played ? m_accent : QColor(255,255,255,50);

        // Mirror waveform
        p.fillRect(x, mid - h, 1, h,     col);
        QColor dimCol = played
            ? QColor(m_accent.red(), m_accent.green(), m_accent.blue(), 120)
            : QColor(255,255,255,25);
        p.fillRect(x, mid,     1, h / 2, dimCol);
    }

    // Playhead
    if (playedX > 0 && playedX < W) {
        p.fillRect(playedX, 0, 2, H, Qt::white);
    }
}
