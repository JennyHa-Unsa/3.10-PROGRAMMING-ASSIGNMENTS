# Ejercicio 4

**Escriba un programa MPI que calcule una suma global usando un patrón "mariposa" (butterfly). Primero escriba su programa para el caso especial en que el tamaño del comunicador (comn_sz) es una potencia de dos. ¿Puede modificar su programa para que maneje cualquier número de procesos?**



## Explicación de la lógica de resolución:

### **Concepto del algoritmo "Butterfly" (Mariposa):**
El algoritmo butterfly es un patrón de comunicación que permite realizar operaciones de reducción (como la suma global) en tiempo logarítmico. Cada proceso intercambia datos con otro proceso en cada paso, duplicando el alcance de los datos combinados en cada iteración.

### **Lógica para cuando el número de procesos es potencia de 2:**

1. **Inicialización:**
   - Cada proceso tiene un valor local que contribuirá a la suma global.
   - Se determina el número de etapas (steps) como `log2(p)`, donde `p` es el número de procesos.

2. **Comunicación en etapas:**
   - En cada etapa `k` (comenzando desde 0):
     - **Pareja de comunicación:** Cada proceso calcula con qué otro proceso debe comunicarse en esta etapa usando operaciones de bits (XOR o desplazamientos).
     - **Intercambio y suma:** Los procesos pareados intercambian sus valores actuales y suman el valor recibido al suyo propio.
     - Después de `log2(p)` etapas, todos los procesos tendrán la suma global.

3. **Ejemplo con 4 procesos (p=4, etapas=2):**
   - **Etapa 0:** Pares (0,1), (2,3) se comunican y suman.
   - **Etapa 1:** Pares (0,2), (1,3) se comunican y suman.
   - Resultado: Todos los procesos tienen la suma total.

### **Extensión para cualquier número de procesos (no potencia de 2):**

1. **Enfoque 1: Usar MPI_Reduce y MPI_Bcast**
   - Se puede realizar una reducción a un proceso raíz y luego una difusión, aunque esto no sigue estrictamente el patrón butterfly y tiene un cuello de botella en el proceso raíz.

2. **Enfoque 2: Simular procesos virtuales**
   - Si el número de procesos no es potencia de 2, se pueden "simular" procesos adicionales hasta la siguiente potencia de 2.
   - Los procesos reales participan normalmente; los procesos virtuales no existen pero su lógica se maneja con condicionales para evitar comunicación con rangos inexistentes.

3. **Enfoque 3: Algoritmo butterfly adaptable**
   - Modificar el algoritmo para que en cada etapa, si un proceso no tiene un par válido (porque su pareja calculada excede el tamaño del comunicador), simplemente no realiza comunicación en esa etapa.
   - Esto requiere verificar en cada etapa si el par de comunicación es menor que el tamaño del comunicador.

### **Implementación práctica:**
- **Para potencia de 2:** Usar desplazamientos de bits para calcular pares.
- **Para cualquier número:** Incluir verificaciones de límites y posiblemente usar operaciones colectivas de MPI que ya estén optimizadas para cualquier tamaño.


## Caso Potencia de 2

Aquí tienes la implementación en C++ usando MPI con estilo moderno (sin clases):

```cpp
#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

// Función para verificar si un número es potencia de 2
bool is_power_of_two(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

// Algoritmo butterfly para suma global
double butterfly_sum(double local_value, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    double sum = local_value;
    int stages = std::log2(size);
    
    for (int stage = 0; stage < stages; ++stage) {
        int partner = rank ^ (1 << stage);  // XOR para calcular el compañero
        
        double received_value;
        // Intercambiar y sumar con el compañero
        MPI_Sendrecv(&sum, 1, MPI_DOUBLE, partner, 0,
                    &received_value, 1, MPI_DOUBLE, partner, 0,
                    comm, MPI_STATUS_IGNORE);
        
        sum += received_value;
    }
    
    return sum;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Verificar que el número de procesos es potencia de 2
    if (!is_power_of_two(size)) {
        if (rank == 0) {
            std::cerr << "Error: Number of processes must be a power of 2" << std::endl;
            std::cerr << "Current number of processes: " << size << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Cada proceso tiene un valor local (por ejemplo, su rank + 1)
    double local_value = static_cast<double>(rank + 1);
    
    // Calcular suma global usando butterfly
    double global_sum = butterfly_sum(local_value, MPI_COMM_WORLD);
    
    // Verificar el resultado (la suma de 1 + 2 + ... + n = n(n+1)/2)
    if (rank == 0) {
        double expected_sum = size * (size + 1) / 2.0;
        std::cout << "Global sum computed: " << global_sum << std::endl;
        std::cout << "Expected sum: " << expected_sum << std::endl;
        std::cout << "Error: " << std::abs(global_sum - expected_sum) << std::endl;
    }
    
    // Esperar a que todos los procesos terminen antes de imprimir
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Cada proceso imprime su contribución y el resultado
    std::cout << "Process " << rank << ": local value = " << local_value 
              << ", global sum = " << global_sum << std::endl;
    
    MPI_Finalize();
    return 0;
}
```

### Explicación de la lógica implementada:

### **1. Función `is_power_of_two`:**
- Usa operaciones bitwise para verificar si un número es potencia de 2
- `n & (n - 1) == 0` es verdadero solo para potencias de 2

### **2. Función `butterfly_sum`:**
- **Parámetros:** Valor local del proceso y comunicador MPI
- **Variables:**
  - `sum`: Acumula la suma parcial en cada etapa
  - `stages`: Número de etapas = log₂(size)

### **3. Algoritmo Butterfly:**
- **Cálculo del compañero:** `partner = rank ^ (1 << stage)`
  - `stage = 0`: compañeros difieren en el bit 0 (0↔1, 2↔3, etc.)
  - `stage = 1`: compañeros difieren en el bit 1 (0↔2, 1↔3, etc.)
- **Comunicación:** `MPI_Sendrecv` para intercambiar valores de forma segura
- **Acumulación:** Cada proceso suma el valor recibido al suyo propio

### **4. Ejemplo con 4 procesos:**
```
Etapa 0:
  Proceso 0 ↔ Proceso 1
  Proceso 2 ↔ Proceso 3
  Resultados: (0+1), (1+0), (2+3), (3+2)

Etapa 1:
  Proceso 0 ↔ Proceso 2 (valores: 0+1 y 2+3)
  Proceso 1 ↔ Proceso 3 (valores: 1+0 y 3+2)
  Resultado final: Todos tienen 0+1+2+3 = 6
```

### **5. Verificación:**
- Se usa la fórmula n(n+1)/2 para verificar la suma correcta
- Cada proceso imrime su contribución y el resultado final

### **Compilación y ejecución:**
```bash
mpic++ -std=c++17 -o butterfly butterfly.cpp
mpiexec -n 4 ./butterfly
```

Esta implementación es eficiente (O(log n)) y sigue estrictamente el patrón butterfly para potencias de 2.

## Caso general