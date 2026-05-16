#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;
using namespace chrono;

// Чтение квадратной матрицы из файла
// Формат: первая строка - размер n, затем n строк по n чисел
vector<vector<int>> readMatrix(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Failed to open file " << filename << endl;
        exit(1);
    }
    int n;
    file >> n;
    vector<vector<int>> mat(n, vector<int>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            file >> mat[i][j];
        }
    }
    file.close();
    return mat;
}

// Запись квадратной матрицы в файл
void writeMatrix(const string& filename, const vector<vector<int>>& mat) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Failed to open file " << filename << endl;
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

// Параллельное перемножение двух квадратных матриц с использованием OpenMP
vector<vector<int>> multiplyMatrices(const vector<vector<int>>& A,
    const vector<vector<int>>& B, int numThreads) {
    int n = A.size();
    vector<vector<int>> C(n, vector<int>(n, 0.0));

    // Распараллеливание внешнего цикла по строкам результирующей матрицы
    #pragma omp parallel for default(none) shared(A, B, C, n)
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int sum = 0;
            for (int k = 0; k < n; ++k) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
    return C;
}

int main(int argc, char* argv[]) {
    // Аргументы: A.txt B.txt result.txt [threads]
    if (argc < 4 || argc > 5) {
        cerr << "Using: " << argv[0] << " <filename_1.txt> <filename_2.txt> <result.txt> [number_threads]" << endl;
        return 1;
    }

    string fileA = argv[1];
    string fileB = argv[2];
    string fileC = argv[3];
    int numThreads = (argc == 5) ? stoi(argv[4]) : 1;

    // Чтение матриц
    cout << "Open matrix A from " << fileA << " ..." << endl;
    auto A = readMatrix(fileA);
    cout << "Open matrix B from " << fileB << " ..." << endl;
    auto B = readMatrix(fileB);

    int n = A.size();
    if (n != B.size()) {
        cerr << "Error: different size matrix!" << endl;
        return 1;
    }
    cout << "Matrix size: " << n << " x " << n << endl;

#ifdef _OPENMP
    omp_set_num_threads(numThreads);
#endif

    auto start = high_resolution_clock::now();
    auto C = multiplyMatrices(A, B, numThreads);
    auto end = high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    cout << "Matrix size (N): " << n << endl;
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "Threads: " << numThreads << endl;

    // Запись результата
    cout << "Recording the result in " << fileC << " ..." << endl;
    writeMatrix(fileC, C);

    cout << "Ready" << endl;
    return 0;
}