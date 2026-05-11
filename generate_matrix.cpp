#include <iostream>
#include <fstream>
#include <random>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Использование: " << argv[0] << " <размер_матрицы> <выходной_файл>" << endl;
        cerr << "Пример: " << argv[0] << " 100 A.txt" << endl;
        return 1;
    }

    int n = stoi(argv[1]);
    string filename = argv[2];

    if (n <= 0) {
        cerr << "Ошибка: размер матрицы должен быть положительным числом." << endl;
        return 1;
    }

    // Генератор случайных целых чисел в диапазоне [0, 99]
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, 99);

    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Ошибка: не удалось создать файл " << filename << endl;
        return 1;
    }

    file << n << endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            file << dist(gen);
            if (j != n - 1) file << " ";
        }
        file << endl;
    }

    file.close();
    cout << "Матрица " << n << "x" << n << " со случайными целыми числами [0,99] записана в " << filename << endl;
    return 0;
}