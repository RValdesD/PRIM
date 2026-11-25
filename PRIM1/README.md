# Tarea 1 CC4102 — Algoritmo de Prim con colas binomiales y de Fibonacci

Implementación en C11 de Prim sobre una cola binomial y una cola de Fibonacci, generador de grafos aleatorios conexos, batería de experimentos y una aplicación (agrupamiento single-linkage).

## Requisitos
- Linux (o cualquier sistema POSIX) con `gcc` (C11), `make` y `bash`. Se usa `clock_gettime(CLOCK_MONOTONIC)`; no compila en Windows nativo.
- Para la batería completa: **≈ 1,5 GB de RAM libres** (el caso mayor, V = 2^20 y E = 2^24, ocupa ≈ 0,9 GB solo en el grafo) y varios minutos u horas según la máquina. No usa archivos temporales.

## Compilar
```
make            # genera ./prim_mst
make test       # pruebas de corrección (colas, Prim vs Kruskal, CLI, clustering)
make sanitize   # las mismas pruebas con AddressSanitizer + UBSan
```

## Ejecutar TODOS los experimentos (sin modificar código)
```
./run_experiments.sh                 # escribe resultados.csv y setup_experimental.txt
./run_experiments.sh otro.csv        # otro nombre de salida
SHIFT=4 SEEDS=3 ./run_experiments.sh # prueba rápida (exponentes - 4, 3 grafos por configuración)
```
(`make experiments` hace lo mismo.) Equivale a `./prim_mst experiments resultados.csv 10 0`. `SHIFT` va de 0 a 4; con 0 se reproduce exactamente el enunciado.

Series (10 grafos distintos por configuración, semillas 1–10, V = 2^i, E = 2^j):

| Serie | i | j | Medición |
|---|---|---|---|
| A | 20 | 20..24 | total |
| B | 18..22 | 24 | total |
| C | 18 | 18..22 | total + decreaseKey cronometrado |
| D | 14..18 | 22 | total + decreaseKey cronometrado |

Para cada grafo se corre Prim con ambas colas (el orden alterna según la paridad de la semilla) y se **verifica que ambos MST pesen lo mismo**; si no, el programa termina con error.

### Dos modos de medición
- `modo=total`: reloj solo alrededor de Prim (construcción de la cola + bucle principal); **sin** cronometrar cada decreaseKey, para no sumar overhead del reloj. No incluye generación del grafo ni liberación de memoria. Los contadores de operaciones (cortes/swaps) se registran igual, porque no tienen costo medible.
- `modo=amortized` (series C y D): mismo grafo, pero cronometrando cada llamada a decreaseKey. Su `tiempo_ms` incluye ese overhead y no debe usarse como costo total.

### Columnas del CSV
`serie, i, j, V, E, seed, cola, modo, tiempo_ms, decrease_calls, decrease_time_ms, operaciones, cascading_cuts, peso`

- `operaciones`: cortes (`cut`) en Fibonacci, intercambios (`swap`) en binomial.
- `cascading_cuts`: solo Fibonacci (subconjunto de los cortes).
- `decrease_time_ms`: 0 en modo `total`.

Promedio de tiempo total por configuración y cola:
```
awk -F, 'NR>1 && $8=="total" {k=$1","$2","$3","$7; s[k]+=$9; n[k]++} END{for(k in s) printf "%s,%.3f\n",k,s[k]/n[k]}' resultados.csv | sort
```
Para costo amortizado usa `$8=="amortized"` y las columnas 10–13.

`setup_experimental.txt` guarda CPU, cachés, RAM, SO, versión de gcc y el comando de compilación (flags `-O2`), listo para el informe.

## Uso manual
```
./prim_mst generate V E SEED grafo.txt          # genera un grafo (1 <= V, V-1 <= E <= V(V-1)/2)
./prim_mst run grafo.txt binomial|fibonacci|both [ROOT] [--edges]
./prim_mst bench V E SEED binomial|fibonacci|both [ROOT]   # genera en memoria y corre (sin archivo)
./prim_mst experiments out.csv [SEEDS=10] [SHIFT=0]
./prim_mst cluster grafo.txt K [--labels]
```
Formato del archivo de grafo: primera línea `V E`; luego E líneas `u v peso` (vértices desde 0, una sola copia por arista, pesos en (0,1]). Los errores de entrada terminan con código 1.

Generador: para cada vértice i ≥ 1 se agrega una arista a un padre uniforme en [0, i−1] (árbol cobertor), y luego se sortean las aristas restantes descartando lazos y repetidas (en grafos densos se muestrean los pares sin reintentos). Pesos uniformes en (0,1] con SplitMix64; la misma semilla produce el mismo grafo.

## Aplicación (bonus): agrupamiento single-linkage
`./prim_mst cluster grafo.txt K` calcula el MST con Prim + cola de Fibonacci, elimina las K−1 aristas más pesadas y entrega los K grupos (tamaños, pesos de las aristas cortadas y, con `--labels`, el grupo de cada vértice). Es útil cuando el grafo es de disimilitudes y denso (E ≫ V): el MST cuesta O(E + V log V) con Fibonacci, frente a O(E log V) con binomial, y el resto del algoritmo es solo ordenar V−1 aristas.

Ejemplo: `./prim_mst generate 1000 5000 7 g.txt && ./prim_mst cluster g.txt 4`

## Archivos
`main.c` (CLI), `grafo.c/.h` (grafo, generador, E/S), `binomial_heap.c/.h`, `fibonacci_heap.c/.h`, `prim.c` (Prim y métricas), `experiments.c/.h` (batería), `cluster.c/.h` (bonus), `tests.c`, `tests_cli.sh`, `run_experiments.sh`, `Makefile`. `make zip` arma `entrega.zip` (sin datasets).
