#include <mpi.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Función para calcular la siguiente potencia de 2 mayor o igual a n
int next_power_of_two(int n) {
    int power = 1;
    while (power < n) {
        power *= 2;
    }
    return power;
}

// Función para generar un valor local aleatorio para cada proceso
double generate_local_value(int rank) {
    std::srand(std::time(nullptr) + rank); // Semilla diferente para cada proceso
    return (std::rand() % 100) + 1; // Valor entre 1 y 100
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    // Generar valor local para cada proceso
    double local_val = generate_local_value(my_rank);
    double original_local = local_val; // Guardar copia para verificación
    
    std::cout << "Proceso " << my_rank << " valor inicial: " << local_val << std::endl;
    
    // Sincronizar antes de comenzar
    MPI_Barrier(MPI_COMM_WORLD);
    
    // **ALGORITMO DE SUMA EN ÁRBOL**
    
    // Caso especial: solo un proceso
    if (comm_sz == 1) {
        std::cout << "Suma total: " << local_val << std::endl;
        MPI_Finalize();
        return 0;
    }
    
    // Calcular la potencia de 2 más cercana (hacia arriba)
    int power_of_two = next_power_of_two(comm_sz);
    
    // **FASE 1: Procesos que participan en el árbol binario completo**
    int processes_in_tree = (comm_sz < power_of_two) ? comm_sz : power_of_two;
    
    if (my_rank < processes_in_tree) {
        // Participar en el árbol binario
        for (int gap = 1; gap < processes_in_tree; gap *= 2) {
            if (my_rank % (2 * gap) == 0) {
                // Este proceso es RECEPTOR en este nivel
                if (my_rank + gap < processes_in_tree) {
                    double received_val;
                    MPI_Recv(&received_val, 1, MPI_DOUBLE, my_rank + gap, 
                            0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    local_val += received_val;
                    
                    if (my_rank == 0) {
                        std::cout << "Proceso 0 recibió " << received_val 
                                  << " de proceso " << my_rank + gap 
                                  << ", suma parcial: " << local_val << std::endl;
                    }
                }
            } else if (my_rank % gap == 0) {
                // Este proceso es EMISOR en este nivel
                int target = my_rank - gap;
                MPI_Send(&local_val, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD);
                
                std::cout << "Proceso " << my_rank << " envió " << local_val 
                          << " a proceso " << target << std::endl;
                break; // Este proceso termina su participación
            }
        }
    }
    
    // **FASE 2: Manejar procesos extras (si comm_sz no es potencia de 2)**
    if (my_rank >= processes_in_tree && my_rank < comm_sz) {
        // Proceso EXTRA: enviar directamente al proceso 0
        MPI_Send(&local_val, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        std::cout << "Proceso extra " << my_rank << " envió " << local_val 
                  << " directamente al proceso 0" << std::endl;
    }
    
    // **FASE 3: Proceso 0 recibe de procesos extras**
    if (my_rank == 0 && processes_in_tree < comm_sz) {
        for (int extra = processes_in_tree; extra < comm_sz; extra++) {
            double extra_val;
            MPI_Recv(&extra_val, 1, MPI_DOUBLE, extra, 1, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local_val += extra_val;
            
            std::cout << "Proceso 0 recibió " << extra_val 
                      << " de proceso extra " << extra 
                      << ", suma total: " << local_val << std::endl;
        }
    }
    
    // Sincronizar antes de mostrar resultados
    MPI_Barrier(MPI_COMM_WORLD);
    
    // **VERIFICACIÓN: Comparar con MPI_Allreduce (opcional)**
    if (my_rank == 0) {
        // Verificación usando MPI_Allreduce para comparar
        double verification_sum;
        double total_manual = local_val; // Nuestro resultado
        
        // Calcular suma usando MPI (para verificación)
        MPI_Reduce(&original_local, &verification_sum, 1, MPI_DOUBLE, 
                  MPI_SUM, 0, MPI_COMM_WORLD);
        
        std::cout << "\n=== RESULTADOS FINALES ===" << std::endl;
        std::cout << "Suma con algoritmo de árbol: " << total_manual << std::endl;
        std::cout << "Suma con MPI_Reduce (verificación): " << verification_sum << std::endl;
        std::cout << "Resultado " << 
            (std::abs(total_manual - verification_sum) < 1e-10 ? "CORRECTO" : "INCORRECTO") 
                  << std::endl;
    } else {
        // Otros procesos participan en la verificación
        MPI_Reduce(&original_local, nullptr, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
    return 0;
}