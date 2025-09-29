#include <mpi.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <iomanip>

// Función para realizar un ping-pong simple
void ping_pong(int rank, int partner_rank, int iterations) {
    const int DATA_SIZE = 1;  // Enviamos un solo entero
    int send_data = 42;
    int recv_data;
    MPI_Status status;

    if (rank == 0) {
        // Proceso 0: inicia el ping-pong
        for (int i = 0; i < iterations; i++) {
            MPI_Send(&send_data, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);
            MPI_Recv(&recv_data, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, &status);
        }
    } else {
        // Proceso 1: responde al ping-pong
        for (int i = 0; i < iterations; i++) {
            MPI_Recv(&recv_data, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, &status);
            MPI_Send(&send_data, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 2) {
        if (world_rank == 0) {
            std::cerr << "Este programa requiere al menos 2 procesos MPI" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    int partner_rank = (world_rank == 0) ? 1 : 0;
    const int MAX_ITERATIONS = 1000000;
    
    if (world_rank == 0) {
        std::cout << "=== Experimento Ping-Pong MPI ===" << std::endl;
        std::cout << "Buscando el minimo de iteraciones para clock() != 0..." << std::endl;
    }

    // Experimento 1: Encontrar el minimo de iteraciones para clock() != 0
    if (world_rank == 0) {
        std::cout << "\n--- Prueba con clock() ---" << std::endl;
        
        for (int iterations = 1; iterations <= MAX_ITERATIONS; iterations *= 10) {
            clock_t start_clock = clock();
            ping_pong(world_rank, partner_rank, iterations);
            clock_t end_clock = clock();
            
            double cpu_time = ((double)(end_clock - start_clock)) / CLOCKS_PER_SEC;
            
            std::cout << "Iteraciones: " << std::setw(8) << iterations 
                      << " | Tiempo clock(): " << std::scientific << cpu_time 
                      << " segundos" << std::endl;
            
            if (cpu_time > 0.0) {
                std::cout << ">>> Primer tiempo no-cero encontrado con " 
                          << iterations << " iteraciones" << std::endl;
                break;
            }
            
            if (iterations * 10 > MAX_ITERATIONS && iterations < MAX_ITERATIONS) {
                iterations = MAX_ITERATIONS;
            }
        }
    } else {
        // Proceso 1 solo participa en la comunicación
        for (int iterations = 1; iterations <= MAX_ITERATIONS; iterations *= 10) {
            ping_pong(world_rank, partner_rank, iterations);
            if (iterations * 10 > MAX_ITERATIONS && iterations < MAX_ITERATIONS) {
                iterations = MAX_ITERATIONS;
            }
        }
    }

    // Sincronizar antes del siguiente experimento
    MPI_Barrier(MPI_COMM_WORLD);

    // Experimento 2: Comparación detallada entre clock() y MPI_Wtime()
    if (world_rank == 0) {
        std::cout << "\n--- Comparación clock() vs MPI_Wtime() ---" << std::endl;
        std::cout << std::setw(12) << "Iteraciones" 
                  << std::setw(15) << "clock() (s)" 
                  << std::setw(15) << "MPI_Wtime() (s)" 
                  << std::setw(15) << "Diferencia" 
                  << std::setw(20) << "Tiempo por ping-pong (μs)" 
                  << std::endl;
        std::cout << std::string(80, '-') << std::endl;
    }

    const int TEST_ITERATIONS[] = {100, 1000, 10000, 100000};
    const int NUM_TESTS = sizeof(TEST_ITERATIONS) / sizeof(TEST_ITERATIONS[0]);

    for (int test = 0; test < NUM_TESTS; test++) {
        int iterations = TEST_ITERATIONS[test];
        
        // Medir con clock()
        MPI_Barrier(MPI_COMM_WORLD);
        clock_t start_clock = clock();
        ping_pong(world_rank, partner_rank, iterations);
        clock_t end_clock = clock();
        double cpu_time = ((double)(end_clock - start_clock)) / CLOCKS_PER_SEC;

        // Medir con MPI_Wtime()
        MPI_Barrier(MPI_COMM_WORLD);
        double start_mpi = MPI_Wtime();
        ping_pong(world_rank, partner_rank, iterations);
        double end_mpi = MPI_Wtime();
        double wall_time = end_mpi - start_mpi;

        if (world_rank == 0) {
            double difference = wall_time - cpu_time;
            double time_per_pingpong = (wall_time * 1e6) / iterations;  // Microsegundos
            
            std::cout << std::setw(12) << iterations 
                      << std::setw(15) << std::scientific << cpu_time 
                      << std::setw(15) << std::scientific << wall_time 
                      << std::setw(15) << std::scientific << difference
                      << std::setw(20) << std::fixed << std::setprecision(3) << time_per_pingpong
                      << std::endl;
        }
    }

    // Experimento 3: Análisis de la resolución temporal
    if (world_rank == 0) {
        std::cout << "\n--- Análisis de Resolución Temporal ---" << std::endl;
        
        // Probamos con números pequeños de iteraciones
        for (int iterations = 1; iterations <= 100; iterations++) {
            clock_t start_clock = clock();
            ping_pong(world_rank, partner_rank, iterations);
            clock_t end_clock = clock();
            
            double cpu_time = ((double)(end_clock - start_clock)) / CLOCKS_PER_SEC;
            
            if (cpu_time > 0.0) {
                std::cout << "Resolución mínima detectable con clock(): " 
                          << iterations << " iteraciones" << std::endl;
                std::cout << "Tiempo correspondiente: " << cpu_time << " segundos" << std::endl;
                break;
            }
        }
        
        // Probamos la resolución de MPI_Wtime con una sola iteración
        double start_mpi = MPI_Wtime();
        ping_pong(world_rank, partner_rank, 1);
        double end_mpi = MPI_Wtime();
        double single_pingpong_time = end_mpi - start_mpi;
        
        std::cout << "Tiempo de un solo ping-pong (MPI_Wtime): " 
                  << std::scientific << single_pingpong_time << " segundos" << std::endl;
    } else {
        // Proceso 1 participa en las mediciones
        for (int iterations = 1; iterations <= 100; iterations++) {
            ping_pong(world_rank, partner_rank, iterations);
        }
        ping_pong(world_rank, partner_rank, 1);
    }

    if (world_rank == 0) {
        std::cout << "\n=== Conclusiones ===" << std::endl;
        std::cout << "1. clock() mide tiempo de CPU, MPI_Wtime() mide tiempo real (wall time)" << std::endl;
        std::cout << "2. Para operaciones de E/S/red, MPI_Wtime() es más apropiado" << std::endl;
        std::cout << "3. La diferencia muestra el tiempo gastado en espera de comunicación" << std::endl;
    }

    MPI_Finalize();
    return 0;
}