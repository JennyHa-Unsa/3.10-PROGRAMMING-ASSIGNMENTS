#include <mpi.h>
#include <iostream>
#include <ctime>

double random_double() {
    return (double)rand() / RAND_MAX * 2.0 - 1.0;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    
    long long total_tosses, local_tosses;
    long long local_count = 0, global_count = 0;
    
    // Proceso 0 lee y distribuye
    if (rank == 0) {
        printf("Ingrese número total de tiros: ");
        scanf("%lld", &total_tosses);
    }
    
    // Broadcast del total a todos
    MPI_Bcast(&total_tosses, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    // Dividir trabajo
    local_tosses = total_tosses / num_processes;
    if (rank == 0) {
        local_tosses += total_tosses % num_processes; // Proceso 0 toma el resto
    }
    
    // Semilla única por proceso
    srand(time(NULL) + rank);
    
    // Conteo local
    for (long long i = 0; i < local_tosses; i++) {
        double x = random_double();
        double y = random_double();
        if (x*x + y*y <= 1.0) {
            local_count++;
        }
    }
    
    // Reducir todos los conteos al proceso 0
    MPI_Reduce(&local_count, &global_count, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    
    // Proceso 0 calcula e imprime π
    if (rank == 0) {
        double pi_estimate = 4.0 * global_count / total_tosses;
        printf("π estimado: %.10f\n", pi_estimate);
        printf("Tiros en círculo: %lld de %lld\n", global_count, total_tosses);
    }
    
    MPI_Finalize();
    return 0;
}