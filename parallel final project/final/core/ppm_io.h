#ifndef PPM_IO_H
#define PPM_IO_H
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "filters.h"

inline bool readPPM(const std::string& path, std::string& fmt,
                    int& w, int& h, int& maxval, std::vector<Pixel>& img) {
    std::ifstream f(path);
    if (!f) { std::cerr<<"Cannot open "<<path<<"\n"; return false; }
    f>>fmt>>w>>h>>maxval;
    img.resize(w*h);
    for (int i=0;i<w*h;i++) {
        int r,g,b; f>>r>>g>>b;
        img[i]={(unsigned char)r,(unsigned char)g,(unsigned char)b};
    }
    return true;
}
inline bool writePPM(const std::string& path, const std::string& fmt,
                     int w, int h, int maxval, const std::vector<Pixel>& img) {
    std::ofstream f(path);
    if (!f) { std::cerr<<"Cannot write "<<path<<"\n"; return false; }
    f<<fmt<<"\n"<<w<<" "<<h<<"\n"<<maxval<<"\n";
    for (int i=0;i<(int)img.size();i++) {
        f<<(int)img[i].r<<" "<<(int)img[i].g<<" "<<(int)img[i].b;
        if (i<(int)img.size()-1) f<<"\n";
    }
    f<<"\n";
    return true;
}
#endif
