#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>

using namespace std;
using namespace chrono;

// Чтение квадратной матрицы из файла
// Формат: первая строка - размер n, затем n строк по n чисел
vector<vector<double>> readMatrix(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Ошибка: не удалось открыть файл " << filename << endl;
        exit(1);
    }
    int n;
    file >> n;
    vector<vector<double>> mat(n, vector<double>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            file >> mat[i][j];
        }
    }
    file.close();
    return mat;
}

// Запись квадратной матрицы в файл
void writeMatrix(const string& filename, const vector<vector<double>>& mat) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Ошибка: не удалось создать файл " << filename << endl;
        exit(1);
    }
    int n = mat.size();
    file << n << endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            file << fixed << setprecision(6) << mat[i][j];
            if (j != n - 1) file << " ";
        }
        file << endl;
    }
    file.close();
}

// Последовательное перемножение двух квадратных матриц
vector<vector<double>> multiplyMatrices(const vector<vector<double>>& A,
    const vector<vector<double>>& B) {
    int n = A.size();
    vector<vector<double>> C(n, vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < n; ++k) {
            double aik = A[i][k];
            for (int j = 0; j < n; ++j) {
                C[i][j] += aik * B[k][j];
            }
        }
    }
    return C;
}

// Вычисление объёма задачи (количество операций умножения и сложения)
void printTaskVolume(int n) {
    long long multOps = (long long)n * n * n;          // n^3 умножений
    long long addOps = (long long)n * n * (n - 1);     // n^2*(n-1) сложений
    long long totalOps = multOps + addOps;
    long long memoryElements = 3LL * n * n;            // A, B, C – всего 3n^2 элементов
    cout << "Объём задачи:" << endl;
    cout << "  Размер матриц: " << n << " x " << n << endl;
    cout << "  Количество элементов: " << memoryElements << " (входные + выходная)" << endl;
    cout << "  Количество арифметических операций: " << totalOps << " (умножений: " << multOps << ", сложений: " << addOps << ")" << endl;
}

int main(int argc, char* argv[]) {
    // Аргументы: program matrix1.txt matrix2.txt result.txt
    if (argc != 4) {
        cerr << "Использование: " << argv[0] << " <файл_матрицы_A> <файл_матрицы_B> <файл_результата>" << endl;
        return 1;
    }

    string fileA = argv[1];
    string fileB = argv[2];
    string fileC = argv[3];

    // Чтение матриц
    cout << "Чтение матрицы A из " << fileA << " ..." << endl;
    auto A = readMatrix(fileA);
    cout << "Чтение матрицы B из " << fileB << " ..." << endl;
    auto B = readMatrix(fileB);

    int n = A.size();
    if (n != B.size()) {
        cerr << "Ошибка: матрицы разного размера!" << endl;
        return 1;
    }
    cout << "Размер матриц: " << n << " x " << n << endl;

    // Объём задачи
    printTaskVolume(n);

    // Умножение с замером времени
    cout << "Выполняется умножение..." << endl;
    auto start = high_resolution_clock::now();
    auto C = multiplyMatrices(A, B);
    auto end = high_resolution_clock::now();
	chrono::duration<double> duration = end - start;

    cout << fixed << setprecision(6);
    cout << "Время выполнения: " << duration.count() << " секунд" << endl;

    // Запись результата
    cout << "Запись результата в " << fileC << " ..." << endl;
    writeMatrix(fileC, C);

    cout << "Готово." << endl;
    return 0;
}