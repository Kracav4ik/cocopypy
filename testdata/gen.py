import numpy as np

size = 512
for i in range(8):
    print(size, hex(size))
    print('  generating...')
    bytes = np.random.bytes(size)
    print('  writing...')
    with open('sample-%s.bin' % i, 'wb') as f:
        f.write(bytes)
    size *= 8
