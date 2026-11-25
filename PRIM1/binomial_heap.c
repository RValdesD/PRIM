#include "binomial_heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

/**
 * @brief Crea un nuevo nodo binomial individual de grado 0.
 * @param v Identificador del vértice.
 * @param key Clave inicial del nodo.
 * @return Puntero al nodo creado, o NULL si falla la reserva de memoria.
 */
static BinomialNode* create_node(int v, double key) {
    BinomialNode* node = (BinomialNode*)malloc(sizeof(BinomialNode));
    if (!node) return NULL;
    node->v = v;
    node->key = key;
    node->degree = 0;
    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;
    return node;
}

/**
 * @brief Enlaza dos árboles del mismo grado; child pasa a ser hijo de parent.
 * @param child Raíz del árbol que se enlaza.
 * @param parent Raíz del árbol resultante.
 */
static void binomial_link(BinomialNode* child, BinomialNode* parent) {
    child->parent = parent;
    child->sibling = parent->child;
    parent->child = child;
    parent->degree++;
}

/**
 * @brief Fusiona dos listas de raíces ordenadas por grado.
 * @param h1 Primera lista de raíces.
 * @param h2 Segunda lista de raíces.
 * @return Cabeza de la lista fusionada.
 */
static BinomialNode* merge_root_lists(BinomialNode* h1, BinomialNode* h2) {
    if (!h1) return h2;
    if (!h2) return h1;

    BinomialNode* head = NULL;
    BinomialNode** tail = &head;

    while (h1 && h2) {
        if (h1->degree <= h2->degree) {
            *tail = h1;
            h1 = h1->sibling;
        } else {
            *tail = h2;
            h2 = h2->sibling;
        }
        tail = &((*tail)->sibling);
    }

    *tail = (h1) ? h1 : h2;
    return head;
}

/**
 * @brief Une dos colas binomiales y consolida los árboles del mismo grado.
 * @param h1 Raíces de la primera cola.
 * @param h2 Raíces de la segunda cola.
 * @return Cabeza de la cola resultante.
 */
static BinomialNode* binomial_heap_union(BinomialNode* h1, BinomialNode* h2) {
    BinomialNode* head = merge_root_lists(h1, h2);
    if (!head) return NULL;

    BinomialNode* prev = NULL;
    BinomialNode* curr = head;
    BinomialNode* next = curr->sibling;

    while (next) {
        if ((curr->degree != next->degree) ||
            (next->sibling && next->sibling->degree == curr->degree)) {
            prev = curr;
            curr = next;
        } else {
            if (curr->key <= next->key) {
                curr->sibling = next->sibling;
                binomial_link(next, curr);
            } else {
                if (!prev) {
                    head = next;
                } else {
                    prev->sibling = next;
                }
                binomial_link(curr, next);
                curr = next;
            }
        }
        next = curr->sibling;
    }
    return head;
}

/**
 * @brief Crea una cola binomial vacía.
 * @param max_v Cantidad máxima de vértices; debe ser positiva.
 * @return Cola creada, o NULL si la capacidad es inválida o falla la reserva de memoria.
 */
BinomialHeap* create_binomial_heap(int max_v) {
    if (max_v <= 0) return NULL;
    BinomialHeap* heap = (BinomialHeap*)malloc(sizeof(BinomialHeap));
    if (!heap) return NULL;

    heap->head = NULL;
    heap->num_elements = 0;
    heap->max_capacity = max_v;
    heap->swap_operations = 0;
    heap->node_map = (BinomialNode**)malloc(max_v * sizeof(BinomialNode*));

    if (!heap->node_map) {
        free(heap);
        return NULL;
    }

    for (int i = 0; i < max_v; i++) {
        heap->node_map[i] = NULL;
    }
    return heap;
}

/**
 * @brief Inserta un vértice que todavía no pertenece a la cola binomial.
 * @param heap Cola de prioridad.
 * @param v Identificador del vértice en [0, max_capacity).
 * @param key Clave inicial; admite infinito positivo, pero no NaN ni infinito negativo.
 * @return Nodo insertado, o NULL si los argumentos son inválidos o no hay memoria.
 */
BinomialNode* binomial_heap_insert(BinomialHeap* heap, int v, double key) {
    if (!heap || v < 0 || v >= heap->max_capacity ||
        heap->node_map[v] || isnan(key) || key == -INFINITY) return NULL;

    BinomialNode* node = create_node(v, key);
    if (!node) return NULL;

    heap->node_map[v] = node;
    /* Acarreo binario: detenerse en el primer grado libre. No se recorre
       el resto de las raíces. n inserciones realizan menos de n enlaces. */
    BinomialNode *rest = heap->head;
    BinomialNode *carry = node;
    while (rest && rest->degree == carry->degree) {
        BinomialNode *next = rest->sibling;
        if (carry->key <= rest->key) binomial_link(rest, carry);
        else { binomial_link(carry, rest); carry = rest; }
        rest = next;
    }
    carry->sibling = rest;
    heap->head = carry;
    heap->num_elements++;
    return node;
}

/**
 * @brief Extrae y libera el nodo de menor clave de la cola.
 * @param heap Cola de prioridad.
 * @param min_v Salida opcional para el identificador extraído; admite NULL.
 * @param min_key Salida opcional para la clave extraída; admite NULL.
 * @return true si se extrajo un nodo; false si la cola es NULL o está vacía.
 */
bool binomial_heap_extract_min(BinomialHeap* heap, int* min_v, double* min_key) {
    if (!heap || !heap->head) return false;

    /* 1. Localizar la raíz con la clave mínima en la lista de raíces */
    BinomialNode* min_node = heap->head;
    BinomialNode* min_prev = NULL;
    BinomialNode* curr = heap->head;
    BinomialNode* prev = NULL;

    while (curr) {
        if (curr->key < min_node->key) {
            min_node = curr;
            min_prev = prev;
        }
        prev = curr;
        curr = curr->sibling;
    }

    /* 2. Extraer el nodo mínimo de la lista de raíces */
    if (min_prev) {
        min_prev->sibling = min_node->sibling;
    } else {
        heap->head = min_node->sibling;
    }

    /* 3. Revertir la lista de hijos del nodo mínimo */
    BinomialNode* child_head = NULL;
    BinomialNode* child_curr = min_node->child;
    while (child_curr) {
        BinomialNode* next = child_curr->sibling;
        child_curr->parent = NULL;
        child_curr->sibling = child_head;
        child_head = child_curr;
        child_curr = next;
    }

    /* 4. Unir la lista de raíces remanente con la de los hijos revertidos */
    heap->head = binomial_heap_union(heap->head, child_head);

    if (min_v) *min_v = min_node->v;
    if (min_key) *min_key = min_node->key;

    heap->node_map[min_node->v] = NULL;
    free(min_node);
    heap->num_elements--;
    return true;
}

/**
 * @brief Disminuye la clave de un vértice presente en la cola.
 * @param heap Cola de prioridad.
 * @param v Identificador del vértice.
 * @param new_key Nueva clave, no mayor que la actual; rechaza NaN e infinito negativo.
 * @return true si se actualizó la clave; false si los argumentos son inválidos.
 */
bool binomial_heap_decrease_key(BinomialHeap* heap, int v, double new_key) {
    if (!heap || v < 0 || v >= heap->max_capacity) return false;

    BinomialNode* x = heap->node_map[v];
    if (!x || isnan(new_key) || new_key == -INFINITY || new_key > x->key) return false;

    x->key = new_key;
    BinomialNode* y = x->parent;

    /*
     * Ascenso en O(log n) intercambiando únicamente la carga útil (v, key)
     * y actualizando inmediatamente las referencias cruzadas en heap->node_map.
     */
    while (y && x->key < y->key) {
        heap->swap_operations++;

        /* Intercambio de valores */
        int temp_v = x->v;
        double temp_key = x->key;

        x->v = y->v;
        x->key = y->key;

        y->v = temp_v;
        y->key = temp_key;

        /* Actualización del mapeo de punteros para acceso directo */
        heap->node_map[x->v] = x;
        heap->node_map[y->v] = y;

        x = y;
        y = x->parent;
    }

    return true;
}

/**
 * @brief Indica si la cola está vacía.
 * @param heap Cola que se desea consultar; admite NULL.
 * @return true si la cola es NULL o no contiene elementos.
 */
bool binomial_heap_is_empty(const BinomialHeap* heap) {
    return (!heap || heap->num_elements == 0);
}

/**
 * @brief Libera recursivamente una lista de árboles binomiales.
 * @param node Primer nodo de la lista; admite NULL.
 */
static void free_tree_nodes(BinomialNode* node) {
    while (node) {
        BinomialNode* sibling = node->sibling;
        if (node->child) {
            free_tree_nodes(node->child);
        }
        free(node);
        node = sibling;
    }
}

/**
 * @brief Libera la cola y todos los nodos que todavía contiene.
 * @param heap Cola que se desea liberar; admite NULL.
 */
void free_binomial_heap(BinomialHeap* heap) {
    if (!heap) return;
    if (heap->head) {
        free_tree_nodes(heap->head);
    }
    if (heap->node_map) {
        free(heap->node_map);
    }
    free(heap);
}
