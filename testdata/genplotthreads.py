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
r = importlib.import_module(f"result-sample-threads")

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
    avgs[N] = size * 1_000_000 / np.mean(arr[dist < limit]) / 1024 / 1024
    full[N] = size * 1_000_000 / arr / 1024 / 1024
    
    
ax.set_title('threads')
ax.set_xlabel('Threads count')
ax.set_ylabel('Speed, MiB/s')
keys = sorted(avgs.keys())

print(keys)
print([avgs[k] for k in keys])
print([str(k) for k in keys])
ax.plot(keys, [avgs[k] for k in keys])

sck = []
scf = []
for k in keys:
    sck += [k]*len(full[k])
    scf += list(full[k])
ax.scatter(sck, scf, alpha=0.4)

ax.set_xticks(keys)
ax.set_xticklabels([str(k) for k in keys])

plt.grid(True)
plt.show()