import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as im
import copy


image_size = 1024



# bump = np.array([[[v, v, v] for v in row] for row in bump])
# print(diffuse)
dirs = ["metal2", "wood", "cardboard"]
rows = []
for fdir in dirs:
    diffuse = im.imread(f"{fdir}/diffuse.jpg")
    normal = im.imread(f"{fdir}/normal.jpg")[:, :, :3]
    row = np.hstack((diffuse, normal))
    rows.append(row)
img = np.vstack(tuple(rows))
print(img.shape)
plt.imshow(img)
plt.show()

plt.imsave("lumped.png", img)