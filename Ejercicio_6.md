# Ejercicio6


**Implementa la multiplicación matriz-vector utilizando una distribución de la matriz en bloques de submatrices. Supón que los vectores se distribuyen entre los procesos diagonales. Nuevamente, puedes hacer que el proceso 0 lea la matriz y agregue las submatrices antes de enviarlas a los procesos. Supón que \(\texttt{comm.sz}\) es un cuadrado perfecto y que \(\sqrt{\texttt{comm.sz}}\) divide uniformemente el orden de la matriz.**

---

### Explicación de la Lógica y Flujo de Resolución

#### **Contexto**
Este problema plantea realizar una multiplicación matriz-vector en un entorno de memoria distribuida (por ejemplo, usando MPI), donde:
- La matriz se divide en bloques (submatrices) distribuidos entre los procesos.
- El vector se distribuye solo entre los procesos ubicados en la "diagonal" de la malla de procesos.
- El número total de procesos (`comm_sz`) debe ser un cuadrado perfecto (ej: 4, 9, 16).
- La dimensión de la matriz (\(n\)) debe ser divisible uniformemente por \(\sqrt{\texttt{comm.sz}}\).

---

#### **Flujo de Resolución**

1. **Organización de los procesos**:
   - Los procesos se organizan en una malla cuadrada de tamaño \( \sqrt{\texttt{comm.sz}} \times \sqrt{\texttt{comm.sz}} \).
   - Cada proceso se identifica por sus coordenadas \((fila, columna)\) en la malla.

2. **Distribución de la matriz**:
   - El proceso 0 lee la matriz completa.
   - Divide la matriz en bloques de tamaño:
     \[
     \text{tamaño del bloque} = \frac{n}{\sqrt{\texttt{comm.sz}}}
     \]
   - Cada bloque \((i, j)\) se envía al proceso correspondiente en la malla.

3. **Distribución del vector**:
   - Solo los procesos en la diagonal (donde \(fila = columna\)) reciben una parte del vector.
   - El vector se divide en segmentos de igual tamaño, y cada proceso diagonal recibe uno.

4. **Comunicación del vector**:
   - Cada proceso diagonal envía su segmento del vector a todos los procesos en su misma columna (difusión dentro de la columna).

5. **Cálculo local**:
   - Cada proceso multiplica su submatriz por el segmento del vector que recibió.
   - Esto produce un segmento local del vector resultante.

6. **Reducción de resultados**:
   - Los procesos en la misma fila suman (reducen) sus segmentos locales del resultado.
   - El proceso diagonal de cada fila recibe el resultado final de esa fila.

7. **Recolección del resultado**:
   - Los procesos diagonales envían sus segmentos del resultado al proceso 0.
   - El proceso 0 ensambla el vector resultante completo.

---

#### **Resumen Gráfico**

```
Proceso 0 lee matriz A y vector v.
Divide A en bloques y v en segmentos.

Envía bloque A(i,j) al proceso (i,j).
Envía segmento v_j al proceso diagonal (j,j).

Cada proceso (j,j) difunde v_j a su columna.

Cada proceso (i,j) calcula: resultado_local = A(i,j) * v_j.

Reduce por filas: resultado_fila_i = Σ resultado_local.

Proceso (i,i) envía resultado_fila_i a proceso 0.

Proceso 0 ensambla el vector resultado.
```

Este enfoque minimiza la comunicación y aprovecha la distribución de datos para paralelizar eficientemente la multiplicación matriz-vector.