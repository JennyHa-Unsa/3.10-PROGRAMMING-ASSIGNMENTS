#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

// Función para verificar si un número es potencia de 2
bool is_power_of_two(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

// Algoritmo butterfly para suma global
double butterfly_sum(double local_value, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    double sum = local_value;
    int stages = std::log2(size);
    
    for (int stage = 0; stage < stages; ++stage) {
        int partner = rank ^ (1 << stage);  // XOR para calcular el compañero
        
        double received_value;
        // Intercambiar y sumar con el compañero
        MPI_Sendrecv(&sum, 1, MPI_DOUBLE, partner, 0,
                    &received_value, 1, MPI_DOUBLE, partner, 0,
                    comm, MPI_STATUS_IGNORE);
        
        sum += received_value;
    }
    
    return sum;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Verificar que el número de procesos es potencia de 2
    if (!is_power_of_two(size)) {
        if (rank == 0) {
            std::cerr << "Error: Number of processes must be a power of 2" << std::endl;
            std::cerr << "Current number of processes: " << size << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Cada proceso tiene un valor local (por ejemplo, su rank + 1)
    double local_value = static_cast<double>(rank + 1);
    
    // Calcular suma global usando butterfly
    double global_sum = butterfly_sum(local_value, MPI_COMM_WORLD);
    
    // Verificar el resultado (la suma de 1 + 2 + ... + n = n(n+1)/2)
    if (rank == 0) {
        double expected_sum = size * (size + 1) / 2.0;
        std::cout << "Global sum computed: " << global_sum << std::endl;
        std::cout << "Expected sum: " << expected_sum << std::endl;
        std::cout << "Error: " << std::abs(global_sum - expected_sum) << std::endl;
    }
    
    // Esperar a que todos los procesos terminen antes de imprimir
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Cada proceso imprime su contribución y el resultado
    std::cout << "Process " << rank << ": local value = " << local_value 
              << ", global sum = " << global_sum << std::endl;
    
    MPI_Finalize();
    return 0;
}