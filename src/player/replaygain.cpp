#include "replaygain.h"
#include "../../third_party/miniaudio.h"
#include <cmath>

void ReplayGainScanner::run() {
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder dec;
    QByteArray path = m_url.toLocalFile().toLocal8Bit();
    if (ma_decoder_init_file(path.constData(), &cfg, &dec) != MA_SUCCESS) {
        emit done(m_id, 0.f); return;
    }

    // Compute mean square
    double sumSq = 0; long long count = 0;
    const int CHUNK = 4096;
    float buf[CHUNK * 2];
    while (true) {
        ma_uint64 got = 0;
        ma_decoder_read_pcm_frames(&dec, buf, CHUNK, &got);
        if (got == 0) break;
        for (ma_uint64 i = 0; i < got * 2; ++i)
            sumSq += buf[i] * buf[i];
        count += got * 2;
    }
    ma_decoder_uninit(&dec);

    if (count == 0) { emit done(m_id, 0.f); return; }

    double rmsDB = 10.0 * std::log10(sumSq / count + 1e-12);
    // Target: -18 dBFS (EBU R128 reference)
    float gainDB = static_cast<float>(-18.0 - rmsDB);
    gainDB = qBound(-20.f, gainDB, 20.f);
    emit done(m_id, gainDB);
}
