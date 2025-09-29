# Ejercicio 5

### Traducción al español

**Ejercicio 5**  
Implementa la multiplicación matriz-vector utilizando una distribución de la matriz por bloques de columnas. Puedes hacer que el proceso 0 lea la matriz y simplemente use un bucle de envíos para distribuirla entre los procesos. Supón que la matriz es cuadrada de orden \(n\) y que \(n\) es divisible uniformemente por \(\operatorname{comm.sz}\). Quizá quieras revisar la función de MPI `MPI_Reduce_scatter`.

---

### Explicación de la lógica y flujo de resolución

#### **Contexto**  
Se trata de un problema de programación paralela usando MPI. La matriz \(A\) (de tamaño \(n \times n\)) y el vector \(x\) (de tamaño \(n\)) deben multiplicarse en paralelo, distribuyendo la matriz por **bloques de columnas** entre los procesos.

---

#### **Lógica de distribución**
- **Matriz A**: Se divide en bloques de columnas. Si hay `p` procesos, cada proceso recibe \(n/p\) columnas consecutivas de \(A\).
- **Vector x**: Como cada proceso necesita las componentes de \(x\) correspondientes a sus columnas de \(A\), el vector \(x\) completo se envía a todos los procesos (o se distribuye según necesidad).
- **Multiplicación parcial**: Cada proceso multiplica su bloque de \(A\) por las componentes correspondientes de \(x\), obteniendo un segmento del vector resultante \(y\).
- **Combinación de resultados**: Las contribuciones parciales de cada proceso deben sumarse adecuadamente para formar el vector \(y\) final. Aquí es donde `MPI_Reduce_scatter` puede ser útil.

---

#### **Flujo paso a paso**
1. **Proceso 0 lee la matriz y el vector**.
2. **Distribución**:
   - Proceso 0 envía a cada proceso `i` su bloque de columnas de \(A\) (usando `MPI_Send` o `MPI_Scatter`).
   - El vector \(x\) se envía completo a todos los procesos (usando `MPI_Bcast`).
3. **Cálculo local**:
   - Cada proceso multiplica su bloque de \(A\) (\(n \times n/p\)) por el segmento correspondiente de \(x\) (\(n/p \times 1\)), obteniendo un vector parcial de tamaño \(n\).
4. **Reducción y combinación**:
   - Se usa `MPI_Reduce_scatter` para sumar las contribuciones parciales de todos los procesos y que cada proceso reciba solo una parte del resultado final \(y\).
   - Alternativamente, se puede usar `MPI_Reduce` con `MPI_SUM` y luego repartir el resultado.
5. **Recolección o impresión** (opcional, según el enunciado).

---

#### **Esquema visual**
```
Proceso 0: 
  A = [A0 | A1 | ... | A_{p-1}]   (bloques de columnas)
  x = [x0, x1, ..., x_{p-1}]       (segmentos correspondientes)

Cada proceso i:
  Recibe Ai (n x n/p) y todo x (pero solo usa x_i correspondiente a sus columnas).
  Calcula: y_local = Ai * x_i   (vector de tamaño n)
  
MPI_Reduce_scatter: 
  Suma todos y_local y distribuye partes del resultado y final.
```

---

#### **Nota sobre `MPI_Reduce_scatter`**
Esta función combina una reducción (suma de vectores) y un scatter (reparto del resultado en bloques). Es ideal aquí porque:
- Cada proceso tiene un vector `y_local` de longitud \(n\).
- Tras sumarlos, cada proceso recibe un segmento de tamaño \(n/p\) del vector resultante \(y\).

---

### Resumen
El algoritmo aprovecha el paralelismo dividiendo la matriz por columnas, calculando contribuciones parciales y luego combinándolas eficientemente con operaciones colectivas de MPI.