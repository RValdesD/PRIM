#ifndef FIBONACCI_HEAP_H
#define FIBONACCI_HEAP_H
#include <stdbool.h>
/**
 * @brief Nodo de Fibonacci con hermanos en lista circular; mark indica la pérdida de un hijo.
 */
typedef struct FibonacciNode {
    int v;                         /* Vértice que representa. */
    int degree;                    /* Cantidad de hijos. */
    double key;                    /* Costo asociado al vértice. */
    bool mark;                     /* true si perdió un hijo desde que dejó de ser raíz. */
    struct FibonacciNode *parent;  /* Padre; NULL en las raíces. */
    struct FibonacciNode *child;   /* Un hijo cualquiera; NULL si no tiene. */
    struct FibonacciNode *left;    /* Hermano anterior en la lista circular. */
    struct FibonacciNode *right;   /* Hermano siguiente en la lista circular. */
} FibonacciNode;

/**
 * @brief Cola de Fibonacci con mínimo, acceso directo por vértice y contadores de cortes.
 */
typedef struct {
    FibonacciNode *min;          /* Raíz de menor clave; NULL si la cola está vacía. */
    FibonacciNode **node_map;    /* node_map[v]: nodo de v, o NULL si no está en la cola. */
    FibonacciNode *node_pool;    /* Bloque con un nodo por vértice (evita un malloc por inserción). */
    FibonacciNode **root_buffer; /* Buffer reutilizable de tamaño max_capacity para consolidate(); evita malloc/free en cada extractMin. */
    int num_elements;            /* Elementos actuales. */
    int max_capacity;            /* Cantidad máxima de vértices. */
    unsigned long long cut_operations;   /* Cortes totales (el inicial de decreaseKey más los en cascada). */
    unsigned long long cascading_cuts;   /* Solo los cortes provocados por cascadingCut. */
} FibonacciHeap;

/**
 * @brief Crea una cola de Fibonacci vacía.
 * @param max_v Cantidad máxima de vértices; debe ser positiva.
 * @return Cola creada, o NULL si la capacidad es inválida o falla la reserva de memoria.
 */
FibonacciHeap *create_fibonacci_heap(int max_v);

/**
 * @brief Inserta un vértice que todavía no pertenece a la cola de Fibonacci.
 * @param heap Cola de prioridad.
 * @param v Identificador del vértice en [0, max_capacity).
 * @param key Clave inicial; admite infinito positivo, pero no NaN ni infinito negativo.
 * @return Nodo insertado, o NULL si los argumentos son inválidos o no hay memoria.
 */
FibonacciNode *fibonacci_heap_insert(FibonacciHeap *heap, int v, double key);

/**
 * @brief Extrae y libera el nodo de menor clave de la cola.
 * @param heap Cola de prioridad.
 * @param min_v Salida opcional para el identificador extraído; admite NULL.
 * @param min_key Salida opcional para la clave extraída; admite NULL.
 * @return true si se extrajo un nodo; false si la cola es NULL o está vacía.
 */
bool fibonacci_heap_extract_min(FibonacciHeap *heap, int *min_v, double *min_key);

/**
 * @brief Disminuye la clave de un vértice presente en la cola.
 * @param heap Cola de prioridad.
 * @param v Identificador del vértice.
 * @param new_key Nueva clave, no mayor que la actual; rechaza NaN e infinito negativo.
 * @return true si se actualizó la clave; false si los argumentos son inválidos.
 */
bool fibonacci_heap_decrease_key(FibonacciHeap *heap, int v, double new_key);

/**
 * @brief Indica si la cola está vacía.
 * @param heap Cola que se desea consultar; admite NULL.
 * @return true si la cola es NULL o no contiene elementos.
 */
bool fibonacci_heap_is_empty(const FibonacciHeap *heap);

/**
 * @brief Libera la cola y todos los nodos que todavía contiene.
 * @param heap Cola que se desea liberar; admite NULL.
 */
void free_fibonacci_heap(FibonacciHeap *heap);
#endif
