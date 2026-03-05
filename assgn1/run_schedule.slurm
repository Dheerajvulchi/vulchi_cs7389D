#!/bin/bash
#SBATCH -A ASC23013
#SBATCH -J omp_schedule
#SBATCH -o schedule.out
#SBATCH -e schedule.err
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --cpus-per-task=64
#SBATCH -p normal
#SBATCH -t 00:10:00

cd $SLURM_SUBMIT_DIR

module load gcc

N=10000000
export OMP_NUM_THREADS=64

for S in static dynamic guided
do
  export OMP_SCHEDULE=$S
  echo "Schedule = $S"
  ./compute_pi $N
done
