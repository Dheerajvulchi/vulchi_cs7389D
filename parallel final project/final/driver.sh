#!/bin/bash
# driver.sh — Build and run all 4 variants on LS6
# Run from inside an idev session on a compute node
# Usage: bash driver.sh

set -e
ROOT="$(cd "$(dirname "$0")"; pwd)"
INPUT="$ROOT/input/input_4k.ppm"

module reset
module load gcc/11.2.0
module load impi/19.0.9
module load python3

mkdir -p "$ROOT/output/seq" "$ROOT/output/omp" \
         "$ROOT/output/mpi" "$ROOT/output/hybrid" \
         "$ROOT/results"

# Generate image if missing
if [ ! -f "$INPUT" ]; then
    echo "[*] Generating 4K test image..."
    python3 "$ROOT/scripts/gen_image.py"
fi

# Build
echo "[BUILD]"
cd "$ROOT/sequential" && g++ -O2 -std=c++14 -I../core -o seq_image seq_image.cpp && echo "  SEQ OK"
cd "$ROOT/openmp"     && g++ -O2 -std=c++14 -fopenmp -I../core -o omp_image omp_image.cpp && echo "  OMP OK"
cd "$ROOT/mpi"        && mpicxx -O2 -std=c++14 -I../core -o mpi_image mpi_image.cpp && echo "  MPI OK"
cd "$ROOT/hybrid"     && mpicxx -O2 -std=c++14 -fopenmp -I../core -o hybrid_image hybrid_image.cpp && echo "  HYBRID OK"

# Run
echo ""
echo "[RUN]"
cd "$ROOT/sequential" && ./seq_image "$INPUT" "$ROOT/output/seq/output_seq.ppm"
cd "$ROOT/openmp"     && ./omp_image "$INPUT" "$ROOT/output/omp/output_omp.ppm" 4
cd "$ROOT/mpi"        && ibrun -np 4 ./mpi_image "$INPUT" "$ROOT/output/mpi/output_mpi.ppm"
cd "$ROOT/hybrid"     && OMP_NUM_THREADS=4 ibrun -np 2 ./hybrid_image "$INPUT" "$ROOT/output/hybrid/output_hybrid.ppm" 4

# Correctness
echo ""
echo "[CORRECTNESS]"
cd "$ROOT"
diff output/seq/output_seq.ppm output/omp/output_omp.ppm     && echo "  OMP    : MATCH" || echo "  OMP    : MISMATCH"
diff output/seq/output_seq.ppm output/mpi/output_mpi.ppm     && echo "  MPI    : MATCH" || echo "  MPI    : MISMATCH"
diff output/seq/output_seq.ppm output/hybrid/output_hybrid.ppm && echo "  HYBRID : MATCH" || echo "  HYBRID : MISMATCH"

echo ""
echo "[ANALYSIS]"
python3 "$ROOT/analytics/analyze.py" "$ROOT/results/timings_sweep.csv"
