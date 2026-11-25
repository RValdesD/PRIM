#ifndef GRAFO_H
#define GRAFO_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/**
 * @brief Nodo de adyacencia; cada arista no dirigida se almacena en ambos sentidos.
 */
typedef struct AdjNode {
    int dest;
    double weight;
    struct AdjNode *next;
} AdjNode;

/**
 * @brief Grafo no dirigido; los grafos grandes reservan sus nodos en un bloque.
 */
typedef struct {
    int num_vertices;
    long long num_edges;
    AdjNode **adj_lists;
    AdjNode *node_pool;       /* NULL en grafos construidos con add_edge normal. */
    long long pool_capacity;  /* Número máximo de aristas en node_pool. */
} Graph;

/**
 * @brief Crea un grafo vacío representado mediante listas de adyacencia.
 * @param v Cantidad de vértices; debe ser positiva.
 * @return Grafo creado, o NULL si el tamaño es inválido o no hay memoria.
 */
Graph *create_graph(int v);

/**
 * @brief Añade una arista no dirigida; rechaza lazos, duplicados y pesos inválidos.
 * @param graph Grafo que se modifica.
 * @param u Primer extremo de la arista.
 * @param v Segundo extremo de la arista.
 * @param weight Peso positivo y finito.
 * @return true si se insertó; false si falla, sin modificar el grafo.
 */
bool add_edge(Graph *graph, int u, int v, double weight);

/**
 * @brief Comprueba si existe una arista entre dos vértices.
 * @param graph Grafo que se consulta.
 * @param u Primer extremo de la arista.
 * @param v Segundo extremo de la arista.
 * @return true si existe la arista; false si no existe o los argumentos son inválidos.
 */
bool edge_exists(const Graph *graph, int u, int v);

/**
 * @brief Genera un grafo simple y conexo reproducible a partir de una semilla.
 * @param v Cantidad positiva de vértices.
 * @param e Cantidad de aristas, entre v - 1 y v * (v - 1) / 2.
 * @param seed Semilla del generador pseudoaleatorio.
 * @return Grafo generado, o NULL si los argumentos son inválidos o no hay memoria.
 */
Graph *generate_random_connected_graph_seeded(int v, long long e, uint64_t seed);

/**
 * @brief Genera un grafo simple y conexo utilizando la semilla fija 42.
 * @param v Cantidad positiva de vértices.
 * @param e Cantidad de aristas, entre v - 1 y v * (v - 1) / 2.
 * @return Grafo generado, o NULL en caso de error.
 */
Graph *generate_random_connected_graph(int v, long long e);

/**
 * @brief Lee V E y luego E ternas u v peso; rechaza datos inválidos o sobrantes.
 * @param input Archivo de entrada; los identificadores comienzan en cero.
 * @return Grafo leído, o NULL si falla la lectura, la validación o la reserva de memoria.
 */
Graph *read_graph(FILE *input);

/**
 * @brief Escribe el grafo en texto, incluyendo una sola copia de cada arista.
 * @param graph Grafo que se desea guardar.
 * @param output Archivo de salida.
 * @return true si la escritura fue correcta; false en caso de error.
 */
bool write_graph(const Graph *graph, FILE *output);

/**
 * @brief Libera las listas de adyacencia y la estructura del grafo.
 * @param graph Grafo que se desea liberar; admite NULL.
 */
void free_graph(Graph *graph);

/**
 * @brief Estados de ejecución de Prim: éxito, entrada inválida y fallos detectados.
 */
typedef enum {
    MST_OK,
    MST_INVALID,
    MST_NO_MEMORY,
    MST_DISCONNECTED,
    MST_CLOCK_ERROR
} MSTStatus;

/**
 * @brief Resultado de Prim con arreglos propios; deben liberarse con free_mst_result.
 */
typedef struct {
    double total_weight;                 /* Peso total del MST. */
    unsigned long long operations_count; /* Operaciones estructurales de decreaseKey: cortes (cut) en Fibonacci, intercambios (swap) en binomial. */
    double execution_time_ms;            /* Tiempo de Prim (construcción de Q + bucle principal), sin lectura ni generación del grafo. */
    double decrease_time_ms;             /* Tiempo acumulado dentro de decreaseKey; solo si se activó prim_set_decrease_timing(true). */
    unsigned long long decrease_calls;   /* Cantidad de llamadas a decreaseKey. */
    unsigned long long cascading_cuts;   /* Cortes en cascada (solo Fibonacci; 0 en binomial). */
    int num_vertices;                    /* Cantidad de vértices del grafo. */
    int edge_count;                      /* Aristas del MST (V - 1 si el grafo es conexo). */
    int *parent;                         /* parent[v]: padre de v en el MST; -1 para la raíz. */
    double *key;                         /* key[v]: peso de la arista (parent[v], v); 0 para la raíz. */
    MSTStatus status;                    /* Estado de la ejecución; el resto es válido solo si es MST_OK. */
} MSTResult;

/**
 * @brief Calcula un árbol de expansión mínima mediante Prim y una cola binomial.
 * @param graph Grafo de entrada.
 * @param start_root Vértice inicial, dentro del rango del grafo.
 * @return Resultado con estado, peso, padres, claves y contadores; liberar con free_mst_result.
 */
MSTResult prim_binomial(const Graph *graph, int start_root);

/**
 * @brief Calcula un árbol de expansión mínima mediante Prim y una cola de Fibonacci.
 * @param graph Grafo de entrada.
 * @param start_root Vértice inicial, dentro del rango del grafo.
 * @return Resultado con estado, peso, padres, claves y contadores; liberar con free_mst_result.
 */
MSTResult prim_fibonacci(const Graph *graph, int start_root);

/**
 * @brief Activa o desactiva el cronometraje individual de cada decreaseKey.
 * @param on true para acumular decrease_time_ms (agrega overhead a execution_time_ms); false (por omisión) para medir el costo total sin instrumentar.
 */
void prim_set_decrease_timing(bool on);

/**
 * @brief Libera los arreglos del resultado y deja sus punteros en NULL.
 * @param result Resultado cuyos arreglos se desean liberar; admite NULL.
 */
void free_mst_result(MSTResult *result);

/**
 * @brief Obtiene una descripción textual del estado de ejecución de Prim.
 * @param status Estado que se desea describir.
 * @return Cadena constante; no debe liberarse.
 */
const char *mst_status_string(MSTStatus status);
#endif
