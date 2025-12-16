#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <omp.h>

using Matrix = std::vector<std::vector<double>>;

Matrix GenerateRandomMatrix(int n) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    Matrix Matrix(n, std::vector<double>(n));
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++) 
        {
            Matrix[i][j] = dis(gen);
        }
    }
    return Matrix;
}

Matrix MultiplyMatricesSequential(const Matrix& A, const Matrix& B) 
{
    int n = A.size();
    Matrix C(n, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++) 
        {
            double sum = 0.0;
            for (int k = 0; k < n; k++) 
            {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
    return C;
}

Matrix MultiplyMatricesParallelRow(const Matrix& A, const Matrix& B)
{
    int n = A.size();
    Matrix C(n, std::vector<double>(n, 0.0));

    #pragma omp parallel for
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++) 
        {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
            {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
    return C;
}


bool VerifyResults(const Matrix& C1, const Matrix& C2, double epsilon = 1e-6) 
{
    int n = C1.size();
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < n; j++)
        {
            if (std::abs(C1[i][j] - C2[i][j]) > epsilon) 
            {
                return false;
            }
        }
    }
    return true;
}

int main()
{
    setlocale(LC_ALL, "RU");
    const int n = 50;
    const int numThreads = omp_get_max_threads();

    std::cout << "=== Параллельное умножение матриц ===\n";
    std::cout << "Размер матриц: " << n << "x" << n << "\n";
    std::cout << "Количество потоков: " << numThreads << "\n";
    std::cout << std::fixed << std::setprecision(6);

    std::cout << "\nГенерация матриц...\n";
    auto A = GenerateRandomMatrix(n);
    auto B = GenerateRandomMatrix(n);

    std::cout << "\nПоследовательное умножение...\n";
    auto start = std::chrono::high_resolution_clock::now();
    auto C_seq = MultiplyMatricesSequential(A, B);
    auto end = std::chrono::high_resolution_clock::now();
    auto seq_time = std::chrono::duration<double>(end - start).count();
    std::cout << "Время: " << seq_time << " сек\n";

    std::cout << "\nПараллельное умножение (распараллеливание по строкам)...\n";
    start = std::chrono::high_resolution_clock::now();
    auto C_par_row = MultiplyMatricesParallelRow(A, B);
    end = std::chrono::high_resolution_clock::now();
    auto par_row_time = std::chrono::duration<double>(end - start).count();
    std::cout << "Время: " << par_row_time << " сек\n";


    std::cout << "\nПроверка корректности...\n";
    bool valid_row = VerifyResults(C_seq, C_par_row);

    std::cout << "Результат parallel row: " << (valid_row ? "OK" : "ERROR") << "\n";

    system("pause");

    return 0;
}