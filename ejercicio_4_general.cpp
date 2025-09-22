#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Función para calcular el techo de log2
int ceil_log2(int n) {
    if (n <= 1) return 0;
    return std::ceil(std::log2(n));
}

// Algoritmo butterfly general para cualquier número de procesos
double butterfly_sum_general(double local_value, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    double sum = local_value;
    int total_stages = ceil_log2(size);
    
    for (int stage = 0; stage < total_stages; ++stage) {
        int distance = 1 << stage;  // 2^stage
        int partner = rank ^ distance;  // XOR para calcular el compañero
        
        // Solo comunicar si el compañero existe
        if (partner < size) {
            double received_value;
            MPI_Sendrecv(&sum, 1, MPI_DOUBLE, partner, stage,
                        &received_value, 1, MPI_DOUBLE, partner, stage,
                        comm, MPI_STATUS_IGNORE);
            
            sum += received_value;
        }
        // Si el compañero no existe, el proceso conserva su suma actual
        // y espera a la siguiente etapa donde pueda tener un compañero válido
    }
    
    return sum;
}

// Versión alternativa usando comunicación más robusta
double butterfly_sum_robust(double local_value, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    double sum = local_value;
    int total_stages = ceil_log2(size);
    
    for (int stage = 0; stage < total_stages; ++stage) {
        int distance = 1 << stage;
        int partner = rank ^ distance;

        double update;
        
        if (partner < size) {
            // Comunicación punto a punto
            if (rank < partner) {
                // Proceso con rank menor envía y luego recibe
                MPI_Send(&sum, 1, MPI_DOUBLE, partner, stage, comm);
                double temp;
                MPI_Recv(&temp, 1, MPI_DOUBLE, partner, stage, comm, MPI_STATUS_IGNORE);
                sum += temp;
            } else {
                // Proceso con rank mayor recibe y luego envía
                double temp;
                MPI_Recv(&temp, 1, MPI_DOUBLE, partner, stage, comm, MPI_STATUS_IGNORE);
                sum += temp;
                update = sum - temp; // Guardar el valor actualizado
                MPI_Send(&update, 1, MPI_DOUBLE, partner, stage, comm); // Envía el valor original
            }
        }
    }
    
    return sum;
}

// Función para verificar el resultado usando MPI_Reduce
double verify_global_sum(double local_value, MPI_Comm comm) {
    double global_sum;
    MPI_Allreduce(&local_value, &global_sum, 1, MPI_DOUBLE, MPI_SUM, comm);
    return global_sum;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Cada proceso tiene un valor local
    double local_value = static_cast<double>(rank + 1);
    
    // Calcular suma global usando butterfly general
    double butterfly_sum = butterfly_sum_general(local_value, MPI_COMM_WORLD);
    
    // Verificar usando MPI_Allreduce
    double verified_sum = verify_global_sum(local_value, MPI_COMM_WORLD);
    
    // Calcular error
    double error = std::abs(butterfly_sum - verified_sum);
    
    // Imprimir resultados
    if (rank == 0) {
        std::cout << "=== Butterfly Sum Results ===" << std::endl;
        std::cout << "Number of processes: " << size << std::endl;
        std::cout << "Butterfly sum: " << butterfly_sum << std::endl;
        std::cout << "Verified sum (MPI_Allreduce): " << verified_sum << std::endl;
        std::cout << "Error: " << error << std::endl;
        std::cout << "=============================" << std::endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Cada proceso imprime su información
    std::cout << "Process " << rank << "/" << size - 1 
              << ": local = " << local_value 
              << ", butterfly_sum = " << butterfly_sum 
              << ", error = " << error << std::endl;
    
    MPI_Finalize();
    return 0;
}