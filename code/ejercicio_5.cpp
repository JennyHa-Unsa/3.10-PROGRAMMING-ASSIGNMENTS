#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>

void matrix_vector_mult_block_col(int n, const std::vector<double>& A, 
                                 const std::vector<double>& x, 
                                 std::vector<double>& y) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int cols_per_proc = n / size;
    int block_size = n * cols_per_proc;
    
    std::vector<double> local_A(block_size);
    std::vector<double> local_x(n);
    std::vector<double> local_y(n, 0.0);
    std::vector<double> y_final(n);
    
    // Proceso 0 lee y distribuye los datos
    if (rank == 0) {
        // Distribuir bloques de columnas de A
        for (int dest = 0; dest < size; dest++) {
            if (dest == 0) {
                // Copiar directamente para proceso 0
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < cols_per_proc; j++) {
                        local_A[i * cols_per_proc + j] = A[i * n + j];
                    }
                }
            } else {
                // Enviar bloque a otros procesos
                std::vector<double> send_block(block_size);
                int start_col = dest * cols_per_proc;
                
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < cols_per_proc; j++) {
                        send_block[i * cols_per_proc + j] = 
                            A[i * n + (start_col + j)];
                    }
                }
                
                MPI_Send(send_block.data(), block_size, MPI_DOUBLE, 
                        dest, 0, MPI_COMM_WORLD);
            }
        }
        
        // Broadcast del vector x completo a todos los procesos
        local_x = x;
    } else {
        // Otros procesos reciben su bloque de A
        MPI_Recv(local_A.data(), block_size, MPI_DOUBLE, 
                0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Broadcast del vector x a todos los procesos
    MPI_Bcast(local_x.data(), n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // Multiplicación local: local_A * segmento correspondiente de x
    int start_col = rank * cols_per_proc;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < cols_per_proc; j++) {
            int col_index = start_col + j;
            local_y[i] += local_A[i * cols_per_proc + j] * local_x[col_index];
        }
    }
    
    // Usar MPI_Reduce_scatter para combinar resultados
    std::vector<int> recv_counts(size, n);
    
    MPI_Reduce_scatter(local_y.data(), y_final.data(), recv_counts.data(),
                      MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    
    // Recolectar el resultado completo en proceso 0 (opcional)
    if (rank == 0) {
        y = y_final;
        
        // Recibir segmentos de otros procesos
        for (int src = 1; src < size; src++) {
            int start_idx = src * cols_per_proc;
            MPI_Recv(&y[start_idx], cols_per_proc, MPI_DOUBLE,
                    src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        // Enviar segmento local a proceso 0
        int start_idx = rank * cols_per_proc;
        MPI_Send(&y_final[0], cols_per_proc, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }
}

// Versión alternativa usando MPI_Reduce + MPI_Scatter (más clara)
void matrix_vector_mult_block_col_alt(int n, const std::vector<double>& A, 
                                     const std::vector<double>& x, 
                                     std::vector<double>& y) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int cols_per_proc = n / size;
    int block_size = n * cols_per_proc;
    
    std::vector<double> local_A(block_size);
    std::vector<double> local_x(n);
    std::vector<double> local_y(n, 0.0);
    std::vector<double> total_y(n);
    
    // Distribución similar a la versión anterior
    if (rank == 0) {
        for (int dest = 0; dest < size; dest++) {
            int start_col = dest * cols_per_proc;
            
            if (dest == 0) {
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < cols_per_proc; j++) {
                        local_A[i * cols_per_proc + j] = A[i * n + (start_col + j)];
                    }
                }
            } else {
                std::vector<double> send_block(block_size);
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < cols_per_proc; j++) {
                        send_block[i * cols_per_proc + j] = 
                            A[i * n + (start_col + j)];
                    }
                }
                MPI_Send(send_block.data(), block_size, MPI_DOUBLE, 
                        dest, 0, MPI_COMM_WORLD);
            }
        }
        local_x = x;
    } else {
        MPI_Recv(local_A.data(), block_size, MPI_DOUBLE, 
                0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    MPI_Bcast(local_x.data(), n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // Multiplicación local
    int start_col = rank * cols_per_proc;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < cols_per_proc; j++) {
            local_y[i] += local_A[i * cols_per_proc + j] * local_x[start_col + j];
        }
    }
    
    // Reducir sumando todos los local_y a total_y en proceso 0
    MPI_Reduce(local_y.data(), total_y.data(), n, MPI_DOUBLE, 
               MPI_SUM, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        y = total_y;
    }
}

// Función para generar datos de prueba
void generate_test_data(int n, std::vector<double>& A, std::vector<double>& x) {
    A.resize(n * n);
    x.resize(n);
    
    for (int i = 0; i < n; i++) {
        x[i] = i + 1;  // x = [1, 2, 3, ..., n]
        for (int j = 0; j < n; j++) {
            A[i * n + j] = (i == j) ? 1.0 : 0.1;  // Diagonal dominante
        }
    }
}

// Función de multiplicación secuencial para verificación
void sequential_matrix_vector_mult(int n, const std::vector<double>& A, 
                                  const std::vector<double>& x, 
                                  std::vector<double>& y) {
    y.assign(n, 0.0);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            y[i] += A[i * n + j] * x[j];
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int n = 8;  // Tamaño de la matriz (debe ser divisible por comm_sz)
    
    std::vector<double> A, x, y_parallel, y_sequential;
    
    if (rank == 0) {
        generate_test_data(n, A, x);
        std::cout << "Matriz de tamaño " << n << "x" << n << std::endl;
        std::cout << "Vector de tamaño " << n << std::endl;
    }
    
    // Ejecutar multiplicación paralela
    matrix_vector_mult_block_col_alt(n, A, x, y_parallel);
    
    // Verificación en proceso 0
    if (rank == 0) {
        sequential_matrix_vector_mult(n, A, x, y_sequential);
        
        std::cout << "\nResultado paralelo: ";
        for (int i = 0; i < n; i++) {
            std::cout << y_parallel[i] << " ";
        }
        std::cout << "\nResultado secuencial: ";
        for (int i = 0; i < n; i++) {
            std::cout << y_sequential[i] << " ";
        }
        std::cout << std::endl;
        
        // Verificar precisión
        double error = 0.0;
        for (int i = 0; i < n; i++) {
            error += std::abs(y_parallel[i] - y_sequential[i]);
        }
        std::cout << "Error total: " << error << std::endl;
    }
    
    MPI_Finalize();
    return 0;
}