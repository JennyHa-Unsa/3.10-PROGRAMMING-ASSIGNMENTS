#include <iostream>
#include <algorithm>
#include <cmath>
#include <mpi.h>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

int Find_bin(double value, double* bin_maxes, int bin_count, double min_meas) {
    if (value < min_meas) {
        return 0;
    }
    for (int b = 0; b < bin_count; b++) {
        if (value <= bin_maxes[b]) {
            return b;
        }
    }
    return bin_count - 1;
}

// Función para leer datos desde archivo
vector<double> read_data_from_file(const string& filename, int& data_count) {
    vector<double> data;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << filename << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        double value;
        while (ss >> value) {
            data.push_back(value);
        }
    }
    file.close();
    
    data_count = data.size();
    return data;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int my_rank, process_count;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    
    // Verificar argumentos de línea de comandos
    if (argc != 2 && my_rank == 0) {
        cerr << "Uso: " << argv[0] << " <archivo_de_datos>" << endl;
        cerr << "El archivo debe contener números separados por espacios o saltos de línea" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    string filename = argv[1];
    int data_count = 0;
    vector<double> data;
    
    // Solo el proceso 0 lee el archivo
    if (my_rank == 0) {
        data = read_data_from_file(filename, data_count);
        
        if (data_count == 0) {
            cerr << "Error: El archivo está vacío o no contiene datos válidos" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        printf("ARCHIVO: %s\n", filename.c_str());
        printf("- Elementos leídos: %d\n", data_count);
    }
    
    // Broadcast del número de datos a todos los procesos
    MPI_Bcast(&data_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Preparar array para los datos (todos los procesos necesitan el array completo)
    double* data_array = new double[data_count];
    
    // Proceso 0 copia los datos al array
    if (my_rank == 0) {
        copy(data.begin(), data.end(), data_array);
    }
    
    // Broadcast de todos los datos a todos los procesos
    MPI_Bcast(data_array, data_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // Proceso 0 calcula min_meas y max_meas
    double min_meas, max_meas;
    if (my_rank == 0) {
        // Crear copia temporal para ordenar
        double* temp_data = new double[data_count];
        copy(data_array, data_array + data_count, temp_data);
        sort(temp_data, temp_data + data_count);
        min_meas = temp_data[0];
        max_meas = temp_data[data_count - 1];
        delete[] temp_data;

        printf("\nINPUT DATA:\n");
        printf("- Count: %d, Min: %f, Max: %f\n", data_count, min_meas, max_meas);
    }
    
    // Broadcast de min_meas y max_meas a todos los procesos
    MPI_Bcast(&min_meas, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_meas, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // Calcular número de bins y ancho (igual para todos)
    int bin_count = 1 + log2(data_count);
    double bin_width = (max_meas - min_meas) / bin_count;
    
    if (my_rank == 0) {
        printf("- Bin count: %d, Bin width: %f\n", bin_count, bin_width);
        printf("- Process count: %d\n", process_count);
        printf("\nOUTPUT DATA:\n");
    }
    
    // Calcular límites superiores de bins (igual para todos)
    double* bin_maxes = new double[bin_count];
    for (int b = 0; b < bin_count; b++) {
        bin_maxes[b] = min_meas + bin_width * (b + 1);
    }
    
    // Distribuir datos entre procesos
    int local_data_count = data_count / process_count;
    int remainder = data_count % process_count;
    
    // Ajustar conteo local para procesos que reciben datos extra
    int my_local_count = local_data_count;
    if (my_rank < remainder) {
        my_local_count++;
    }
    
    // Calcular índice de inicio para cada proceso
    int start_index = 0;
    for (int p = 0; p < my_rank; p++) {
        start_index += (local_data_count + (p < remainder ? 1 : 0));
    }
    
    // Cada proceso trabaja con su porción local
    int* loc_bin_cts = new int[bin_count]();  // Inicializar a 0
    
    for (int i = start_index; i < start_index + my_local_count; i++) {
        int bin = Find_bin(data_array[i], bin_maxes, bin_count, min_meas);
        loc_bin_cts[bin]++;
    }
    
    // Reducir todos los loc_bin_cts a bin_counts en proceso 0
    int* bin_counts = nullptr;
    if (my_rank == 0) {
        bin_counts = new int[bin_count]();
    }
    
    // Usar MPI_Reduce para sumar todos los arrays locales
    MPI_Reduce(loc_bin_cts, bin_counts, bin_count, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    // Proceso 0 imprime resultados
    if (my_rank == 0) {
        cout << "Histograma (Paralelo - MPI):" << endl;
        for (int b = 0; b < bin_count; b++) {
            double lower_bound = (b == 0) ? min_meas : bin_maxes[b - 1];
            cout << "- Bin " << b << " [" << lower_bound << " - " 
                 << bin_maxes[b] << "): " << bin_counts[b] << endl;
        }
        
        // Verificación: suma total de elementos
        int total_elements = 0;
        for (int b = 0; b < bin_count; b++) {
            total_elements += bin_counts[b];
        }
        cout << "\nVerificación: Total de elementos procesados = " << total_elements 
             << " de " << data_count << " elementos originales" << endl;
    }
    
    // Limpiar memoria
    delete[] data_array;
    delete[] bin_maxes;
    delete[] loc_bin_cts;
    if (my_rank == 0) {
        delete[] bin_counts;
    }
    
    MPI_Finalize();
    return 0;
}

// Ejecución del código
// mpic++ -o ejercicio_1 ejercicio_1.cpp
// mpirun -np 4 ./ejercicio_1