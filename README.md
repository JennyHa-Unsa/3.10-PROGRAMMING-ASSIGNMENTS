# 3.10-PROGRAMMING-ASSIGNMENTS
Distributed-Memory Programming with MPI

## 1. Ejercicio 1

Use MPI to implement the histogram program discussed in Section 2.7.1. Have
process 0 read in the input data and distribute it among the processes. Also have
process 0 print out the histogram.


INPUT DATA:
- Count: 20, Min: 0.300000, Max: 4.900000
- Bin count: 5, Bin width: 0.920000

OUTPUT DATA:
Histograma:
- Bin 0 [0.3 - 1.22): 6
- Bin 1 [1.22 - 2.14): 3
- Bin 2 [2.14 - 3.06): 2
- Bin 3 [3.06 - 3.98): 3
- Bin 4 [3.98 - 4.9): 6


## Ejercicio 2
Write an MPI program that uses a Monte Carlo method to estimate π.
Process 0 should read in the total number of tosses and broadcast it to the
other processes. Use MPI Reduce to find the global sum of the local variablenumber in circle, and have process 0 print the result. You may want to uselong long ints for the number of hits in the circle and the number of tosses,
since both may have to be very large to get a reasonable estimate of π.

### Flujo del programa en MPI-

- Proceso 0:
    - Lee el número total de tiros
    - Broadcast a todos los procesos
    
- Todos los procesos:
    - Calculan cuántos tiros les corresponden: tiros_por_proceso = total_tiros / num_procesos
    - Generan números aleatorios y cuentan hits en su porción
    
- Proceso 0:
    - Usa MPI_Reduce para sumar todos los hits
    - Calcula: π ≈ 4 × (total_hits / total_tiros)
    - Imprime el resultado
