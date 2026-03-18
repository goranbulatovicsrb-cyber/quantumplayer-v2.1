#pragma once
#include <cmath>

// Second-order IIR (biquad) filter
struct BiquadFilter {
    enum Type { PeakEQ, LowShelf, HighShelf, LowPass, HighPass };

    // Current coefficients
    double b0=1, b1=0, b2=0, a1=0, a2=0;
    // Target coefficients (for smooth transition)
    double tb0=1, tb1=0, tb2=0, ta1=0, ta2=0;
    // State
    double x1=0, x2=0, y1=0, y2=0;
    // Smoothing: 0=instant, higher=smoother (0.9995 ≈ 50ms at 44100Hz)
    static constexpr double SMOOTH = 0.9998;  // ~200ms, zero crackle
    bool needsSmooth = false;

    void setParams(Type type, double freq, double sampleRate,
                   double gainDB, double Q = 1.41421356) {
        double A  = std::pow(10.0, gainDB / 40.0);
        double w0 = 2.0 * M_PI * freq / sampleRate;
        double cosW = std::cos(w0);
        double sinW = std::sin(w0);
        double alpha = sinW / (2.0 * Q);
        double nb0, nb1, nb2, na0, na1, na2;

        switch (type) {
        case PeakEQ:
            nb0 = 1 + alpha * A;
            nb1 = -2 * cosW;
            nb2 = 1 - alpha * A;
            na0 = 1 + alpha / A;
            na1 = -2 * cosW;
            na2 = 1 - alpha / A;
            break;
        case LowShelf: {
            double sqA = std::sqrt(A);
            double aq  = 2.0 * sqA * alpha;
            nb0 = A * ((A+1) - (A-1)*cosW + aq);
            nb1 = 2*A*((A-1) - (A+1)*cosW);
            nb2 = A * ((A+1) - (A-1)*cosW - aq);
            na0 = (A+1) + (A-1)*cosW + aq;
            na1 = -2*((A-1) + (A+1)*cosW);
            na2 = (A+1) + (A-1)*cosW - aq;
            break;
        }
        case HighShelf: {
            double sqA = std::sqrt(A);
            double aq  = 2.0 * sqA * alpha;
            nb0 = A*((A+1) + (A-1)*cosW + aq);
            nb1 = -2*A*((A-1) + (A+1)*cosW);
            nb2 = A*((A+1) + (A-1)*cosW - aq);
            na0 = (A+1) - (A-1)*cosW + aq;
            na1 = 2*((A-1) - (A+1)*cosW);
            na2 = (A+1) - (A-1)*cosW - aq;
            break;
        }
        default:
            nb0=nb1=nb2=na0=na1=na2=1; break;
        }

        // Store as targets — coefficients will interpolate smoothly
        tb0 = nb0/na0; tb1 = nb1/na0; tb2 = nb2/na0;
        ta1 = na1/na0; ta2 = na2/na0;
        needsSmooth = true;

        // First time: set immediately (no crackle on initial load)
        if (b0 == 1 && b1 == 0 && b2 == 0) {
            b0=tb0; b1=tb1; b2=tb2; a1=ta1; a2=ta2;
            needsSmooth = false;
        }
    }

    inline double process(double x) {
        // Smooth coefficient interpolation (one step per sample)
        if (needsSmooth) {
            b0 += (tb0-b0) * (1.0-SMOOTH);
            b1 += (tb1-b1) * (1.0-SMOOTH);
            b2 += (tb2-b2) * (1.0-SMOOTH);
            a1 += (ta1-a1) * (1.0-SMOOTH);
            a2 += (ta2-a2) * (1.0-SMOOTH);
            // Stop smoothing once close enough
            if (std::abs(b0-tb0) < 1e-9 &&
                std::abs(b1-tb1) < 1e-9 &&
                std::abs(b2-tb2) < 1e-9)
                needsSmooth = false;
        }
        double y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2=x1; x1=x; y2=y1; y1=y;
        return y;
    }

    void reset() {
        x1=x2=y1=y2=0;
        b0=1; b1=b2=a1=a2=0;
        tb0=1; tb1=tb2=ta1=ta2=0;
        needsSmooth=false;
    }
};
