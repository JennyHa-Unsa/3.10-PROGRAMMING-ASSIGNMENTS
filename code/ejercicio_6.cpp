#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

using namespace std;

// Función para imprimir una matriz
void print_matrix(const vector<vector<double>>& matrix, const string& name) {
    cout << name << ":\n";
    for (const auto& row : matrix) {
        for (double val : row) {
            cout << val << "\t";
        }
        cout << endl;
    }
    cout << endl;
}

// Función para imprimir un vector
void print_vector(const vector<double>& vec, const string& name) {
    cout << name << ": [";
    for (double val : vec) {
        cout << val << " ";
    }
    cout << "]\n";
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    // Verificar que comm_sz sea un cuadrado perfecto
    int q = sqrt(comm_sz);
    if (q * q != comm_sz) {
        if (my_rank == 0) {
            cerr << "Error: El número de procesos debe ser un cuadrado perfecto" << endl;
        }
        MPI_Finalize();
        return 1;
    }
    
    int n = 12; // Orden de la matriz (debe ser divisible por q)
    if (n % q != 0) {
        if (my_rank == 0) {
            cerr << "Error: n debe ser divisible por sqrt(comm_sz)" << endl;
        }
        MPI_Finalize();
        return 1;
    }
    
    int block_size = n / q;
    vector<vector<double>> local_A(block_size, vector<double>(block_size, 0.0));
    vector<double> local_v(block_size, 0.0);
    vector<double> local_result(block_size, 0.0);
    vector<double> final_result(n, 0.0);
    
    // Crear un comunicador cartesiano (grid 2D)
    int dims[2] = {q, q};
    int periods[2] = {0, 0};
    int reorder = 0;
    MPI_Comm grid_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &grid_comm);
    
    int my_coords[2];
    MPI_Cart_coords(grid_comm, my_rank, 2, my_coords);
    int my_row = my_coords[0];
    int my_col = my_coords[1];
    
    // Proceso 0 lee y distribuye los datos
    if (my_rank == 0) {
        // Crear matriz A y vector v de ejemplo
        vector<vector<double>> A(n, vector<double>(n, 0.0));
        vector<double> v(n, 0.0);
        
        // Inicializar A y v con valores de ejemplo
        for (int i = 0; i < n; i++) {
            v[i] = i + 1; // v = [1, 2, 3, ..., n]
            for (int j = 0; j < n; j++) {
                A[i][j] = i * n + j + 1; // Valores consecutivos
            }
        }
        
        //print_matrix(A, "Matriz A completa");
        //print_vector(v, "Vector v completo");
        
        // Distribuir submatrices a todos los procesos
        for (int dest_rank = 0; dest_rank < comm_sz; dest_rank++) {
            int dest_coords[2];
            MPI_Cart_coords(grid_comm, dest_rank, 2, dest_coords);
            int dest_row = dest_coords[0];
            int dest_col = dest_coords[1];
            
            // Extraer el bloque correspondiente
            vector<double> block_data(block_size * block_size);
            for (int i = 0; i < block_size; i++) {
                for (int j = 0; j < block_size; j++) {
                    int global_i = dest_row * block_size + i;
                    int global_j = dest_col * block_size + j;
                    block_data[i * block_size + j] = A[global_i][global_j];
                }
            }
            
            MPI_Send(block_data.data(), block_size * block_size, MPI_DOUBLE, 
                    dest_rank, 0, grid_comm);
        }
        
        // Distribuir segmentos del vector a procesos diagonales
        for (int diag = 0; diag < q; diag++) {
            int diag_rank;
            int diag_coords[2] = {diag, diag};
            MPI_Cart_rank(grid_comm, diag_coords, &diag_rank);
            
            vector<double> v_segment(block_size);
            for (int i = 0; i < block_size; i++) {
                v_segment[i] = v[diag * block_size + i];
            }
            
            MPI_Send(v_segment.data(), block_size, MPI_DOUBLE, diag_rank, 1, grid_comm);
        }
    }
    
    // Todos los procesos reciben su submatriz
    vector<double> block_data(block_size * block_size);
    MPI_Recv(block_data.data(), block_size * block_size, MPI_DOUBLE, 
            0, 0, grid_comm, MPI_STATUS_IGNORE);
    
    // Reconstruir la submatriz local 2D
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            local_A[i][j] = block_data[i * block_size + j];
        }
    }
    
    // Procesos diagonales reciben su segmento del vector
    if (my_row == my_col) {
        MPI_Recv(local_v.data(), block_size, MPI_DOUBLE, 0, 1, grid_comm, MPI_STATUS_IGNORE);
    }
    
    // Crear comunicadores por columna para la difusión del vector
    MPI_Comm col_comm;
    int remain_dims[2] = {0, 1}; // Mantener la dimensión columna
    MPI_Cart_sub(grid_comm, remain_dims, &col_comm);
    
    // Difundir el vector dentro de cada columna
    int col_root = my_col; // El proceso diagonal es el root en su columna
    MPI_Bcast(local_v.data(), block_size, MPI_DOUBLE, col_root, col_comm);
    
    // Realizar la multiplicación local: local_result = local_A * local_v
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            local_result[i] += local_A[i][j] * local_v[j];
        }
    }
    
    // Crear comunicadores por fila para la reducción
    MPI_Comm row_comm;
    int remain_dims_row[2] = {1, 0}; // Mantener la dimensión fila
    MPI_Cart_sub(grid_comm, remain_dims_row, &row_comm);
    
    // Reducir los resultados por fila (suma)
    vector<double> row_result(block_size, 0.0);
    int row_root = my_row; // El proceso diagonal recibe el resultado de la fila
    
    MPI_Reduce(local_result.data(), row_result.data(), block_size, 
               MPI_DOUBLE, MPI_SUM, row_root, row_comm);
    
    // Procesos diagonales envían sus resultados al proceso 0
    if (my_row == my_col) {
        MPI_Send(row_result.data(), block_size, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
    }
    
    // Proceso 0 recoge y ensambla el resultado final
    if (my_rank == 0) {
        for (int diag = 0; diag < q; diag++) {
            vector<double> segment(block_size);
            int diag_rank;
            int diag_coords[2] = {diag, diag};
            MPI_Cart_rank(grid_comm, diag_coords, &diag_rank);
            
            MPI_Recv(segment.data(), block_size, MPI_DOUBLE, diag_rank, 
                    2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Colocar el segmento en la posición correcta del resultado final
            for (int i = 0; i < block_size; i++) {
                final_result[diag * block_size + i] = segment[i];
            }
        }
        
        // Imprimir el resultado
        print_vector(final_result, "Resultado final A*v");
        
        // Verificación (opcional): calcular secuencialmente y comparar
        vector<double> sequential_result(n, 0.0);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                sequential_result[i] += (i * n + j + 1) * (j + 1);
            }
        }
        print_vector(sequential_result, "Resultado secuencial (verificación)");
    }
    
    // Liberar comunicadores
    MPI_Comm_free(&grid_comm);
    MPI_Comm_free(&col_comm);
    MPI_Comm_free(&row_comm);
    
    MPI_Finalize();
    return 0;
}