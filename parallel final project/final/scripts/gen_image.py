#!/usr/bin/env python3
import sys, os, math

def gen_ppm(width, height, path):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, 'w') as f:
        f.write(f"P3\n{width} {height}\n255\n")
        for y in range(height):
            row = []
            for x in range(width):
                r = int(127 + 127 * math.sin(x * 0.02))
                g = int(127 + 127 * math.cos(y * 0.02))
                b = int(127 + 127 * math.sin((x + y) * 0.01))
                row.append(f"{r} {g} {b}")
            f.write("\n".join(row) + "\n")
    kb = os.path.getsize(path) // 1024
    print(f"  Generated {path}  ({width}x{height}, {kb} KB)")

if __name__ == "__main__":
    base = os.path.join(os.path.dirname(__file__), "..", "input")
    w = int(sys.argv[1]) if len(sys.argv) > 1 else 3840
    h = int(sys.argv[2]) if len(sys.argv) > 2 else 2160
    print("Generating test images...")
    gen_ppm(w, h,       os.path.join(base, "input_4k.ppm"))
    gen_ppm(1920, 1080, os.path.join(base, "input_1080p.ppm"))
    gen_ppm(512,  512,  os.path.join(base, "input_small.ppm"))
    print("Done.")
