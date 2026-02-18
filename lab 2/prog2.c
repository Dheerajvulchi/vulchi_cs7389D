#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int nproc, rank;
    int value;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int next = (rank + 1) % nproc;
    int prev = (rank - 1 + nproc) % nproc;

    if (rank == 0) {
        srand(time(NULL));
        value = rand() % 1000;
        printf("Rank 0 initial value = %d\n", value);

        MPI_Send(&value, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        MPI_Recv(&value, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("Rank 0 received value = %d from Rank %d\n", value, prev);
    } else {
        MPI_Recv(&value, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Rank %d received value %d from Rank %d\n", rank, value, prev);

        MPI_Send(&value, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        printf("Rank %d sent value %d to Rank %d\n", rank, value, next);
    }

    MPI_Finalize();
    return 0;
}

