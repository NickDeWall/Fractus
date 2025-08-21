#pragma once
#include <SDL2/SDL.h>
#include "config.h"
#include <cmath>
#include <vector>
#include <stdexcept>

namespace MathUtils {
    void hsvToRgb(float h, float s, float v, Uint8& r, Uint8& g, Uint8& b);
    void rgbToHsv(Uint8 r, Uint8 g, Uint8 b, float& h, float& s, float& v);
    double linearInterpolate(double y0, double y1, double t);
    double linearInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi);
    double cubicInterpolate(double y0, double y1, double y2, double y3, double t);
    double catmullRomInterpolate(double y0, double y1, double y2, double y3, double t);
    double hermiteInterpolate(double y0, double y1, double t0, double t1, double t);
    double cosineInterpolate(double y0, double y1, double t);
    double smoothstepInterpolate(double y0, double y1, double t);
    double smootherstepInterpolate(double y0, double y1, double t);
    double lagrangeInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi);
    std::vector<std::vector<double>> dividedDifferences(const std::vector<double>& x, const std::vector<double>& y);
    double newtonInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi);
    double barycentricInterpolate(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& w, double xi);
    double bilinearInterpolate(double q00, double q01, double q10, double q11, double x, double y);
    std::vector<double> chebyshevNodes(int n, double a, double b);
    std::vector<double> chebyshevWeights(const std::vector<double>& nodes);
}