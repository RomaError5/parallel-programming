#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <string>

#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>

using namespace std;
using namespace chrono;
using Matrix = vector<vector<int>>;

// ---------- Чтение матрицы ----------
Matrix readMatrix(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open " << filename << endl;
        exit(1);
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
    int n = (int)mat.size();
    file << n << endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            file << mat[i][j] << (j == n - 1 ? "" : " ");
        file << endl;
    }
}

// ---------- Проверка ошибок OpenCL ----------
void checkError(cl_int err, const string& msg) {
    if (err != CL_SUCCESS) {
        cerr << "OpenCL error: " << msg << " (code " << err << ")" << endl;
        exit(1);
    }
}

// ---------- Основная функция ----------
int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 6) {
        cerr << "Usage: " << argv[0] << " A.txt B.txt result.txt [block_cols [block_rows]]" << endl;
        cerr << "  block_cols - work-group width (x-dimension), default 16" << endl;
        cerr << "  block_rows - work-group height (y-dimension), default = block_cols" << endl;
        return 1;
    }

    string fileA = argv[1];
    string fileB = argv[2];
    string fileC = argv[3];
    int block_cols = 16, block_rows = 16;
    if (argc >= 5) {
        block_cols = stoi(argv[4]);
        block_rows = (argc == 6) ? stoi(argv[5]) : block_cols;
    }

    // --- Чтение матриц ---
    cout << "Reading A from " << fileA << " ..." << endl;
    auto A_host = readMatrix(fileA);
    cout << "Reading B from " << fileB << " ..." << endl;
    auto B_host = readMatrix(fileB);
    int n = (int)A_host.size();
    if (n != (int)B_host.size()) {
        cerr << "Matrix size mismatch!" << endl;
        return 1;
    }
    cout << "Matrix size: " << n << " x " << n << endl;

    // --- Подготовка данных: плоские векторы ---
    vector<int> A_flat(n * n);
    vector<int> B_flat(n * n);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            A_flat[i * n + j] = A_host[i][j];
            B_flat[i * n + j] = B_host[i][j];
        }

    // --- Инициализация OpenCL ---
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;

    // Получаем первую платформу
    err = clGetPlatformIDs(1, &platform, nullptr);
    checkError(err, "clGetPlatformIDs");
    // Получаем первое устройство (обычно Intel GPU, если есть)
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    if (err == CL_DEVICE_NOT_FOUND) {
        cerr << "No GPU device, falling back to CPU" << endl;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
        checkError(err, "clGetDeviceIDs CPU");
    }

    // Выводим информацию об устройстве
    char deviceName[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(deviceName), deviceName, nullptr);
    cout << "Using device: " << deviceName << endl;

    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");
    queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
    checkError(err, "clCreateCommandQueue");

    // --- Код ядра OpenCL (в виде строки) ---
    const char* kernelSource = R"(
__kernel void matmul(__global const int* A, __global const int* B, __global int* C, int n) {
    int row = get_global_id(0);
    int col = get_global_id(1);
    if (row < n && col < n) {
        int sum = 0;
        for (int k = 0; k < n; ++k) {
            sum += A[row * n + k] * B[k * n + col];
        }
        C[row * n + col] = sum;
    }
}
)";

    // Создаём программу
    program = clCreateProgramWithSource(context, 1, &kernelSource, nullptr, &err);
    checkError(err, "clCreateProgramWithSource");
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // Вывод лога ошибок
        size_t logSize;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        vector<char> log(logSize + 1);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        cerr << "Build error: " << log.data() << endl;
        exit(1);
    }
    kernel = clCreateKernel(program, "matmul", &err);
    checkError(err, "clCreateKernel");

    // --- Выделение памяти на устройстве ---
    cl_mem d_A = clCreateBuffer(context, CL_MEM_READ_ONLY, n * n * sizeof(int), nullptr, &err);
    cl_mem d_B = clCreateBuffer(context, CL_MEM_READ_ONLY, n * n * sizeof(int), nullptr, &err);
    cl_mem d_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, n * n * sizeof(int), nullptr, &err);
    checkError(err, "clCreateBuffer");

    // Копирование данных
    err = clEnqueueWriteBuffer(queue, d_A, CL_TRUE, 0, n * n * sizeof(int), A_flat.data(), 0, nullptr, nullptr);
    err |= clEnqueueWriteBuffer(queue, d_B, CL_TRUE, 0, n * n * sizeof(int), B_flat.data(), 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer");

    // Установка аргументов ядра
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_A);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_B);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_C);
    clSetKernelArg(kernel, 3, sizeof(int), &n);

    size_t max_work_group_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, nullptr);
    checkError(err, "clGetDeviceInfo max work group size");
    cout << "Max work group size: " << max_work_group_size << endl;

    // Корректировка блоков, если превышает лимит
    int used_cols = block_cols, used_rows = block_rows;
    if (used_cols * used_rows > (int)max_work_group_size) {
        // Пытаемся сохранить соотношение сторон, уменьшая пропорционально
        double ratio = (double)used_cols / used_rows;
        int total = used_cols * used_rows;
        int new_total = (int)max_work_group_size;
        int new_cols = (int)sqrt(new_total * ratio);
        int new_rows = new_total / new_cols;
        if (new_cols * new_rows > new_total) new_cols--;
        used_cols = new_cols;
        used_rows = new_rows;
        cout << "Warning: requested block size " << block_cols << "x" << block_rows
             << " exceeds limit. Adjusted to " << used_cols << "x" << used_rows << endl;
    }

    size_t localWorkSize[2] = { (size_t)used_cols, (size_t)used_rows };
    size_t globalWorkSize[2] = {
        ((n + used_cols - 1) / used_cols) * used_cols,
        ((n + used_rows - 1) / used_rows) * used_rows
    };

    auto start = high_resolution_clock::now();
    err = clEnqueueNDRangeKernel(queue, kernel, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel");
    clFinish(queue); // дождаться завершения
    auto end = high_resolution_clock::now();
    double elapsed = duration<double>(end - start).count();

    // --- Чтение результата ---
    vector<int> C_flat(n * n);
    err = clEnqueueReadBuffer(queue, d_C, CL_TRUE, 0, n * n * sizeof(int), C_flat.data(), 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer");

    // Преобразование обратно в матрицу
    Matrix C_host(n, vector<int>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            C_host[i][j] = C_flat[i * n + j];

    cout << fixed << setprecision(6);
    cout << "Execution time (OpenCL kernel only): " << elapsed << " seconds" << endl;
    cout << "Local work group size: " << used_cols << " x " << used_rows << endl;

    writeMatrix(fileC, C_host);
    cout << "Result saved to " << fileC << endl;

    // --- Освобождение ресурсов ---
    clReleaseMemObject(d_A);
    clReleaseMemObject(d_B);
    clReleaseMemObject(d_C);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}