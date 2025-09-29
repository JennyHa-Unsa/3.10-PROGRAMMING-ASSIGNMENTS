#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

int Find_bin(double value, double* bin_maxes, int bin_count, double min_meas){
    if(value < min_meas) return 0;
    for (int b = 0; b < bin_count; b++) {
        if (value <= bin_maxes[b]) return b;
    }
    return bin_count - 1;
}

int main(int argc, char* argv[]) {

    // Abrir archivo de datos
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <archivo.txt>" << endl;
        return 1;
    }

    string filename = argv[1];
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        return 1;
    }

    vector<double> data;
    double value;
    while (infile >> value) {
        data.push_back(value);
    }
    infile.close();

    // Calcular tamaño de los datos
    int data_count = data.size();
    if (data_count == 0) {
        cerr << "El archivo está vacío o no contiene datos válidos." << endl;
        return 1;
    }
    
    // Calcular min_meas y max_meas
    sort(data.begin(), data.end());
    double min_meas = data.front();
    double max_meas = data.back();

    printf("INPUT DATA:\n");
    printf("- Count: %d, Min: %f, Max: %f\n", data_count, min_meas, max_meas);

    int bin_count = 1 + log2(data_count);
    double bin_width = (max_meas - min_meas) / bin_count;

    printf("- Bin count: %d, Bin width: %f\n", bin_count, bin_width);
    printf("\nOUTPUT DATA:\n");

    double bin_maxes[bin_count];
    for (int b = 0; b < bin_count; b++) {
        bin_maxes[b] = min_meas + bin_width * (b + 1);
    }

    int bin_counts[bin_count] = {0};
    for (int i = 0; i < data_count; i++) {
        int bin = Find_bin(data[i], bin_maxes, bin_count, min_meas);
        bin_counts[bin]++;
    }

    cout << "Histograma:" << endl;
    for (int b = 0; b < bin_count; b++) {
        cout << "- Bin " << b << " ["
             << (b == 0 ? min_meas : bin_maxes[b - 1])
             << " - " << bin_maxes[b] << "): "
             << bin_counts[b] << endl;
    }

    return 0;
}
