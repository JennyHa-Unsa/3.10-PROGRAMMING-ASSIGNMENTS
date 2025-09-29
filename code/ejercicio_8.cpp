#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <mpi.h>

class DistributedVector {
private:
    int rank, size;
    int n; // tamaño total del vector
    std::vector<int> local_data;
    std::vector<int> global_indices; // índices globales de los elementos locales
    
public:
    DistributedVector(int total_size, int proc_rank, int proc_size) 
        : n(total_size), rank(proc_rank), size(proc_size) {}
    
    // Distribución por bloques
    void initialize_block_distribution() {
        int block_size = n / size;
        int remainder = n % size;
        
        int start = rank * block_size + std::min(rank, remainder);
        int end = start + block_size + (rank < remainder ? 1 : 0);
        
        local_data.clear();
        global_indices.clear();
        
        for (int i = start; i < end; i++) {
            local_data.push_back(i); // valor = índice global (para simplificar)
            global_indices.push_back(i);
        }
    }
    
    // Distribución cíclica
    void initialize_cyclic_distribution() {
        local_data.clear();
        global_indices.clear();
        
        for (int i = rank; i < n; i += size) {
            local_data.push_back(i);
            global_indices.push_back(i);
        }
    }
    
    // Obtener el dueño de un índice global en distribución cíclica
    int get_cyclic_owner(int global_index) {
        return global_index % size;
    }
    
    // Obtener el dueño de un índice global en distribución por bloques
    int get_block_owner(int global_index) {
        int block_size = n / size;
        int remainder = n % size;
        
        if (global_index < remainder * (block_size + 1)) {
            return global_index / (block_size + 1);
        } else {
            return remainder + (global_index - remainder * (block_size + 1)) / block_size;
        }
    }
    
    // Redistribuir de bloques a cíclica
    double redistribute_block_to_cyclic() {
        auto start_time = MPI_Wtime();
        
        // Paso 1: Cada proceso determina qué elementos necesita enviar y a quién
        std::map<int, std::vector<int>> send_data; // destino -> datos a enviar
        std::map<int, std::vector<int>> send_indices; // destino -> índices a enviar
        
        for (size_t i = 0; i < local_data.size(); i++) {
            int global_idx = global_indices[i];
            int cyclic_owner = get_cyclic_owner(global_idx);
            
            if (cyclic_owner != rank) {
                send_data[cyclic_owner].push_back(local_data[i]);
                send_indices[cyclic_owner].push_back(global_idx);
            }
        }
        
        // Paso 2: Intercambiar información sobre cuántos elementos se enviarán
        std::vector<int> send_counts(size, 0);
        std::vector<int> recv_counts(size, 0);
        
        for (const auto& pair : send_data) {
            send_counts[pair.first] = pair.second.size();
        }
        
        MPI_Alltoall(send_counts.data(), 1, MPI_INT, 
                    recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
        
        // Paso 3: Calcular desplazamientos para comunicación
        std::vector<int> send_displs(size, 0);
        std::vector<int> recv_displs(size, 0);
        
        for (int i = 1; i < size; i++) {
            send_displs[i] = send_displs[i-1] + send_counts[i-1];
            recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
        }
        
        int total_send = send_displs[size-1] + send_counts[size-1];
        int total_recv = recv_displs[size-1] + recv_counts[size-1];
        
        // Paso 4: Preparar datos para envío
        std::vector<int> send_buffer_data(total_send);
        std::vector<int> send_buffer_indices(total_send);
        
        for (int dest = 0; dest < size; dest++) {
            if (!send_data[dest].empty()) {
                std::copy(send_data[dest].begin(), send_data[dest].end(),
                         send_buffer_data.begin() + send_displs[dest]);
                std::copy(send_indices[dest].begin(), send_indices[dest].end(),
                         send_buffer_indices.begin() + send_displs[dest]);
            }
        }
        
        // Paso 5: Comunicar datos
        std::vector<int> recv_buffer_data(total_recv);
        std::vector<int> recv_buffer_indices(total_recv);
        
        MPI_Alltoallv(send_buffer_data.data(), send_counts.data(), send_displs.data(), MPI_INT,
                     recv_buffer_data.data(), recv_counts.data(), recv_displs.data(), MPI_INT,
                     MPI_COMM_WORLD);
        
        MPI_Alltoallv(send_buffer_indices.data(), send_counts.data(), send_displs.data(), MPI_INT,
                     recv_buffer_indices.data(), recv_counts.data(), recv_displs.data(), MPI_INT,
                     MPI_COMM_WORLD);
        
        // Paso 6: Reconstruir datos locales (mantener elementos que no se enviaron + recibidos)
        std::vector<int> new_local_data;
        std::vector<int> new_global_indices;
        
        // Agregar elementos que se quedaron locales
        for (size_t i = 0; i < local_data.size(); i++) {
            int global_idx = global_indices[i];
            if (get_cyclic_owner(global_idx) == rank) {
                new_local_data.push_back(local_data[i]);
                new_global_indices.push_back(global_idx);
            }
        }
        
        // Agregar elementos recibidos
        for (int i = 0; i < total_recv; i++) {
            new_local_data.push_back(recv_buffer_data[i]);
            new_global_indices.push_back(recv_buffer_indices[i]);
        }
        
        // Paso 7: Ordenar por índice global (para consistencia)
        // Usamos un simple bubble sort para pequeños volúmenes
        for (size_t i = 0; i < new_global_indices.size(); i++) {
            for (size_t j = i + 1; j < new_global_indices.size(); j++) {
                if (new_global_indices[i] > new_global_indices[j]) {
                    std::swap(new_global_indices[i], new_global_indices[j]);
                    std::swap(new_local_data[i], new_local_data[j]);
                }
            }
        }
        
        local_data = new_local_data;
        global_indices = new_global_indices;
        
        auto end_time = MPI_Wtime();
        return end_time - start_time;
    }
    
    // Redistribuir de cíclica a bloques (similar al anterior pero con lógica inversa)
    double redistribute_cyclic_to_block() {
        auto start_time = MPI_Wtime();
        
        std::map<int, std::vector<int>> send_data;
        std::map<int, std::vector<int>> send_indices;
        
        for (size_t i = 0; i < local_data.size(); i++) {
            int global_idx = global_indices[i];
            int block_owner = get_block_owner(global_idx);
            
            if (block_owner != rank) {
                send_data[block_owner].push_back(local_data[i]);
                send_indices[block_owner].push_back(global_idx);
            }
        }
        
        // El resto del código es idéntico al método anterior...
        std::vector<int> send_counts(size, 0);
        std::vector<int> recv_counts(size, 0);
        
        for (const auto& pair : send_data) {
            send_counts[pair.first] = pair.second.size();
        }
        
        MPI_Alltoall(send_counts.data(), 1, MPI_INT, 
                    recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
        
        std::vector<int> send_displs(size, 0);
        std::vector<int> recv_displs(size, 0);
        
        for (int i = 1; i < size; i++) {
            send_displs[i] = send_displs[i-1] + send_counts[i-1];
            recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
        }
        
        int total_send = send_displs[size-1] + send_counts[size-1];
        int total_recv = recv_displs[size-1] + recv_counts[size-1];
        
        std::vector<int> send_buffer_data(total_send);
        std::vector<int> send_buffer_indices(total_send);
        
        for (int dest = 0; dest < size; dest++) {
            if (!send_data[dest].empty()) {
                std::copy(send_data[dest].begin(), send_data[dest].end(),
                         send_buffer_data.begin() + send_displs[dest]);
                std::copy(send_indices[dest].begin(), send_indices[dest].end(),
                         send_buffer_indices.begin() + send_displs[dest]);
            }
        }
        
        std::vector<int> recv_buffer_data(total_recv);
        std::vector<int> recv_buffer_indices(total_recv);
        
        MPI_Alltoallv(send_buffer_data.data(), send_counts.data(), send_displs.data(), MPI_INT,
                     recv_buffer_data.data(), recv_counts.data(), recv_displs.data(), MPI_INT,
                     MPI_COMM_WORLD);
        
        MPI_Alltoallv(send_buffer_indices.data(), send_counts.data(), send_displs.data(), MPI_INT,
                     recv_buffer_indices.data(), recv_counts.data(), recv_displs.data(), MPI_INT,
                     MPI_COMM_WORLD);
        
        std::vector<int> new_local_data;
        std::vector<int> new_global_indices;
        
        for (size_t i = 0; i < local_data.size(); i++) {
            int global_idx = global_indices[i];
            if (get_block_owner(global_idx) == rank) {
                new_local_data.push_back(local_data[i]);
                new_global_indices.push_back(global_idx);
            }
        }
        
        for (int i = 0; i < total_recv; i++) {
            new_local_data.push_back(recv_buffer_data[i]);
            new_global_indices.push_back(recv_buffer_indices[i]);
        }
        
        // Ordenar
        for (size_t i = 0; i < new_global_indices.size(); i++) {
            for (size_t j = i + 1; j < new_global_indices.size(); j++) {
                if (new_global_indices[i] > new_global_indices[j]) {
                    std::swap(new_global_indices[i], new_global_indices[j]);
                    std::swap(new_local_data[i], new_local_data[j]);
                }
            }
        }
        
        local_data = new_local_data;
        global_indices = new_global_indices;
        
        auto end_time = MPI_Wtime();
        return end_time - start_time;
    }
    
    // Verificar distribución (para debugging)
    void print_local_data() {
        std::cout << "Process " << rank << " has " << local_data.size() 
                  << " elements: [";
        for (size_t i = 0; i < local_data.size(); i++) {
            std::cout << local_data[i];
            if (i < local_data.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    int get_local_size() const { return local_data.size(); }
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int n = 1000; // Tamaño del vector
    if (argc > 1) n = std::atoi(argv[1]);
    
    DistributedVector vec(n, rank, size);
    double time1, time2;
    
    if (rank == 0) {
        std::cout << "=== Redistribución de Vector Distribuido ===" << std::endl;
        std::cout << "Tamaño del vector: " << n << std::endl;
        std::cout << "Número de procesos: " << size << std::endl << std::endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Test: Bloques -> Cíclica
    if (rank == 0) std::cout << "1. Redistribución Bloques -> Cíclica:" << std::endl;
    
    vec.initialize_block_distribution();
    MPI_Barrier(MPI_COMM_WORLD);
    
    time1 = vec.redistribute_block_to_cyclic();
    
    double max_time1;
    MPI_Reduce(&time1, &max_time1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        std::cout << "Tiempo máximo: " << max_time1 * 1000 << " ms" << std::endl << std::endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Test: Cíclica -> Bloques
    if (rank == 0) std::cout << "2. Redistribución Cíclica -> Bloques:" << std::endl;
    
    vec.initialize_cyclic_distribution();
    MPI_Barrier(MPI_COMM_WORLD);
    
    time2 = vec.redistribute_cyclic_to_block();
    
    double max_time2;
    MPI_Reduce(&time2, &max_time2, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        std::cout << "Tiempo máximo: " << max_time2 * 1000 << " ms" << std::endl << std::endl;
        
        std::cout << "=== Resumen ===" << std::endl;
        std::cout << "Bloques -> Cíclica: " << max_time1 * 1000 << " ms" << std::endl;
        std::cout << "Cíclica -> Bloques: " << max_time2 * 1000 << " ms" << std::endl;
        std::cout << "Diferencia: " << std::abs(max_time1 - max_time2) * 1000 << " ms" << std::endl;
    }
    
    MPI_Finalize();
    return 0;
}