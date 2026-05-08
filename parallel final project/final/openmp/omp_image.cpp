#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <omp.h>
#include "../core/filters.h"
#include "../core/ppm_io.h"
using namespace std;
using Clock=chrono::high_resolution_clock;
using MS=chrono::duration<double,milli>;

int main(int argc, char* argv[]) {
    string in =(argc>1)?argv[1]:"../input/input_4k.ppm";
    string out=(argc>2)?argv[2]:"../output/omp/output_omp.ppm";
    int nt=(argc>3)?atoi(argv[3]):omp_get_max_threads();
    omp_set_num_threads(nt);

    string fmt; int w,h,maxval;
    vector<Pixel> img;
    auto t0=Clock::now();
    if (!readPPM(in,fmt,w,h,maxval,img)) return 1;
    double t_read=MS(Clock::now()-t0).count();
    long long npix=(long long)w*h;

    auto ts=Clock::now();
    #pragma omp parallel for schedule(static)
    for (int i=0;i<(int)img.size();i++){
        unsigned char g=(unsigned char)(0.299f*img[i].r+0.587f*img[i].g+0.114f*img[i].b);
        img[i].r=img[i].g=img[i].b=g;
    }
    double t_gray=MS(Clock::now()-ts).count();

    ts=Clock::now();
    #pragma omp parallel for schedule(static)
    for (int i=0;i<(int)img.size();i++){
        img[i].r=(unsigned char)min(255,max(0,(int)img[i].r+30));
        img[i].g=(unsigned char)min(255,max(0,(int)img[i].g+30));
        img[i].b=(unsigned char)min(255,max(0,(int)img[i].b+30));
    }
    double t_bright=MS(Clock::now()-ts).count();

    ts=Clock::now();
    #pragma omp parallel for schedule(static)
    for (int i=0;i<(int)img.size();i++){
        unsigned char v=((int)(0.299f*img[i].r+0.587f*img[i].g+0.114f*img[i].b)>100)?255:0;
        img[i].r=img[i].g=img[i].b=v;
    }
    double t_thresh=MS(Clock::now()-ts).count();

    double t_compute=t_gray+t_bright+t_thresh;
    ts=Clock::now();
    if (!writePPM(out,fmt,w,h,maxval,img)) return 1;
    double t_write=MS(Clock::now()-ts).count();

    cout<<"=== OpenMP Pipeline ===\n";
    cout<<"Image       : "<<w<<"x"<<h<<" ("<<npix<<" pixels)\n";
    cout<<"Threads     : "<<nt<<"\n\n";
    cout<<"  Grayscale      : "<<t_gray  <<" ms\n";
    cout<<"  Brighten       : "<<t_bright<<" ms\n";
    cout<<"  Threshold      : "<<t_thresh<<" ms\n";
    cout<<"  --------------------------------\n";
    cout<<"  Compute total  : "<<t_compute<<" ms\n";
    cout<<"  I/O read       : "<<t_read  <<" ms\n";
    cout<<"  I/O write      : "<<t_write <<" ms\n\n";
    cout<<"Throughput      : "<<(npix/t_compute/1000.0)<<" Mpix/s\n";
    cout<<"RESULT omp "<<nt<<" "<<t_compute<<" "<<t_read<<" "<<t_write<<"\n";
    return 0;
}
