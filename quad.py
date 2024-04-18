import numpy as np
import matplotlib.pyplot as plt

xs = []
ys = []

view = 'divfloor.sum'
with open(f'sizes/{view}.csv') as f:
    for line in f.readlines():
        x, y = line.split(',')
        xs.append(int(x))
        ys.append(int(y))

unique_pairs, counts = np.unique((xs, ys), axis=1, return_counts=True)
area = counts*10
c = np.sqrt(area)
plt.clf()
plt.scatter(unique_pairs[0], unique_pairs[1], s=area, alpha=0.1, c=c)

x_trend = range(40)
y_trend = [5*x ** 2 for x in x_trend]
plt.plot(x_trend, y_trend)
plt.savefig(f'{view}.fit.png')