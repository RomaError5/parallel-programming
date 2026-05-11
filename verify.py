import sys
import numpy as np

def read_matrix(filename):
    """Читает матрицу из файла в формате: первая строка - размер, затем строки с числами."""
    with open(filename, 'r') as f:
        lines = f.readlines()
    n = int(lines[0].strip())
    mat = []
    for i in range(1, n+1):
        row = list(map(float, lines[i].strip().split()))
        if len(row) != n:
            raise ValueError(f"Неверное количество элементов в строке {i} файла {filename}")
        mat.append(row)
    return np.array(mat)

def main():
    if len(sys.argv) != 4:
        print("Использование: python verify.py <matrix1.txt> <matrix2.txt> <result.txt>")
        sys.exit(1)

    fileA, fileB, fileC = sys.argv[1], sys.argv[2], sys.argv[3]

    A = read_matrix(fileA)
    B = read_matrix(fileB)
    C_prog = read_matrix(fileC)

    # Вычисляем эталонное произведение с помощью numpy
    C_ref = A @ B

    # Сравнение с допуском (для учёта возможных погрешностей double)
    if np.allclose(C_prog, C_ref, rtol=1e-9, atol=1e-12):
        print("Верификация пройдена: результат программы совпадает с эталоном (numpy).")
        return 0
    else:
        print("Ошибка верификации: результат не совпадает с эталоном.")
        diff = np.abs(C_prog - C_ref)
        print(f"Максимальное абсолютное отклонение: {np.max(diff)}")
        return 1

if __name__ == "__main__":
    sys.exit(main())