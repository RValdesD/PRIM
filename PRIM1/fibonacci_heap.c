#include "fibonacci_heap.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

/**
 * @brief Crea e inicializa una cola de Fibonacci vacía con capacidad para max_v vértices.
 * @param max_v Cantidad máxima de vértices soportados (identificadores de 0 a max_v - 1).
 * @return Puntero a la estructura FibonacciHeap creada, o NULL si la capacidad es inválida o falla malloc/calloc.
 */
FibonacciHeap *create_fibonacci_heap(int max_v) {
    if (max_v <= 0) {
        return NULL;
    }
    FibonacciHeap *heap = malloc(sizeof(FibonacciHeap));
    if (heap == NULL) {
        return NULL;
    }
    heap->node_map = calloc((size_t)max_v, sizeof(FibonacciNode *));
    heap->node_pool = calloc((size_t)max_v, sizeof(FibonacciNode));
    heap->root_buffer = malloc((size_t)max_v * sizeof(FibonacciNode *));
    if (heap->node_map == NULL || heap->node_pool == NULL || heap->root_buffer == NULL) {
        free(heap->node_map);
        free(heap->node_pool);
        free(heap->root_buffer);
        free(heap);
        return NULL;
    }
    heap->min = NULL;
    heap->num_elements = 0;
    heap->max_capacity = max_v;
    heap->cut_operations = 0;
    heap->cascading_cuts = 0;
    return heap;
}

/**
 * @brief Inserta un nuevo vértice con su clave en la lista de raíces de la cola de Fibonacci.
 * @param heap Puntero a la cola de Fibonacci.
 * @param v Identificador del vértice a insertar (debe estar en el rango [0, max_capacity)).
 * @param key Clave de prioridad asociada al vértice (no se admiten NaN ni -INFINITY).
 * @return Puntero al FibonacciNode insertado, o NULL en caso de argumentos inválidos o vértice ya presente.
 */
FibonacciNode *fibonacci_heap_insert(FibonacciHeap *heap, int v, double key) {
    if (heap == NULL || v < 0 || v >= heap->max_capacity) {
        return NULL;
    }
    if (heap->node_map[v] != NULL || isnan(key) || (isinf(key) && key < 0)) {
        return NULL;
    }

    FibonacciNode *node = &heap->node_pool[v];
    memset(node, 0, sizeof(FibonacciNode));
    node->v = v;
    node->key = key;

    if (heap->min == NULL) {
        node->left = node;
        node->right = node;
        heap->min = node;
    } else {
        node->right = heap->min->right;
        node->left = heap->min;
        heap->min->right->left = node;
        heap->min->right = node;
        if (node->key < heap->min->key) {
            heap->min = node;
        }
    }

    heap->node_map[v] = node;
    heap->num_elements++;
    return node;
}

/**
 * @brief Enlaza el árbol con raíz y como hijo del árbol con raíz x.
 * @param y Nodo raíz que pasará a ser hijo de x (se desmarca su flag mark).
 * @param x Nodo raíz que se convertirá en padre de y (su grado se incrementa en 1).
 */
static void link_trees(FibonacciNode *y, FibonacciNode *x) {
    y->left->right = y->right;
    y->right->left = y->left;
    y->parent = x;

    if (x->child == NULL) {
        x->child = y;
        y->left = y;
        y->right = y;
    } else {
        y->right = x->child->right;
        y->left = x->child;
        x->child->right->left = y;
        x->child->right = y;
    }
    x->degree++;
    y->mark = false;
}

/**
 * @brief Consolida la lista de raíces de la cola uniendo árboles del mismo grado hasta que no queden duplicados.
 * @param heap Puntero a la cola de Fibonacci a consolidar.
 */
static void consolidate(FibonacciHeap *heap) {
    /*
     * `a` indexa por grado: el grado máximo de un árbol de Fibonacci con n
     * nodos es O(log_phi n) (~1.44 log2 n); 128 es una cota segura para
     * cualquier n representable en un int, así que no hace falta reservar
     * memoria dinámica aquí.
     *
     * `roots`, en cambio, SÍ necesita hasta max_capacity casillas: la cola
     * inicial se construye con |V| inserciones sucesivas (sec. 3.4 del
     * enunciado), así que justo antes del primer extractMin puede haber
     * hasta max_capacity raíces (un árbol por vértice). Para no pagar ese
     * costo O(V) en cada llamada (que arruinaría el O(log V) amortizado de
     * las extracciones siguientes), se usa heap->root_buffer, reservado una
     * sola vez en create_fibonacci_heap y reutilizado en cada consolidate().
     */
    FibonacciNode *a[128];
    for (int i = 0; i < 128; i++) {
        a[i] = NULL;
    }

    FibonacciNode **roots = heap->root_buffer;
    int num_roots = 0;

    FibonacciNode *curr = heap->min;
    if (curr != NULL) {
        do {
            roots[num_roots++] = curr;
            curr = curr->right;
        } while (curr != heap->min);
    }

    for (int i = 0; i < num_roots; i++) {
        FibonacciNode *x = roots[i];
        int d = x->degree;
        while (d < 128 && a[d] != NULL) {
            FibonacciNode *y = a[d];
            if (x->key > y->key) {
                FibonacciNode *tmp = x;
                x = y;
                y = tmp;
            }
            link_trees(y, x);
            a[d] = NULL;
            d++;
        }
        if (d < 128) {
            a[d] = x;
        }
    }

    heap->min = NULL;
    for (int i = 0; i < 128; i++) {
        if (a[i] != NULL) {
            if (heap->min == NULL) {
                a[i]->left = a[i];
                a[i]->right = a[i];
                heap->min = a[i];
            } else {
                a[i]->right = heap->min->right;
                a[i]->left = heap->min;
                heap->min->right->left = a[i];
                heap->min->right = a[i];
                if (a[i]->key < heap->min->key) {
                    heap->min = a[i];
                }
            }
        }
    }
}

/**
 * @brief Extrae el nodo con la clave mínima de la cola y reestructura el heap mediante consolidate().
 * @param heap Puntero a la cola de Fibonacci.
 * @param min_v Puntero donde se almacenará el identificador del vértice extraído (admite NULL).
 * @param min_key Puntero donde se almacenará la clave mínima extraída (admite NULL).
 * @return true si la extracción fue exitosa; false si el heap es NULL o se encuentra vacío.
 */
bool fibonacci_heap_extract_min(FibonacciHeap *heap, int *min_v, double *min_key) {
    if (heap == NULL || heap->min == NULL) {
        return false;
    }

    FibonacciNode *z = heap->min;
    if (min_v != NULL) *min_v = z->v;
    if (min_key != NULL) *min_key = z->key;

    if (z->child != NULL) {
        FibonacciNode *c = z->child;
        FibonacciNode *start = c;
        do {
            FibonacciNode *next = c->right;
            c->parent = NULL;
            c->mark = false;
            c->right = heap->min->right;
            c->left = heap->min;
            heap->min->right->left = c;
            heap->min->right = c;
            c = next;
        } while (c != start);
        z->child = NULL;
    }

    heap->node_map[z->v] = NULL;

    if (z == z->right) {
        heap->min = NULL;
    } else {
        heap->min = z->right;
        z->left->right = z->right;
        z->right->left = z->left;
        consolidate(heap);
    }

    z->left = NULL;
    z->right = NULL;
    z->parent = NULL;
    z->degree = 0;
    z->mark = false;

    heap->num_elements--;
    return true;
}

/**
 * @brief Corta el enlace entre el nodo x y su padre y, moviendo x a la lista de raíces de la cola.
 * @param heap Puntero a la cola de Fibonacci.
 * @param x Nodo hijo que se desacopla.
 * @param y Nodo padre del cual se retira x.
 */
static void cut(FibonacciHeap *heap, FibonacciNode *x, FibonacciNode *y) {
    heap->cut_operations++;
    if (x->right == x) {
        y->child = NULL;
    } else {
        x->left->right = x->right;
        x->right->left = x->left;
        if (y->child == x) {
            y->child = x->right;
        }
    }
    y->degree--;

    x->right = heap->min->right;
    x->left = heap->min;
    heap->min->right->left = x;
    heap->min->right = x;

    x->parent = NULL;
    x->mark = false;
}

/**
 * @brief Realiza cortes en cascada hacia la raíz si el nodo ya había perdido previamente un hijo.
 * @param heap Puntero a la cola de Fibonacci.
 * @param y Nodo a evaluar para corte en cascada.
 */
static void cascading_cut(FibonacciHeap *heap, FibonacciNode *y) {
    FibonacciNode *z = y->parent;
    if (z != NULL) {
        if (!y->mark) {
            y->mark = true;
        } else {
            heap->cascading_cuts++;
            cut(heap, y, z);
            cascading_cut(heap, z);
        }
    }
}

/**
 * @brief Disminuye la clave asociada a un vértice presente en la cola de Fibonacci.
 * @param heap Puntero a la cola de Fibonacci.
 * @param v Identificador del vértice cuya clave se actualizará.
 * @param new_key Nuevo valor de la clave (debe ser menor o igual al valor actual).
 * @return true si la clave se actualizó correctamente; false si la clave es inválida o el nodo no existe.
 */
bool fibonacci_heap_decrease_key(FibonacciHeap *heap, int v, double new_key) {
    if (heap == NULL || v < 0 || v >= heap->max_capacity) {
        return false;
    }
    FibonacciNode *x = heap->node_map[v];
    if (x == NULL || isnan(new_key) || (isinf(new_key) && new_key < 0) || new_key > x->key) {
        return false;
    }

    x->key = new_key;
    FibonacciNode *y = x->parent;
    if (y != NULL && x->key < y->key) {
        cut(heap, x, y);
        cascading_cut(heap, y);
    }
    if (x->key < heap->min->key) {
        heap->min = x;
    }
    return true;
}

/**
 * @brief Determina si la cola de Fibonacci se encuentra vacía.
 * @param heap Puntero a la cola de Fibonacci.
 * @return true si la cola es NULL o no contiene elementos; false en caso contrario.
 */
bool fibonacci_heap_is_empty(const FibonacciHeap *heap) {
    return heap == NULL || heap->min == NULL;
}

/**
 * @brief Libera la estructura del heap y los arreglos de memoria asociados.
 * @param heap Puntero a la cola de Fibonacci a destruir (admite NULL).
 */
void free_fibonacci_heap(FibonacciHeap *heap) {
    if (heap == NULL) {
        return;
    }
    free(heap->node_pool);
    free(heap->node_map);
    free(heap->root_buffer);
    free(heap);
}
