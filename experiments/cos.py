import math

vals = []
for i in range(8):
    vals.append([])
    for j in range(8):
        if j == 0:
            vals[i].append(math.sqrt(1.0 / 8.0) * math.cos(math.pi / 8.0 * (i + 0.5) * j))
        else:
            vals[i].append(0.5 * math.cos(math.pi / 8.0 * (i + 0.5) * j))


for i in range(8):
    print vals[i][0], vals[i][1], vals[i][2], vals[i][3], vals[i][4], vals[i][5], vals[i][6], vals[i][7]
    