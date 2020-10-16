from scipy import stats
import numpy as np
import matplotlib.pyplot as plt
import importlib

def ss(k):
    if k < 1024:
        return f'{k}B'
    k //= 1024
    if k < 1024:
        return f'{k}KiB'
    k //= 1024
    if k < 1024:
        return f'{k}MiB'
    k //= 1024
    return f'{k}GiB'

#for i in range(8):
i = 7
r = importlib.import_module(f"result-sample-{i}-etalon")

#eval(f"import result-sample-{i}")

fig1, ax = plt.subplots()
avgs = {}
full = {}
for d in r.data:
    arr = np.array(r.data[d])
    med = np.median(arr)
    mad = stats.median_absolute_deviation(arr)
    limit = mad * 3.5
    dist = np.abs(arr - med)
    N, buf, size = d
    buf *= 512
    avgs[buf] = size * 1_000_000 / np.mean(arr[dist < limit]) / 1024 / 1024
    full[buf] = size * 1_000_000 / arr / 1024 / 1024
    
    
ax.set_title('1 thread')
ax.set_xlabel('Buffer size, bytes')
ax.set_ylabel('Speed, MiB/s')
keys = sorted(avgs.keys())

print(keys)
print([avgs[k] for k in keys])
print([ss(k) for k in keys])
ax.plot(keys, [avgs[k] for k in keys])

sck = []
scf = []
for k in keys:
    sck += [k]*len(full[k])
    scf += list(full[k])
ax.scatter(sck, scf, alpha=0.4)

ax.set_xscale('log', basex=2)
ax.set_xticks(keys)
ax.set_xticklabels([ss(k) for k in keys])

plt.grid(True)
plt.show()