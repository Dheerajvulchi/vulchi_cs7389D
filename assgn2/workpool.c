#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAG_WORK 1
#define TAG_ACK 2
#define TAG_ABORT 3

static void do_work(int seed_value) {
    volatile unsigned long long x = (unsigned long long)(seed_value + 3);
    for (int i = 0; i < 2000; i++) {
        x = (x * 1664525ULL + 1013904223ULL) % 2147483647ULL;
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int sim_time = atoi(argv[1]);
    if (sim_time <= 0) {
        if (rank == 0) {
            fprintf(stderr, "Simulation time must be > 0\n");
        }
        MPI_Finalize();
        return 1;
    }

    unsigned int seed = (unsigned int)(time(NULL) + rank * 7919);
    double start = MPI_Wtime();
    int consumed_count = 0;

    while ((MPI_Wtime() - start) < sim_time) {
        int work = (int)(rand_r(&seed) % 100000);
        int target = (int)(rand_r(&seed) % size);

        MPI_Request send_req;
        MPI_Isend(&work, 1, MPI_INT, target, TAG_WORK, MPI_COMM_WORLD, &send_req);

        int ack_received = 0;
        while (!ack_received && (MPI_Wtime() - start) < sim_time) {
            int flag = 0;
            MPI_Status status;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

            if (flag) {
                int msg;
                MPI_Recv(&msg, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);

                if (status.MPI_TAG == TAG_WORK) {
                    int ack = 1;
                    MPI_Send(&ack, 1, MPI_INT, status.MPI_SOURCE, TAG_ACK, MPI_COMM_WORLD);
                    consumed_count++;
                    do_work(msg);
                } else if (status.MPI_TAG == TAG_ACK && status.MPI_SOURCE == target) {
                    ack_received = 1;
                }
            } else {
                do_work(work);
            }
        }

        MPI_Wait(&send_req, MPI_STATUS_IGNORE);
    }

    int total = 0;
    MPI_Reduce(&consumed_count, &total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Total number of messages consumed: %d\n", total);
    }

    MPI_Finalize();
    return 0;
}
