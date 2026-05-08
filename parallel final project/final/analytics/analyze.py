#!/usr/bin/env python3
import csv, sys, os

SEP = "=" * 64

def load(path):
    rows = []
    with open(path, newline='') as f:
        for row in csv.DictReader(f):
            rows.append({
                'version': row['Version'],
                'workers': int(row['Workers']),
                'ranks':   int(row['Ranks']),
                'threads': int(row['Threads']),
                'compute': float(row['Time_ms']),
                'io_read': float(row['IO_read_ms']),
                'io_write':float(row['IO_write_ms']),
            })
    return rows

def analyze(path):
    data = load(path)
    seq = next(r for r in data if r['version'] == 'Sequential')
    seq_t = seq['compute']

    print(SEP)
    print("  Parallel Image Processing — Performance Analysis Report")
    print(SEP)
    print(f"\nSequential baseline  : {seq_t:.2f} ms  (1 worker)\n")

    from collections import defaultdict
    versions = defaultdict(list)
    for r in data:
        if r['version'] != 'Sequential':
            versions[r['version']].append(r)

    for vname, rows in versions.items():
        rows.sort(key=lambda r: r['workers'])
        print(f"--- {vname} ---")
        print(f"  {'Workers':>8} {'Compute(ms)':>12} {'Speedup':>9} {'Efficiency':>11} {'Eff.Loss':>9}")
        print("  " + "-"*54)
        for r in rows:
            sp = seq_t / r['compute']
            ef = sp / r['workers']
            print(f"  {r['workers']:>8} {r['compute']:>12.2f} {sp:>9.2f}x {ef:>10.1%} {(1-ef):>9.1%}")
        print()

    print(SEP)
    print("  Overhead breakdown (best config per variant)")
    print(SEP)
    print(f"  {'Version':<14} {'Workers':>8} {'Compute':>10} {'IO_read':>10} {'IO_write':>10} {'IO%':>7}")
    print("  " + "-"*60)
    for vname, rows in versions.items():
        best = min(rows, key=lambda r: r['compute'])
        total_io = best['io_read'] + best['io_write']
        total = best['compute'] + total_io
        io_pct = total_io / total * 100 if total > 0 else 0
        print(f"  {vname:<14} {best['workers']:>8} {best['compute']:>10.2f} "
              f"{best['io_read']:>10.2f} {best['io_write']:>10.2f} {io_pct:>6.1f}%")

    all_p = [r for r in data if r['version'] != 'Sequential']
    if all_p:
        best = min(all_p, key=lambda r: r['compute'])
        sp = seq_t / best['compute']
        ef = sp / best['workers']
        print(f"\n  Best overall : {best['version']} "
              f"({best['ranks']} rank(s) x {best['threads']} thread(s)) "
              f"— {best['compute']:.2f} ms  speedup={sp:.2f}x  efficiency={ef:.1%}")

    print(f"\n{SEP}")
    print("  Correctness validation")
    print(SEP)
    base_dir = os.path.join(os.path.dirname(path), '..', 'output')
    checks = [
        ('Sequential', 'seq/output_seq.ppm'),
        ('OpenMP',     'omp/output_omp.ppm'),
        ('MPI',        'mpi/output_mpi.ppm'),
        ('Hybrid',     'hybrid/output_hybrid.ppm'),
    ]
    for name, rel in checks:
        full = os.path.join(base_dir, rel)
        status = f"found ({os.path.getsize(full)//1024} KB)" if os.path.exists(full) else "MISSING"
        print(f"  {name:<14}: {status}")
    print()

if __name__ == '__main__':
    csv_path = sys.argv[1] if len(sys.argv) > 1 else \
               os.path.join(os.path.dirname(__file__), '..', 'results', 'timings_sweep.csv')
    if not os.path.exists(csv_path):
        print(f"Error: '{csv_path}' not found.")
        sys.exit(1)
    analyze(csv_path)
