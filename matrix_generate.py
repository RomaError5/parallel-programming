import numpy as np
import sys

def generate_matrix(n, filename):
    matrix = np.random.randint(0, 100, (n, n))
    with open(filename, 'w') as f:
        f.write(f"{n}\n")
        for row in matrix:
            f.write(" ".join(f"{x}" for x in row) + "\n")

if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 100

    generate_matrix(n, "A.txt")
    generate_matrix(n, "B.txt")

    print(f"Сгенерированы 2 {n}x{n} матрицы.")