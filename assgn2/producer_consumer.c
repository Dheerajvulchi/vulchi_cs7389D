#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAG_WORK 1
#define TAG_ACK 2
#define TAG_REQUEST 3
#define TAG_ABORT 4
#define TAG_NO_WORK 5
#define TAG_COUNT 6

typedef struct {
    int *data;
    int front;
    int rear;
    int size;
    int capacity;
} Queue;

static void queue_init(Queue *q, int capacity) {
    q->data = (int *)malloc(sizeof(int) * capacity);
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = capacity;
}

static void queue_free(Queue *q) {
    free(q->data);
    q->data = NULL;
}

static int queue_empty(Queue *q) {
    return q->size == 0;
}

static int queue_full(Queue *q) {
    return q->size == q->capacity;
}

static void queue_push(Queue *q, int value) {
    if (queue_full(q)) return;
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
}

static int queue_pop(Queue *q) {
    if (queue_empty(q)) return -1;
    int value = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return value;
}

static void do_work(int seed_value) {
    volatile unsigned long long x = (unsigned long long)(seed_value + 1);
    for (int i = 0; i < 2000; i++) {
        x = (x * 1103515245ULL + 12345ULL) % 2147483647ULL;
    }
}

static int elapsed_expired(double start, int seconds) {
    return (MPI_Wtime() - start) >= seconds;
}

static void run_producer(int rank, int sim_time) {
    double start = MPI_Wtime();
    unsigned int seed = (unsigned int)(time(NULL) + rank * 1009);

    while (1) {
        int work = (int)(rand_r(&seed) % 100000);
        MPI_Request req;
        MPI_Isend(&work, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &req);

        do_work(work);

        MPI_Wait(&req, MPI_STATUS_IGNORE);

        int response;
        MPI_Recv(&response, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (elapsed_expired(start, sim_time)) {
            if (response == TAG_ABORT) break;
        }
        if (response == TAG_ABORT) break;
    }

    int zero = 0;
    MPI_Request req;
    MPI_Isend(&zero, 1, MPI_INT, 0, TAG_COUNT, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
}

static void run_consumer(int rank, int sim_time) {
    double start = MPI_Wtime();
    int consumed_count = 0;
    int previous_work = 1;

    while (1) {
        int req_val = 1;
        MPI_Request req;
        MPI_Isend(&req_val, 1, MPI_INT, 0, TAG_REQUEST, MPI_COMM_WORLD, &req);

        do_work(previous_work);

        MPI_Wait(&req, MPI_STATUS_IGNORE);

        int msg;
        MPI_Status status;
        MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_WORK) {
            consumed_count++;
            previous_work = msg;
        } else if (status.MPI_TAG == TAG_ABORT) {
            break;
        }

        if (elapsed_expired(start, sim_time) && status.MPI_TAG == TAG_ABORT) {
            break;
        }
    }

    MPI_Request req;
    MPI_Isend(&consumed_count, 1, MPI_INT, 0, TAG_COUNT, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
}

static void run_broker(int size, int sim_time) {
    int producers = (size - 1) / 2;
    int consumers = (size - 1) - producers;
    int first_producer = 1;
    int last_producer = producers;
    int first_consumer = producers + 1;
    int last_consumer = size - 1;

    int buffer_capacity = 2 * producers;
    if (buffer_capacity < 1) buffer_capacity = 1;

    Queue buffer;
    Queue outstanding;
    queue_init(&buffer, buffer_capacity);
    queue_init(&outstanding, size + buffer_capacity + 10);

    int *finished = (int *)calloc(size, sizeof(int));
    int finished_nonbroker = 0;

    double start = MPI_Wtime();

    while (finished_nonbroker < size - 1) {
        MPI_Status status;
        int message;
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int src = status.MPI_SOURCE;
        int tag = status.MPI_TAG;
        int expired = elapsed_expired(start, sim_time);

        if (tag == TAG_COUNT) {
            if (!finished[src]) {
                finished[src] = 1;
                finished_nonbroker++;
            }
            continue;
        }

        if (tag == TAG_WORK) {
            if (expired) {
                int reply = TAG_ABORT;
                MPI_Send(&reply, 1, MPI_INT, src, TAG_ABORT, MPI_COMM_WORLD);
                if (!finished[src]) {
                    finished[src] = 1;
                    finished_nonbroker++;
                }
            } else if (!queue_full(&buffer)) {
                queue_push(&buffer, message);
                int reply = TAG_ACK;
                MPI_Send(&reply, 1, MPI_INT, src, TAG_ACK, MPI_COMM_WORLD);
            } else {
                queue_push(&outstanding, src);
            }
        } else if (tag == TAG_REQUEST) {
            if (expired) {
                int reply = 0;
                MPI_Send(&reply, 1, MPI_INT, src, TAG_ABORT, MPI_COMM_WORLD);
                if (!finished[src]) {
                    finished[src] = 1;
                    finished_nonbroker++;
                }
            } else if (!queue_empty(&buffer)) {
                int work = queue_pop(&buffer);
                MPI_Send(&work, 1, MPI_INT, src, TAG_WORK, MPI_COMM_WORLD);

                if (!queue_empty(&outstanding) && !queue_full(&buffer)) {
                    int prod_rank = queue_pop(&outstanding);
                    int ack = TAG_ACK;
                    MPI_Send(&ack, 1, MPI_INT, prod_rank, TAG_ACK, MPI_COMM_WORLD);
                }
            } else {
                int no_work = 0;
                MPI_Send(&no_work, 1, MPI_INT, src, TAG_NO_WORK, MPI_COMM_WORLD);
            }
        }

        while (!queue_empty(&outstanding) && !queue_full(&buffer)) {
            int prod_rank = queue_pop(&outstanding);
            int ack = TAG_ACK;
            MPI_Send(&ack, 1, MPI_INT, prod_rank, TAG_ACK, MPI_COMM_WORLD);
        }
    }

    int total_consumed = 0;
    for (int r = first_consumer; r <= last_consumer; r++) {
        if (r >= first_consumer && r <= last_consumer) {
            total_consumed += 0;
        }
    }

    free(finished);
    queue_free(&buffer);
    queue_free(&outstanding);

    int local_counts = 0;
    for (int i = first_consumer; i <= last_consumer; i++) {
        (void)i;
    }

    MPI_Status status;
    int remaining_counts = consumers;
    while (remaining_counts > 0) {
        int count_val;
        MPI_Recv(&count_val, 1, MPI_INT, MPI_ANY_SOURCE, TAG_COUNT, MPI_COMM_WORLD, &status);
        total_consumed += count_val;
        remaining_counts--;
    }

    printf("Total number of messages consumed: %d\n", total_consumed);
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

    if (size < 3) {
        if (rank == 0) {
            fprintf(stderr, "Need at least 3 processes\n");
        }
        MPI_Finalize();
        return 1;
    }

    int producers = (size - 1) / 2;
    int consumers = (size - 1) - producers;

    if (producers < 1 || consumers < 1) {
        if (rank == 0) {
            fprintf(stderr, "Configuration must have at least 1 producer and 1 consumer\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        run_broker(size, sim_time);
    } else if (rank <= producers) {
        run_producer(rank, sim_time);
    } else {
        run_consumer(rank, sim_time);
    }

    MPI_Finalize();
    return 0;
}
