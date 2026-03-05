#!/bin/bash
#SBATCH -A ASC23013
#SBATCH -J omp_scaling
#SBATCH -o scaling.out
#SBATCH -e scaling.err
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --cpus-per-task=64
#SBATCH -p normal
#SBATCH -t 00:10:00

cd $SLURM_SUBMIT_DIR

module load gcc

N=10000000

for T in 1 2 4 8 16 32 64
do
  export OMP_NUM_THREADS=$T
  echo "Threads = $T"
  ./compute_pi $N
done
