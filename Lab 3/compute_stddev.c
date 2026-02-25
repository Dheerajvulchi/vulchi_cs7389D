#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <assert.h>

float *rand_num_gen(int num_elements) {
    float *rand_nums = (float *)malloc(sizeof(float) * num_elements);
    assert(rand_nums != NULL);
    for (int i = 0; i < num_elements; i++) {
        rand_nums[i] = (rand() / (float)RAND_MAX);
    }
    return rand_nums;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nproc = 0, rank = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s num_elements_per_proc\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int num_elements_per_proc = atoi(argv[1]);
    if (num_elements_per_proc <= 0) {
        if (rank == 0) {
            fprintf(stderr, "Error: num_elements_per_proc must be > 0\n");
        }
        MPI_Finalize();
        return 1;
    }

    srand(123456 + rank * 1000);

    float *rand_nums = rand_num_gen(num_elements_per_proc);

    double local_sum = 0.0;
    for (int i = 0; i < num_elements_per_proc; i++) {
        local_sum += rand_nums[i];
    }

    double global_sum = 0.0;
    MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    long long global_count = (long long)num_elements_per_proc * (long long)nproc;
    double mean = global_sum / (double)global_count;

    double local_sq_diff_sum = 0.0;
    for (int i = 0; i < num_elements_per_proc; i++) {
        double diff = (double)rand_nums[i] - mean;
        local_sq_diff_sum += diff * diff;
    }

    double global_sq_diff_sum = 0.0;
    MPI_Reduce(&local_sq_diff_sum, &global_sq_diff_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double variance = global_sq_diff_sum / (double)global_count;
        double stddev = sqrt(variance);
        printf("Mean = %.6f\n", mean);
        printf("Standard Deviation = %.6f\n", stddev);
    }

    free(rand_nums);
    MPI_Finalize();
    return 0;
}
