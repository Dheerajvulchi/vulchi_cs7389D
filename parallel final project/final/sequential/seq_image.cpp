#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include "../core/filters.h"
#include "../core/ppm_io.h"
using namespace std;
using Clock=chrono::high_resolution_clock;
using MS=chrono::duration<double,milli>;

int main(int argc, char* argv[]) {
    string in =(argc>1)?argv[1]:"../input/input_4k.ppm";
    string out=(argc>2)?argv[2]:"../output/seq/output_seq.ppm";
    string fmt; int w,h,maxval;
    vector<Pixel> img;
    auto t0=Clock::now();
    if (!readPPM(in,fmt,w,h,maxval,img)) return 1;
    double t_read=MS(Clock::now()-t0).count();
    long long npix=(long long)w*h;

    auto ts=Clock::now();
    grayscale(img);
    double t_gray=MS(Clock::now()-ts).count();
    ts=Clock::now();
    brighten(img,30);
    double t_bright=MS(Clock::now()-ts).count();
    ts=Clock::now();
    threshold(img,100);
    double t_thresh=MS(Clock::now()-ts).count();

    double t_compute=t_gray+t_bright+t_thresh;
    ts=Clock::now();
    if (!writePPM(out,fmt,w,h,maxval,img)) return 1;
    double t_write=MS(Clock::now()-ts).count();

    cout<<"=== Sequential Pipeline ===\n";
    cout<<"Image       : "<<w<<"x"<<h<<" ("<<npix<<" pixels)\n\n";
    cout<<"  Grayscale      : "<<t_gray  <<" ms\n";
    cout<<"  Brighten       : "<<t_bright<<" ms\n";
    cout<<"  Threshold      : "<<t_thresh<<" ms\n";
    cout<<"  --------------------------------\n";
    cout<<"  Compute total  : "<<t_compute<<" ms\n";
    cout<<"  I/O read       : "<<t_read  <<" ms\n";
    cout<<"  I/O write      : "<<t_write <<" ms\n\n";
    cout<<"Throughput      : "<<(npix/t_compute/1000.0)<<" Mpix/s\n";
    cout<<"RESULT seq 1 "<<t_compute<<" "<<t_read<<" "<<t_write<<"\n";
    return 0;
}
