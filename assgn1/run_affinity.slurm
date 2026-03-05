#!/bin/bash
#SBATCH -A ASC23013
#SBATCH -J omp_affinity
#SBATCH -o affinity.out
#SBATCH -e affinity.err
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --cpus-per-task=64
#SBATCH -p normal
#SBATCH -t 00:10:00

# go to the folder where job was submitted
cd $SLURM_SUBMIT_DIR

# load compiler
module reset
module load gcc

# compile program
make clean
make

# problem size
N=10000000

# use 64 threads
export OMP_NUM_THREADS=64

# affinity tests
for PL in cores sockets
do
  for BIND in close spread
  do
    export OMP_PLACES=$PL
    export OMP_PROC_BIND=$BIND

    echo "Run: OMP_PLACES=$OMP_PLACES OMP_PROC_BIND=$OMP_PROC_BIND OMP_NUM_THREADS=$OMP_NUM_THREADS N=$N"

    ./compute_pi $N

    echo "--------------------------------------"
  done
done
