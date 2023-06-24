import numpy as np
import matplotlib.pyplot as plt

def sdf(center, r, p):
    return np.linalg.norm(center - p) - r

def march(r0, d):
    t = 0
    for _ in range(100):
        dist = sdf([0, 0, -5], 1, r0 + t * d)
        if dist < 0.0001:
            return t + abs(dist)
        else:
            t += dist
    return -1



img = np.zeros((100, 100))
rows, cols = img.shape
for i in range(rows):
    for j in range(cols):
        x = -((j * 2 - cols) / cols)
        y = -((i * 2 - rows) / rows)
        fov = 90
        c = np.array([0, 0, -np.tan(fov/2)])
        r0 = np.array([x, y, 0])
        d = r0 - c
        d = -d / np.linalg.norm(d)
        print(d)

        t = march(r0, d)
        if t > 0:
            img[i, j] = 1;
        

plt.imshow(img, cmap="gray")
plt.show()