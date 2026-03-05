import matplotlib.pyplot as plt

# -----------------------------
# Thread Scaling Data
# -----------------------------
threads = [1, 2, 4, 8, 16, 32, 64]
times = [0.013937, 0.007136, 0.004106, 0.002001, 0.001501, 0.001665, 0.001971]

plt.figure()
plt.plot(threads, times, marker='o')
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (seconds)")
plt.title("Thread Scaling Performance")
plt.grid(True)
plt.savefig("thread_scaling.png")
plt.close()

# -----------------------------
# Affinity Comparison
# -----------------------------
affinity_labels = [
    "cores-close",
    "cores-spread",
    "sockets-close",
    "sockets-spread"
]

affinity_times = [
    0.002108,
    0.001542,
    0.001788,
    0.001784
]

plt.figure()
plt.bar(affinity_labels, affinity_times)
plt.xlabel("Affinity Policy")
plt.ylabel("Execution Time (seconds)")
plt.title("Thread Placement Performance")
plt.xticks(rotation=20)
plt.grid(axis='y')
plt.savefig("affinity_comparison.png")
plt.close()

# -----------------------------
# Scheduling Comparison
# -----------------------------
schedule_labels = ["static", "dynamic", "guided"]
schedule_times = [0.002138, 0.002206, 0.001794]

plt.figure()
plt.bar(schedule_labels, schedule_times)
plt.xlabel("Scheduling Policy")
plt.ylabel("Execution Time (seconds)")
plt.title("Scheduling Policy Performance")
plt.grid(axis='y')
plt.savefig("schedule_comparison.png")
plt.close()

print("Plots generated successfully.")
