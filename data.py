import re

processes = [2, 4, 8, 16, 32]
bench_n = [100, 10000, 100000, 1000000, 100000000, 1000000000]
pattern = r"\d+\.\d+s"


def get_avg(p, n):
    sum = 0
    with open(f"./data/result_{p}_{n}.txt", "r") as f:
        content = f.read()
        matches = re.findall(pattern, content)
        for match in matches:
            sum += float(match[:-1])

    return sum / (p - 1)


if __name__ == "__main__":
    with open("./data/averages.txt", "w") as f:
        for p in processes:
            for n in bench_n:
                avg = get_avg(p, n)
                f.write(f"p = {p}, n = {n}, avg = {avg:.9f}\n")
