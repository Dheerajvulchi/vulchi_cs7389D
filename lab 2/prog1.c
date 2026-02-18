#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int nproc, rank;
    int msg;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nproc < 2) {
        if (rank == 0)
            printf("Need at least 2 processes.\n");
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        msg = rank;
        MPI_Send(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("Rank %d sent value %d to Rank 1\n", rank, msg);
    } else if (rank == 1) {
        MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Rank %d received value %d from Rank 0\n", rank, msg);
    }

    MPI_Finalize();
    return 0;
}

