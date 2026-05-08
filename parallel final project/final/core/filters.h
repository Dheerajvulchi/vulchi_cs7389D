#ifndef FILTERS_H
#define FILTERS_H
#include <vector>
#include <algorithm>
#include <cmath>

struct Pixel { unsigned char r, g, b; };

static inline int lum(const Pixel& p) {
    return (int)(0.299f*p.r + 0.587f*p.g + 0.114f*p.b);
}
inline void grayscale(std::vector<Pixel>& img) {
    for (auto& p : img) { unsigned char g=(unsigned char)lum(p); p.r=p.g=p.b=g; }
}
inline void brighten(std::vector<Pixel>& img, int v) {
    for (auto& p : img) {
        p.r=(unsigned char)std::min(255,std::max(0,(int)p.r+v));
        p.g=(unsigned char)std::min(255,std::max(0,(int)p.g+v));
        p.b=(unsigned char)std::min(255,std::max(0,(int)p.b+v));
    }
}
inline void threshold(std::vector<Pixel>& img, int t) {
    for (auto& p : img) { unsigned char v=(lum(p)>t)?255:0; p.r=p.g=p.b=v; }
}
#endif
