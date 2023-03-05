import matplotlib.pyplot as plt

x = [2, 4, 8, 16, 32]
y = [25.738011120, 8.059124947, 3.741562571, 1.803056319, 1.613346500]

n = 1000000000
plt.plot(x, [t * 1000 for t in y], marker='o')
plt.xlabel("Number of processes")
plt.xticks(x)
plt.ylabel("Time to merge (ms)")
plt.title(f"Time vs # of Processes ({n} elements)")
plt.savefig(f"./graph_{n}.png")
