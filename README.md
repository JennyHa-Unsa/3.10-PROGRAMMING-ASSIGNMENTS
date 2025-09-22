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

## Ejercicio 3

**3.3.** Escriba un programa MPI que calcule una suma global con estructura de árbol. Primero escriba su programa para el caso especial en el que `comm_sz` es una potencia de dos. Luego, después de que esta versión funcione, modifique su programa para que pueda manejar cualquier valor de `comm_sz`.

Voy a explicar cómo resolvería este ejercicio paso a paso:

## **Lógica del Algoritmo de Suma en Árbol**

### **Concepto Base:**
- Cada proceso tiene un valor local
- Los procesos se organizan en una estructura jerárquica (árbol binario)
- La suma se propaga desde las hojas hacia la raíz

### **Para comm_sz = potencia de 2:**
```
Proceso 0 (raíz)
     /       \
Proceso 1   Proceso 2
   /   \       /   \
P3     P4   P5     P6
```

## **Flujo de Trabajo Detallado**

### **Fase 1: Versión para Potencia de 2**

**1. Inicialización:**
```c
MPI_Init, MPI_Comm_size, MPI_Comm_rank
Cada proceso genera su valor local
```

**2. Algoritmo Principal:**
```python
# Para comm_sz = 8 (2^3)
niveles = log2(comm_sz)

for nivel desde 0 hasta niveles-1:
    if soy_proceso_receptor(nivel, rank):
        recibo datos del proceso emisor correspondiente
        sumo el valor recibido a mi valor local
    else if soy_proceso_emisor(nivel, rank):
        envío mi valor al proceso receptor
        termino mi participación
```

**3. Lógica de Pares:**
- **Paso 1:** Procesos 4,5,6,7 envían a 0,1,2,3
- **Paso 2:** Procesos 2,3 envían a 0,1  
- **Paso 3:** Proceso 1 envía a 0
- **Resultado:** Proceso 0 tiene la suma total

### **Fase 2: Extensión para Cualquier comm_sz**

**Modificaciones Necesarias:**

**1. Manejo de Procesos "Extra":**
```python
# Identificar procesos que participarán en el árbol binario
procesos_activos = mayor_potencia_de_2 <= comm_sz

if rank >= procesos_activos:
    # Proceso extra: envía directamente al proceso 0
    MPI_Send al proceso 0
    termina
else:
    # Participa en el árbol normal
```

**2. Árbol Adaptativo:**
- Construir árbol que incluya todos los procesos
- Los procesos "extra" se conectan al proceso 0 o a sus hijos
- Mantener balance lo mejor posible

## **Implementación Paso a Paso**

### **Pseudocódigo Completado:**

```c
int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    double local_val = generate_local_value(); // Valor de cada proceso
    
    // Fase 1: Procesos extras envían al proceso 0
    if (my_rank != 0 && my_rank >= next_power_of_two(comm_sz)) {
        MPI_Send(&local_val, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    } 
    else {
        // Participar en el árbol binario
        int remaining_processes = min(comm_sz, next_power_of_two(comm_sz));
        
        for (int gap = 1; gap < remaining_processes; gap *= 2) {
            if (my_rank % (2 * gap) == 0) {
                // Receptor: recibe y suma
                double received_val;
                MPI_Recv(&received_val, 1, MPI_DOUBLE, my_rank + gap, 
                        0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_val += received_val;
            } 
            else if (my_rank % gap == 0) {
                // Emisor: envía y termina
                MPI_Send(&local_val, 1, MPI_DOUBLE, my_rank - gap, 
                        0, MPI_COMM_WORLD);
                break;
            }
        }
        
        // Proceso 0 recibe de procesos extras
        if (my_rank == 0) {
            for (int extra = next_power_of_two(comm_sz); extra < comm_sz; extra++) {
                double extra_val;
                MPI_Recv(&extra_val, 1, MPI_DOUBLE, extra, 
                        0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_val += extra_val;
            }
        }
    }
    
    if (my_rank == 0) {
        printf("Suma total: %f\n", local_val);
    }
    
    MPI_Finalize();
    return 0;
}
```

## **Estrategia de Pruebas**

1. **Caso Simple:** comm_sz = 4 (potencia de 2)
2. **Caso General:** comm_sz = 6, 10, etc.
3. **Validación:** Comparar con suma secuencial
4. **Verificación:** Logs de comunicación entre procesos

## **Ventajas de Este Enfoque**

- **Eficiencia:** Complejidad O(log n) vs O(n) de enfoque lineal
- **Escalabilidad:** Funciona para cualquier número de procesos
- **Modularidad:** Fácil de extender para otras operaciones (producto, máximo, etc.)


