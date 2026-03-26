/* noncontiguous access with a single collective I/O function */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define FILESIZE 1024
#define INTS_PER_BLK 1

int main(int argc, char **argv)
{
    int *buf, *readbuf;
    int rank, nprocs, nints, bufsize, i, num_blocks;
    MPI_File fh;
    MPI_Datatype filetype;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    bufsize = FILESIZE / nprocs;
    nints = bufsize / sizeof(int);

    buf = (int *) malloc(nints * sizeof(int));
    readbuf = (int *) malloc(nints * sizeof(int));

    if (buf == NULL || readbuf == NULL) {
        fprintf(stderr, "Memory allocation failed on rank %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i < nints; i++) {
        buf[i] = rank;
        readbuf[i] = -1;
    }

    num_blocks = nints / INTS_PER_BLK;
    MPI_Type_vector(num_blocks, INTS_PER_BLK, nprocs * INTS_PER_BLK,
                    MPI_INT, &filetype);
    MPI_Type_commit(&filetype);

    MPI_File_open(MPI_COMM_WORLD, "collective_io.dat",
                  MPI_MODE_CREATE | MPI_MODE_WRONLY,
                  MPI_INFO_NULL, &fh);

    MPI_File_set_view(fh,
                      rank * INTS_PER_BLK * sizeof(int),
                      MPI_INT,
                      filetype,
                      "native",
                      MPI_INFO_NULL);

    MPI_File_write_all(fh, buf, nints, MPI_INT, MPI_STATUS_IGNORE);
    MPI_File_close(&fh);

    MPI_File_open(MPI_COMM_WORLD, "collective_io.dat",
                  MPI_MODE_RDONLY,
                  MPI_INFO_NULL, &fh);

    MPI_File_set_view(fh,
                      rank * INTS_PER_BLK * sizeof(int),
                      MPI_INT,
                      filetype,
                      "native",
                      MPI_INFO_NULL);

    MPI_File_read_all(fh, readbuf, nints, MPI_INT, MPI_STATUS_IGNORE);
    MPI_File_close(&fh);

    printf("Rank %d read: ", rank);
    for (i = 0; i < (nints < 8 ? nints : 8); i++) {
        printf("%d ", readbuf[i]);
    }
    printf("\n");

    MPI_Type_free(&filetype);
    free(buf);
    free(readbuf);

    MPI_Finalize();
    return 0;
}
