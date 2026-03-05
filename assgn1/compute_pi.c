#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[])
{
    long long int N = atoll(argv[1]);
    double step = 1.0 / (double) N;
    double x, sum = 0.0;
    double start, end;

    start = omp_get_wtime();

#pragma omp parallel for private(x) reduction(+:sum)
    for(long long int i = 0; i < N; i++){
        x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x*x);
    }

    double pi = step * sum;

    end = omp_get_wtime();

    printf("Pi = %.12f\n", pi);
    printf("Time_seconds = %.6f\n", end - start);

    return 0;
}
