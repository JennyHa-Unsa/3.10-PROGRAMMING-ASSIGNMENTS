/* Use MPI to implement the histogram program discussed in Section 2.7.1. Have
process 0 read in the input data and distribute it among the processes. Also have
process 0 print out the histogram. */

#include <iostream>
#include <algorithm>
#include <cmath>
//#include <mpi.h>

using namespace std;

int Find_bin(double value, double* bin_maxes, int bin_count, double min_meas){
    // Caso especial: cae en el bin 0
    if(value < min_meas) {
        return 0;
    }
    // Buscar el bin correcto
    for (int b = 0; b < bin_count; b++) {
        if (value <= bin_maxes[b]) {
            return b;
        }
    }
    // Si no encontró (dato == max_meas), devolver el último bin
    return bin_count - 1;
}

int main() {
    
    double data[] = {1.3, 2.9, 0.4, 0.3, 1.3, 
    4.4, 1.7, 0.4, 3.2, 0.3, 
    4.9, 2.4, 3.1, 4.4, 3.9, 
    0.4, 4.2, 4.5, 4.9, 0.9};
    
    // Tamaño de los datos
    int data_count = sizeof(data) / sizeof(data[0]);

    // Valor minimo y máximo de los datos
    sort(data, data + data_count);
    double min_meas = data[0];
    double max_meas = data[data_count - 1];
    printf("INPUT DATA:\n");
    printf("- Count: %d, Min: %f, Max: %f\n", data_count, min_meas, max_meas);

    // Número de bins(intervalos) y ancho de cada bin
    int bin_count = 1 + log2(data_count);   // Regla de Sturges
    double bin_width = (max_meas - min_meas) / bin_count;   

    printf("- Bin count: %d, Bin width: %f\n", bin_count, bin_width);
    printf("\nOUTPUT DATA:\n");

    // Calcular los límites superiores de los bins
    double bin_maxes[bin_count];    
    for (int b = 0; b < bin_count; b++) {
        bin_maxes[b] = min_meas + bin_width*(b+1);
    }

    // Contar los datos en bins
    int bin_counts[bin_count] = {0};  
    for (int i = 0; i < data_count; i++) {
        int bin = Find_bin(data[i], bin_maxes, bin_count, min_meas);
        bin_counts[bin]++;
    }

    // Mostrar resultados
    cout << "Histograma:" << endl;
    for (int b = 0; b < bin_count; b++) {
        cout << "- Bin " << b << " [" 
             << (b == 0 ? min_meas : bin_maxes[b-1]) 
             << " - " << bin_maxes[b] << "): "
             << bin_counts[b] << endl;
    }


    return 0;
}

// Ejecución del código
// mpic++ -o ejercicio_1 ejercicio_1.cpp
// mpirun -np 4 ./ejercicio_1