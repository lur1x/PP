#define _USE_MATH_DEFINES

#include <chrono>
#include <iostream>
#include <cmath>
#include <omp.h>

double ComputePiSequential(const long long iterations) 
{
    double pi = 0.0;

    for (long long i = 0; i < iterations; ++i) 
    {
        double denominator = 2.0 * i + 1.0;
        double term = (i % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
        pi += term;
    }

    return pi * 4.0;
}

double ComputePiParallelRace(const long long iterations) 
{
    double pi = 0.0;

    #pragma omp parallel for
    for (long long i = 0; i < iterations; ++i) 
    {
        double denominator = 2.0 * i + 1.0;
        double term = (i % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
        pi += term;
    }

    return pi * 4.0;
}

double ComputePiParallelAtomic(const long long iterations) 
{
    double pi = 0.0;

    #pragma omp parallel for
    for (long long i = 0; i < iterations; ++i) 
    {
        double denominator = 2.0 * i + 1.0;
        double term = (i % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;

    #pragma omp atomic
        pi += term;
    }

    return pi * 4.0;
}

double ComputePiParallelReduction(const long long iterations) 
{
    double pi = 0.0;

    #pragma omp parallel for reduction(+:pi)
    for (long long i = 0; i < iterations; ++i) 
    {
        double denominator = 2.0 * i + 1.0;
        double term = (i % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
        pi += term;
    }

    return pi * 4.0;
}


int main() 
{
    const long long num_iterations = 10000000;

    std::cout << "Comparison of PI calculation methods ("
        << num_iterations << " iterations):\n" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    double result_seq = ComputePiSequential(num_iterations);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seq = end_time - start_time;
    std::cout << "Sequential method: " << result_seq
        << " (time: " << elapsed_seq.count() << " s)" << std::endl;

    start_time = std::chrono::high_resolution_clock::now();
    double result_race = ComputePiParallelRace(num_iterations);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_race = end_time - start_time;
    std::cout << "Parallel (with data race): " << result_race
        << " (time: " << elapsed_race.count() << " s)" << std::endl;

    start_time = std::chrono::high_resolution_clock::now();
    double result_atomic = ComputePiParallelAtomic(num_iterations);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_atomic = end_time - start_time;
    std::cout << "Parallel (atomic): " << result_atomic
        << " (time: " << elapsed_atomic.count() << " s)" << std::endl;

    start_time = std::chrono::high_resolution_clock::now();
    double result_reduction = ComputePiParallelReduction(num_iterations);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_reduction = end_time - start_time;
    std::cout << "Parallel (reduction): " << result_reduction
        << " (time: " << elapsed_reduction.count() << " s)" << std::endl;

    std::cout << "\nExact PI value: " << M_PI << std::endl;


    system("pause");

    return 0;
}