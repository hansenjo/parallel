#!/usr/bin/env python3.9

import sys
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) > 1:
    infilename = sys.argv[1]
else:
    infilename = "benchmark.out"

# Number of analysis threads used
j = np.array([], dtype=int)
# Clock times (ms)
t_init = np.array([], dtype=float)
t_ana = np.array([], dtype=float)
t_out = np.array([], dtype=float)
t_real = np.array([], dtype=float)
# Total CPU time (ms)
t_cpu = np.array([], dtype=float)
# Memory usage: maximum resident set size (bytes)
memusage = np.array([], dtype=float)

infile = open(infilename, 'r')

# Number of events analyzed
nev = int(infile.readline())

for line in infile:
    items = line.split()
    j = np.append(j, int(items[0]))
    t_init = np.append(t_init, float(items[1]))
    t_ana = np.append(t_ana, float(items[2]))
    t_out = np.append(t_out, float(items[3]))
    t_cpu = np.append(t_cpu, float(items[4]))
    t_real = np.append(t_real, float(items[5]))
    memusage = np.append(memusage, float(items[6]))

infile.close()
# Derived quantities
ncores = int((j.size - 2) / 2)
real_rate = 1e3 * nev / t_real  # Analysis rate (Hz)
ideal_rate = 1e3 * nev * j / t_cpu[0]

# Parallelization efficiency in %
efficiency = 100. * real_rate / ideal_rate

# Memory usage analysis
memusage /= 1e6  # Convert to MB
mem_x = j[0: 2 * ncores - 1]
mem_y = memusage[0: 2 * ncores - 1]
fit = np.polyfit(mem_x, mem_y, 1)
# chisq_dof = np.sum((np.polyval(fit, mem_x) - mem_y) ** 2) / (len(mem_x)-2)
# print(f"chisq/dof = {chisq_dof}")
predict = np.poly1d(fit)
fit_memusage = predict(j)
naive_memusage = memusage[0] * j  # Naive scaling, approximating independent processes (ignores sharing)

# Plot results
plt.figure(0)
plt.xlabel('Number of analysis threads')
plt.ylabel('Analysis Rate (Hz)')
plt.title('Parallel Podd Prototype Performance Scaling')
plt.xticks(np.arange(2, 2*ncores + 3, 2))

plt.plot(j, ideal_rate, ".-", label="Ideal rate")
plt.plot(j, real_rate, ".-", label="Actual rate")
plt.axvline(ncores, linestyle="dotted")  # Marker line at number of CPU cores
plt.axvline(ncores * 2, linestyle="dashed")  # Marker lines at number of CPU hyperthreads
plt.legend()
# Text labels with the efficiencies at the number of cores and threads points
plt.annotate(f"{round(efficiency[ncores - 1], 1)}% eff", (j[ncores - 1], real_rate[ncores - 1]),
             textcoords="offset points", xytext=(5, -5), ha="left", va="top")
plt.annotate(f"{round(efficiency[ncores * 2 - 1], 1)}% eff", (j[ncores * 2 - 1], real_rate[ncores * 2 - 1]),
             textcoords="offset points", xytext=(5, -5), ha="left", va="top")
plt.savefig("benchmark-rates.pdf")

plt.figure(1)
plt.title('Parallel Podd Prototype Memory Usage')
plt.xlabel('Number of analysis threads')
plt.ylabel("Memory usage (MB)")
plt.xticks(np.arange(2, 2*ncores + 3, 2))
plt.plot(j, naive_memusage, ".-", label="Multiprocess memory usage (est.)")
plt.plot(j, memusage, ".-", label="Multithreaded memory usage (meas.)", color="C1")
plt.plot(j, fit_memusage, "--r", label="Fit")
plt.ylim(0, memusage[-1] * 1.1)
plt.legend()
# Print estimated memory increase per thread
plt.annotate(f"{round(predict[1], 2)} MB/thread", (j[ncores - 1], predict(ncores)),
             textcoords="offset points", xytext=(5, -5), ha="left", va="top")
plt.savefig("benchmark-memusage.pdf")

plt.show()
