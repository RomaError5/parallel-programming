#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <mpi.h>

using namespace std;
using namespace chrono;
using Matrix = vector<vector<int>>;

// ---------- Чтение матрицы ----------
Matrix readMatrix(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open " << filename << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    int n;
    file >> n;
    Matrix mat(n, vector<int>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            file >> mat[i][j];
    return mat;
}

// ---------- Запись матрицы ----------
void writeMatrix(const string& filename, const Matrix& mat) {
    ofstream file(filename);
    int n = mat.size();
    file << n << endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            file << mat[i][j] << (j == n-1 ? "" : " ");
        file << endl;
    }
}

// ---------- Умножение матриц ----------
Matrix sequentialMultiply(const Matrix& A,
                                        const Matrix& B) {
    int n = A.size();
    Matrix C(n, vector<int>(n, 0));
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < n; ++k)
            for (int j = 0; j < n; ++j)
                C[i][j] += A[i][k] * B[k][j];
    return C;
}

// ---------- Главная функция MPI ----------
int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 4) {
        if (rank == 0)
            cerr << "Usage: mpirun -np N " << argv[0]
                 << " A.txt B.txt result.txt" << endl;
        MPI_Finalize();
        return 1;
    }

    string fileA = argv[1];
    string fileB = argv[2];
    string fileC = argv[3];

    // Только процесс 0 читает матрицы и рассылает данные
    int n = 0;
    Matrix A, B;

    if (rank == 0) {
        A = readMatrix(fileA);
        B = readMatrix(fileB);
        n = A.size();
        if (n != (int)B.size()) {
            cerr << "Matrix size mismatch!" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    // Широковещательная рассылка размера n всем процессам
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Каждый процесс выделит место под свои строки A и полную B
    // Распределение строк: процесс r получит строки от start до end-1
    int rows_per_proc = n / size;
    int remainder = n % size;
    int start_row = rank * rows_per_proc + min(rank, remainder);
    int end_row = start_row + rows_per_proc + (rank < remainder ? 1 : 0);
    int local_rows = end_row - start_row;

    // Вектор для локальной части A (local_rows x n)
    vector<int> local_A(local_rows * n, 0);
    // Вектор для всей матрицы B (n x n) – каждый процесс хранит её целиком
    vector<int> B_flat(n * n, 0);
    // Результат для локальных строк (local_rows x n)
    vector<int> local_C(local_rows * n, 0);

    // Процесс 0 рассылает B всем
    if (rank == 0) {
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                B_flat[i * n + j] = B[i][j];
    }
    MPI_Bcast(B_flat.data(), n * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Рассылка строк A каждому процессу (векторный тип данных для строк)
    // Подготовим описатель типа "строка матрицы A" для MPI
    MPI_Datatype row_type;
    MPI_Type_vector(n, 1, n, MPI_INT, &row_type);
    MPI_Type_commit(&row_type);

    if (rank == 0) {
        // Отправляем каждому процессу его строки
        for (int p = 0; p < size; ++p) {
            int p_start = p * rows_per_proc + min(p, remainder);
            int p_rows = rows_per_proc + (p < remainder ? 1 : 0);
            if (p == 0) {
                // копируем свои строки
                for (int i = 0; i < local_rows; ++i)
                    for (int j = 0; j < n; ++j)
                        local_A[i * n + j] = A[p_start + i][j];
            } else {
                // отправляем строки процессу p
                for (int i = 0; i < p_rows; ++i) {
                    MPI_Send(A[p_start + i].data(), n, MPI_INT, p, i, MPI_COMM_WORLD);
                }
            }
        }
    } else {
        // Приём строк
        for (int i = 0; i < local_rows; ++i) {
            MPI_Recv(&local_A[i * n], n, MPI_INT, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    MPI_Type_free(&row_type);

    // Локальное умножение: local_C = local_A * B
    auto start_time = high_resolution_clock::now();

    for (int i = 0; i < local_rows; ++i) {
        for (int j = 0; j < n; ++j) {
            int sum = 0;
            for (int k = 0; k < n; ++k) {
                sum += local_A[i * n + k] * B_flat[k * n + j];
            }
            local_C[i * n + j] = sum;
        }
    }

    auto end_time = high_resolution_clock::now();
    double elapsed = duration<double>(end_time - start_time).count();

    // Сбор результата на процессе 0
    vector<int> global_C;
    vector<int> recv_counts(size, 0);      // количество элементов от каждого процесса
    vector<int> displs(size, 0);

    if (rank == 0) {
        global_C.resize(n * n);
        // Вычисляем смещения для каждого процесса
        int offset = 0;
        for (int p = 0; p < size; ++p) {
            int p_start = p * rows_per_proc + min(p, remainder);
            int p_rows = rows_per_proc + (p < remainder ? 1 : 0);
            recv_counts[p] = p_rows * n;
            displs[p] = offset;
            offset += recv_counts[p];
        }
    }

    // Собираем все части C
    MPI_Gatherv(local_C.data(), local_rows * n, MPI_INT,
                global_C.data(), recv_counts.data(), displs.data(), MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Преобразуем плоский вектор в матрицу для записи
        Matrix C_mat(n, vector<int>(n));
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                C_mat[i][j] = global_C[i * n + j];

        // Замер времени только для процесса 0 (общее время вычислений)
        cout << fixed << setprecision(6);
        cout << "Matrix size (N): " << n << endl;
        cout << "Execution time: " << elapsed << " seconds" << endl;
        cout << "MPI processes: " << size << endl;

        writeMatrix(fileC, C_mat);
        cout << "Result saved to " << fileC << endl;
    }

    MPI_Finalize();
    return 0;
}