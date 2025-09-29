#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <iterator>

// Función para mezclar dos vectores ordenados
std::vector<int> merge(const std::vector<int>& v1, const std::vector<int>& v2) {
    std::vector<int> result;
    result.reserve(v1.size() + v2.size());
    
    size_t i = 0, j = 0;
    
    while (i < v1.size() && j < v2.size()) {
        if (v1[i] <= v2[j]) {
            result.push_back(v1[i++]);
        } else {
            result.push_back(v2[j++]);
        }
    }
    
    // Agregar elementos restantes
    while (i < v1.size()) result.push_back(v1[i++]);
    while (j < v2.size()) result.push_back(v2[j++]);
    
    return result;
}

// Función para imprimir un vector
void print_vector(const std::vector<int>& vec, const std::string& label) {
    std::cout << label << ": ";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i < vec.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    int n = 0;
    std::vector<int> local_list;
    
    // Proceso 0 lee el valor de n
    if (my_rank == 0) {
        if (argc > 1) {
            n = std::atoi(argv[1]);
        } else {
            n = 32; // Valor por defecto
        }
        std::cout << "Parallel Merge Sort - n = " << n << ", procesos = " << comm_sz << std::endl;
    }
    
    // Broadcast de n a todos los procesos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Cada proceso genera su lista local
    int local_n = n / comm_sz;
    local_list.resize(local_n);
    
    // Generador de números aleatorios (semilla diferente por proceso)
    std::random_device rd;
    std::mt19937 gen(rd() + my_rank);
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (int i = 0; i < local_n; ++i) {
        local_list[i] = dis(gen);
    }
    
    // Cada proceso ordena su lista local
    std::sort(local_list.begin(), local_list.end());
    
    // Proceso 0 recoge e imprime las listas locales
    if (my_rank == 0) {
        std::vector<int> all_lists(n);
        std::vector<int> recv_counts(comm_sz, local_n);
        std::vector<int> displs(comm_sz);
        
        for (int i = 0; i < comm_sz; ++i) {
            displs[i] = i * local_n;
        }
        
        MPI_Gatherv(local_list.data(), local_n, MPI_INT,
                   all_lists.data(), recv_counts.data(), displs.data(), MPI_INT,
                   0, MPI_COMM_WORLD);
        
        std::cout << "\n=== Listas locales ordenadas ===" << std::endl;
        for (int i = 0; i < comm_sz; ++i) {
            std::cout << "Proceso " << i << ": ";
            for (int j = 0; j < local_n; ++j) {
                std::cout << all_lists[i * local_n + j];
                if (j < local_n - 1) std::cout << " ";
            }
            std::cout << std::endl;
        }
    } else {
        MPI_Gatherv(local_list.data(), local_n, MPI_INT,
                   nullptr, nullptr, nullptr, MPI_INT,
                   0, MPI_COMM_WORLD);
    }
    
    // Sincronización antes de comenzar la mezcla
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Fase de mezcla usando comunicación en árbol
    int stages = std::log2(comm_sz);
    int current_partners = comm_sz;
    
    for (int stage = 0; stage < stages; ++stage) {
        int partner_distance = 1 << stage;
        int group_size = partner_distance * 2;
        
        // Determinar si este proceso participa en esta etapa
        if (my_rank % group_size == 0) {
            // Este proceso es el receptor en su grupo
            int sender_rank = my_rank + partner_distance;
            
            if (sender_rank < comm_sz) {
                // Recibir tamaño de la lista del emisor
                int sender_size;
                MPI_Recv(&sender_size, 1, MPI_INT, sender_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Recibir la lista del emisor
                std::vector<int> received_list(sender_size);
                MPI_Recv(received_list.data(), sender_size, MPI_INT, sender_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Mezclar las listas
                local_list = merge(local_list, received_list);
            }
        } else if (my_rank % group_size == partner_distance) {
            // Este proceso es el emisor en su grupo
            int receiver_rank = my_rank - partner_distance;
            
            // Enviar tamaño de la lista
            int local_size = local_list.size();
            MPI_Send(&local_size, 1, MPI_INT, receiver_rank, 0, MPI_COMM_WORLD);
            
            // Enviar la lista
            MPI_Send(local_list.data(), local_size, MPI_INT, receiver_rank, 0, MPI_COMM_WORLD);
        }
        
        // Sincronización entre etapas
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    // Proceso 0 imprime el resultado final
    if (my_rank == 0) {
        std::cout << "\n=== Lista final ordenada ===" << std::endl;
        print_vector(local_list, "Resultado");
        
        // Verificar que está ordenada
        bool is_sorted = std::is_sorted(local_list.begin(), local_list.end());
        std::cout << "Verificación: " << (is_sorted ? "CORRECTO" : "ERROR") << std::endl;
        std::cout << "Tamaño final: " << local_list.size() << std::endl;
    }
    
    MPI_Finalize();
    return 0;
}