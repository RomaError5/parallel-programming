#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <mpi.h>

using namespace std;
using namespace chrono;
using Matrix = vector<vector<int>>;

Matrix generateMatrix(int n) {
    Matrix mat(n, vector<int>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            mat[i][j] = rand() % 100;
    return mat;
}

// Запись матрицы в файл (только для процесса 0)
void writeMatrix(const string& filename, const Matrix& mat) {
    ofstream file(filename);
    int n = mat.size();
    file << n << endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            file << mat[i][j] << (j == n - 1 ? "" : " ");
        file << endl;
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0) {
            cerr << "Usage: mpirun -np N " << argv[0] << " <matrix_size>" << endl;
            cerr << "Example: mpirun -np 4 ./program 2000" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]);

    // Процесс 0 генерирует матрицы A и B
    Matrix A, B;
    if (rank == 0) {
        srand(time(nullptr));
        A = generateMatrix(n);
        B = generateMatrix(n);
    }

    // Распределение строк A между процессами
    int rows_per_proc = n / size;
    int remainder = n % size;
    int start_row = rank * rows_per_proc + min(rank, remainder);
    int end_row = start_row + rows_per_proc + (rank < remainder ? 1 : 0);
    int local_rows = end_row - start_row;

    vector<int> local_A(local_rows * n, 0);
    vector<int> B_flat(n * n, 0);
    vector<int> local_C(local_rows * n, 0);

    // Рассылаем B_flat
    if (rank == 0) {
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                B_flat[i * n + j] = B[i][j];
    }
    MPI_Bcast(B_flat.data(), n * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Рассылка строк A
    MPI_Datatype row_type;
    MPI_Type_vector(n, 1, n, MPI_INT, &row_type);
    MPI_Type_commit(&row_type);

    if (rank == 0) {
        for (int p = 0; p < size; ++p) {
            int p_start = p * rows_per_proc + min(p, remainder);
            int p_rows = rows_per_proc + (p < remainder ? 1 : 0);
            if (p == 0) {
                for (int i = 0; i < local_rows; ++i)
                    for (int j = 0; j < n; ++j)
                        local_A[i * n + j] = A[p_start + i][j];
            }
            else {
                for (int i = 0; i < p_rows; ++i) {
                    MPI_Send(A[p_start + i].data(), n, MPI_INT, p, i, MPI_COMM_WORLD);
                }
            }
        }
    }
    else {
        for (int i = 0; i < local_rows; ++i) {
            MPI_Recv(&local_A[i * n], n, MPI_INT, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    MPI_Type_free(&row_type);

    // Локальное умножение
    MPI_Barrier(MPI_COMM_WORLD); // синхронизация перед замером времени
    double start_time = MPI_Wtime();

    for (int i = 0; i < local_rows; ++i) {
        for (int j = 0; j < n; ++j) {
            int sum = 0;
            for (int k = 0; k < n; ++k) {
                sum += local_A[i * n + k] * B_flat[k * n + j];
            }
            local_C[i * n + j] = sum;
        }
    }

    double end_time = MPI_Wtime();
    double elapsed = end_time - start_time;

    // Сбор результата
    vector<int> global_C;
    vector<int> recv_counts(size, 0);
    vector<int> displs(size, 0);

    if (rank == 0) {
        global_C.resize(n * n);
        int offset = 0;
        for (int p = 0; p < size; ++p) {
            int p_rows = rows_per_proc + (p < remainder ? 1 : 0);
            recv_counts[p] = p_rows * n;
            displs[p] = offset;
            offset += recv_counts[p];
        }
    }

    MPI_Gatherv(local_C.data(), local_rows * n, MPI_INT,
        global_C.data(), recv_counts.data(), displs.data(), MPI_INT,
        0, MPI_COMM_WORLD);

    if (rank == 0) {
        Matrix C_mat(n, vector<int>(n));
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                C_mat[i][j] = global_C[i * n + j];

        cout << fixed << setprecision(6);
        cout << "Matrix size: " << n << " x " << n << endl;
        cout << "Execution time: " << elapsed << " seconds" << endl;
        cout << "MPI processes: " << size << endl;
    }

    MPI_Finalize();
    return 0;
}