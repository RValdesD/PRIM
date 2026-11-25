#ifndef BINOMIAL_HEAP_H
#define BINOMIAL_HEAP_H
#include <stdbool.h>
/**
 * @brief Nodo de un árbol binomial. La carga (v, key) puede cambiar durante decreaseKey.
 */
typedef struct BinomialNode {
    int v, degree;                                     /* v: vértice que representa; degree: cantidad de hijos. */
    double key;                                        /* Costo asociado al vértice. */
    struct BinomialNode *parent, *child, *sibling;     /* Padre, hijo de mayor grado y siguiente hermano (raíces: lista ordenada por grado). */
} BinomialNode;

/**
 * @brief Cola binomial con raíces, acceso directo por vértice y contador de intercambios.
 */
typedef struct {
    BinomialNode *head, **node_map;      /* head: primera raíz (menor grado); node_map[v]: nodo que representa a v, o NULL si no está en la cola. */
    int num_elements, max_capacity;      /* Elementos actuales y cantidad máxima de vértices. */
    unsigned long long swap_operations;  /* Intercambios realizados por decreaseKey. */
} BinomialHeap;

/**
 * @brief Crea una cola binomial vacía.
 * @param max_v Cantidad máxima de vértices; debe ser positiva.
 * @return Cola creada, o NULL si la capacidad es inválida o falla la reserva de memoria.
 */
BinomialHeap *create_binomial_heap(int max_v);

/**
 * @brief Inserta un vértice que todavía no pertenece a la cola binomial.
 * @param heap Cola de prioridad.
 * @param v Identificador del vértice en [0, max_capacity).
 * @param key Clave inicial; admite infinito positivo, pero no NaN ni infinito negativo.
 * @return Nodo insertado, o NULL si los argumentos son inválidos o no hay memoria.
 */
BinomialNode *binomial_heap_insert(BinomialHeap *heap, int v, double key);

/**
 * @brief Extrae y libera el nodo de menor clave de la cola.
 * @param heap Cola de prioridad.
 * @param min_v Salida opcional para el identificador extraído; admite NULL.
 * @param min_key Salida opcional para la clave extraída; admite NULL.
 * @return true si se extrajo un nodo; false si la cola es NULL o está vacía.
 */
bool binomial_heap_extract_min(BinomialHeap *heap, int *min_v, double *min_key);

/**
 * @brief Disminuye la clave de un vértice presente en la cola.
 * @param heap Cola de prioridad.
 * @param v Identificador del vértice.
 * @param new_key Nueva clave, no mayor que la actual; rechaza NaN e infinito negativo.
 * @return true si se actualizó la clave; false si los argumentos son inválidos.
 */
bool binomial_heap_decrease_key(BinomialHeap *heap, int v, double new_key);

/**
 * @brief Indica si la cola está vacía.
 * @param heap Cola que se desea consultar; admite NULL.
 * @return true si la cola es NULL o no contiene elementos.
 */
bool binomial_heap_is_empty(const BinomialHeap *heap);

/**
 * @brief Libera la cola y todos los nodos que todavía contiene.
 * @param heap Cola que se desea liberar; admite NULL.
 */
void free_binomial_heap(BinomialHeap *heap);
#endif
