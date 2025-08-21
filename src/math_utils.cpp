#include "math_utils.h"
#include "config.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <stdexcept>

namespace MathUtils {
    void hsvToRgb(float h, float s, float v, Uint8& r, Uint8& g, Uint8& b) {
        if (s <= 0.0f) {
            r = g = b = static_cast<Uint8>(v * 255);
            return;
        }

        h = fmod(h, 1.0f) * 6.0f;
        int i = static_cast<int>(h);
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i) {
        case 0: r = static_cast<Uint8>(v * 255); g = static_cast<Uint8>(t * 255); b = static_cast<Uint8>(p * 255); break;
        case 1: r = static_cast<Uint8>(q * 255); g = static_cast<Uint8>(v * 255); b = static_cast<Uint8>(p * 255); break;
        case 2: r = static_cast<Uint8>(p * 255); g = static_cast<Uint8>(v * 255); b = static_cast<Uint8>(t * 255); break;
        case 3: r = static_cast<Uint8>(p * 255); g = static_cast<Uint8>(q * 255); b = static_cast<Uint8>(v * 255); break;
        case 4: r = static_cast<Uint8>(t * 255); g = static_cast<Uint8>(p * 255); b = static_cast<Uint8>(v * 255); break;
        default: r = static_cast<Uint8>(v * 255); g = static_cast<Uint8>(p * 255); b = static_cast<Uint8>(q * 255); break;
        }
    }

    void rgbToHsv(Uint8 r, Uint8 g, Uint8 b, float& h, float& s, float& v) {
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;
        float max = std::max({ rf, gf, bf });
        float min = std::min({ rf, gf, bf });
        float delta = max - min;
        v = max;
        s = (max != 0.0f) ? (delta / max) : 0.0f;

        if (delta == 0.0f) {
            h = 0.0f;
        }
        else if (max == rf) {
            h = fmod(((gf - bf) / delta), 6.0f) / 6.0f;
        }
        else if (max == gf) {
            h = ((bf - rf) / delta + 2.0f) / 6.0f;
        }
        else {
            h = ((rf - gf) / delta + 4.0f) / 6.0f;
        }
        if (h < 0.0f) h += 1.0f;
    }

    double linearInterpolate(double y0, double y1, double t) {
        return y0 + t * (y1 - y0);
    }

    double linearInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi) {
        if (x.size() != y.size() || x.size() < 2) {
            throw std::invalid_argument("Invalid input arrays");
        }

        auto it = std::lower_bound(x.begin(), x.end(), xi);
        if (it == x.begin()) return y[0];
        if (it == x.end()) return y.back();

        size_t i = it - x.begin();
        double t = (xi - x[i-1]) / (x[i] - x[i-1]);
        return linearInterpolate(y[i-1], y[i], t);
    }

    double cubicInterpolate(double y0, double y1, double y2, double y3, double t) {
        double a0 = y3 - y2 - y0 + y1;
        double a1 = y0 - y1 - a0;
        double a2 = y2 - y0;
        double a3 = y1;

        return a0 * t * t * t + a1 * t * t + a2 * t + a3;
    }

    double catmullRomInterpolate(double y0, double y1, double y2, double y3, double t) {
        double t2 = t * t;
        double t3 = t2 * t;

        return 0.5 * ((2.0 * y1) +(-y0 + y2) * t +(2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3) * t2 +(-y0 + 3.0 * y1 - 3.0 * y2 + y3) * t3);
    }

    double hermiteInterpolate(double y0, double y1, double t0, double t1, double t) {
        double t2 = t * t;
        double t3 = t2 * t;

        double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
        double h10 = t3 - 2.0 * t2 + t;
        double h01 = -2.0 * t3 + 3.0 * t2;
        double h11 = t3 - t2;

        return h00 * y0 + h10 * t0 + h01 * y1 + h11 * t1;
    }

    double cosineInterpolate(double y0, double y1, double t) {
        double ft = t * Config::PI;
        double f = (1.0 - cos(ft)) * 0.5;
        return y0 * (1.0 - f) + y1 * f;
    }

    double smoothstepInterpolate(double y0, double y1, double t) {
        t = std::clamp(t, 0.0, 1.0);
        t = t * t * (3.0 - 2.0 * t);
        return linearInterpolate(y0, y1, t);
    }

    double smootherstepInterpolate(double y0, double y1, double t) {
        t = std::clamp(t, 0.0, 1.0);
        t = t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
        return linearInterpolate(y0, y1, t);
    }

    double lagrangeInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi) {
        if (x.size() != y.size()) {
            throw std::invalid_argument("x and y vectors must have same size");
        }

        double result = 0.0;
        size_t n = x.size();

        for (size_t i = 0; i < n; ++i) {
            double term = y[i];
            for (size_t j = 0; j < n; ++j) {
                if (i != j) {
                    term *= (xi - x[j]) / (x[i] - x[j]);
                }
            }
            result += term;
        }

        return result;
    }

    std::vector<std::vector<double>> dividedDifferences(const std::vector<double>& x, const std::vector<double>& y) {
        size_t n = x.size();
        std::vector<std::vector<double>> table(n, std::vector<double>(n, 0.0));
                                      
        for (size_t i = 0; i < n; ++i) {
            table[i][0] = y[i];
        }

        for (size_t j = 1; j < n; ++j) {
            for (size_t i = 0; i < n - j; ++i) {
                table[i][j] = (table[i + 1][j - 1] - table[i][j - 1]) / (x[i + j] - x[i]);
            }
        }

        return table;
    }

    double newtonInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi) {
        auto table = dividedDifferences(x, y);
        size_t n = x.size();

        double result = table[0][0];
        double product = 1.0;

        for (size_t i = 1; i < n; ++i) {
            product *= (xi - x[i - 1]);
            result += table[0][i] * product;
        }

        return result;
    }

    double barycentricInterpolate(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& w, double xi) {
        if (x.size() != y.size() || x.size() != w.size()) {
            throw std::invalid_argument("All vectors must have same size");
        }

        double numerator = 0.0;
        double denominator = 0.0;

        for (size_t i = 0; i < x.size(); ++i) {
            if (std::abs(xi - x[i]) < 1e-12) {
                return y[i];
            }

            double temp = w[i] / (xi - x[i]);
            numerator += temp * y[i];
            denominator += temp;
        }

        return numerator / denominator;
    }

    class CubicSpline {
    private:
        std::vector<double> x, y, a, b, c, d;

    public:
        CubicSpline(const std::vector<double>& x_vals, const std::vector<double>& y_vals) 
            : x(x_vals), y(y_vals) {
            if (x.size() != y.size() || x.size() < 3) {
                throw std::invalid_argument("Need at least 3 points for cubic spline");
            }
            computeCoefficients();
        }

        void computeCoefficients() {
            size_t n = x.size() - 1;
            a = y;
            b.resize(n);
            c.resize(n + 1);
            d.resize(n);

            std::vector<double> h(n);
            for (size_t i = 0; i < n; ++i) {
                h[i] = x[i + 1] - x[i];
            }

            std::vector<double> alpha(n);
            for (size_t i = 1; i < n; ++i) {
                alpha[i] = 3.0 * (a[i + 1] - a[i]) / h[i] - 3.0 * (a[i] - a[i - 1]) / h[i - 1];
            }

            std::vector<double> l(n + 1), mu(n + 1), z(n + 1);
            l[0] = 1.0;
            mu[0] = 0.0;
            z[0] = 0.0;

            for (size_t i = 1; i < n; ++i) {
                l[i] = 2.0 * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
                mu[i] = h[i] / l[i];
                z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
            }

            l[n] = 1.0;
            z[n] = 0.0;
            c[n] = 0.0;

            for (int j = n - 1; j >= 0; --j) {
                c[j] = z[j] - mu[j] * c[j + 1];
                b[j] = (a[j + 1] - a[j]) / h[j] - h[j] * (c[j + 1] + 2.0 * c[j]) / 3.0;
                d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
            }
        }

        double interpolate(double xi) const {
            auto it = std::lower_bound(x.begin(), x.end(), xi);
            if (it == x.begin()) it++;
            if (it == x.end()) it--;

            size_t j = it - x.begin() - 1;
            double dx = xi - x[j];

            return a[j] + b[j] * dx + c[j] * dx * dx + d[j] * dx * dx * dx;
        }
    };

    double bilinearInterpolate(double q00, double q01, double q10, double q11, double x, double y) {
        double r1 = linearInterpolate(q00, q10, x);
        double r2 = linearInterpolate(q01, q11, x);
        return linearInterpolate(r1, r2, y);
    }

    std::vector<double> chebyshevNodes(int n, double a = -1.0, double b = 1.0) {
        std::vector<double> nodes(n);
        for (int i = 0; i < n; ++i) {
            double t = cos(Config::PI * (2 * i + 1) / (2 * n));
            nodes[i] = 0.5 * (a + b) + 0.5 * (b - a) * t;
        }
        return nodes;
    }

    std::vector<double> chebyshevWeights(const std::vector<double>& nodes) {
        size_t n = nodes.size();
        std::vector<double> weights(n);

        for (size_t i = 0; i < n; ++i) {
            weights[i] = pow(-1, i);
            if (i == 0 || i == n - 1) {
                weights[i] *= 0.5;
            }
        }

        return weights;
    }
}