#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "../core/filters.h"
#include "../core/ppm_io.h"
using namespace std;

int main(int argc, char* argv[]) {
    int provided;
    MPI_Init_thread(&argc,&argv,MPI_THREAD_FUNNELED,&provided);
    int rank,size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    string in =(argc>1)?argv[1]:"../input/input_4k.ppm";
    string out=(argc>2)?argv[2]:"../output/hybrid/output_hybrid.ppm";
    int nt=(argc>3)?atoi(argv[3]):omp_get_max_threads();
    omp_set_num_threads(nt);

    int w=0,h=0,maxval=255; string fmt="P3";
    vector<Pixel> image; vector<unsigned char> sendbuf;
    double t_read=0;

    if (rank==0){
        double r0=MPI_Wtime();
        if (!readPPM(in,fmt,w,h,maxval,image)) MPI_Abort(MPI_COMM_WORLD,1);
        t_read=(MPI_Wtime()-r0)*1000.0;
        sendbuf.resize(w*h*3);
        for (int i=0;i<w*h;i++){sendbuf[3*i]=image[i].r;sendbuf[3*i+1]=image[i].g;sendbuf[3*i+2]=image[i].b;}
    }
    MPI_Bcast(&w,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&h,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&maxval,1,MPI_INT,0,MPI_COMM_WORLD);

    int base=h/size,rem=h%size;
    vector<int> rc(size),rd(size);
    int off=0;
    for (int i=0;i<size;i++){rc[i]=base+(i<rem?1:0);rd[i]=off;off+=rc[i];}
    vector<int> pc(size),pd(size);
    for (int i=0;i<size;i++){pc[i]=rc[i]*w*3;pd[i]=rd[i]*w*3;}

    int lpx=rc[rank]*w;
    vector<unsigned char> lb(lpx*3);

    double t0=MPI_Wtime();
    MPI_Scatterv(rank==0?sendbuf.data():nullptr,pc.data(),pd.data(),MPI_UNSIGNED_CHAR,
                 lb.data(),lpx*3,MPI_UNSIGNED_CHAR,0,MPI_COMM_WORLD);

    vector<Pixel> chunk(lpx);
    for (int i=0;i<lpx;i++) chunk[i]={lb[3*i],lb[3*i+1],lb[3*i+2]};

    // grayscale
    #pragma omp parallel for schedule(static)
    for (int i=0;i<lpx;i++){
        unsigned char g=(unsigned char)(0.299f*chunk[i].r+0.587f*chunk[i].g+0.114f*chunk[i].b);
        chunk[i].r=chunk[i].g=chunk[i].b=g;
    }
    // brighten
    #pragma omp parallel for schedule(static)
    for (int i=0;i<lpx;i++){
        chunk[i].r=(unsigned char)min(255,max(0,(int)chunk[i].r+30));
        chunk[i].g=(unsigned char)min(255,max(0,(int)chunk[i].g+30));
        chunk[i].b=(unsigned char)min(255,max(0,(int)chunk[i].b+30));
    }
    // threshold
    #pragma omp parallel for schedule(static)
    for (int i=0;i<lpx;i++){
        unsigned char v=((int)(0.299f*chunk[i].r+0.587f*chunk[i].g+0.114f*chunk[i].b)>100)?255:0;
        chunk[i].r=chunk[i].g=chunk[i].b=v;
    }

    for (int i=0;i<lpx;i++){lb[3*i]=chunk[i].r;lb[3*i+1]=chunk[i].g;lb[3*i+2]=chunk[i].b;}

    vector<unsigned char> rb;
    if (rank==0) rb.resize(w*h*3);
    MPI_Gatherv(lb.data(),lpx*3,MPI_UNSIGNED_CHAR,
                rank==0?rb.data():nullptr,pc.data(),pd.data(),MPI_UNSIGNED_CHAR,0,MPI_COMM_WORLD);

    double t_compute=(MPI_Wtime()-t0)*1000.0;

    if (rank==0){
        vector<Pixel> result(w*h);
        for (int i=0;i<w*h;i++) result[i]={rb[3*i],rb[3*i+1],rb[3*i+2]};
        double w0=MPI_Wtime();
        writePPM(out,fmt,w,h,maxval,result);
        double t_write=(MPI_Wtime()-w0)*1000.0;
        long long npix=(long long)w*h;
        cout<<"=== Hybrid MPI+OpenMP Pipeline ===\n";
        cout<<"Image          : "<<w<<"x"<<h<<" ("<<npix<<" pixels)\n";
        cout<<"MPI ranks      : "<<size<<"\n";
        cout<<"OMP threads    : "<<nt<<" per rank\n";
        cout<<"Total workers  : "<<size*nt<<"\n\n";
        cout<<"  Compute total  : "<<t_compute<<" ms\n";
        cout<<"  I/O read       : "<<t_read   <<" ms\n";
        cout<<"  I/O write      : "<<t_write  <<" ms\n\n";
        cout<<"Throughput     : "<<(npix/t_compute/1000.0)<<" Mpix/s\n";
        cout<<"RESULT hybrid "<<size*nt<<" "<<t_compute<<" "<<t_read<<" "<<t_write<<"\n";
    }
    MPI_Finalize();
    return 0;
}
