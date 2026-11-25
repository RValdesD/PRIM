#include "grafo.h"
#include "binomial_heap.h"
#include "fibonacci_heap.h"
#include "cluster.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/**
 * @brief Termina la prueba si no se cumple una condición.
 * @param condition Condición que se desea verificar.
 * @param line Línea de la comprobación que falló.
 */
static void check(bool condition, int line) {
    if (condition == false) {
        fprintf(stderr, "Fallo en tests.c, línea %d.\n", line);
        exit(EXIT_FAILURE);
    }
}

#define N 257
static unsigned rng = 1234567;

/**
 * @brief Genera el siguiente valor pseudoaleatorio de las pruebas.
 * @return Entero sin signo reproducible.
 */
static unsigned draw(void) {
    rng = rng * 1664525u + 1013904223u;
    return rng;
}

/**
 * @brief Compara dos pesos con tolerancia absoluta y relativa de 1e-10.
 * @param a Primer peso.
 * @param b Segundo peso.
 * @return true si los pesos coinciden dentro de la tolerancia.
 */
static bool close_weight(double a, double b) {
    return fabs(a - b) <= 1e-10 * fmax(1.0, fmax(fabs(a), fabs(b)));
}

/**
 * @brief Verifica orden, grados, padres y correspondencia de un árbol binomial.
 * @param h Cola que contiene el árbol.
 * @param x Raíz del subárbol que se verifica.
 * @param parent Padre esperado; NULL para una raíz.
 * @param seen Arreglo que registra los vértices ya visitados.
 * @return Cantidad de nodos visitados; termina la prueba si falla una condición.
 */
static int check_btree(BinomialHeap *h, BinomialNode *x, BinomialNode *parent, bool *seen) {
    check(x->v >= 0 &&x->v < h->max_capacity &&!seen[x->v], __LINE__);
    seen[x->v] = true;
    check(h->node_map[x->v] == x &&x->parent == parent, __LINE__);
    if (parent != NULL) {
        check(parent->key <= x->key, __LINE__);
    }
    int degree = x->degree -1;
    int count = 1;
    for (BinomialNode *c = x->child; c != NULL; c = c->sibling) {
        check(c->degree == degree, __LINE__);
        degree--;
        count += check_btree(h, c, x, seen);
    }
    check(degree == -1, __LINE__);
    return count;
}

/**
 * @brief Verifica todos los invariantes de una cola binomial.
 * @param h Cola que se desea verificar.
 */
static void check_b(BinomialHeap *h) {
    bool *seen = calloc((size_t) h->max_capacity, sizeof(bool));
    check(seen != NULL, __LINE__);
    int count = 0;
    int degree = -1;
    for (BinomialNode *x = h->head; x != NULL; x = x->sibling) {
        check(x->degree > degree, __LINE__);
        degree = x->degree;
        count += check_btree(h, x, NULL, seen);
    }
    check(count == h->num_elements, __LINE__);
    for (int i = 0; i < h->max_capacity; i++) {
        check(seen[i] == (h->node_map[i] != NULL), __LINE__);
    }
    free(seen);
}

/**
 * @brief Verifica una lista circular de Fibonacci y sus descendientes.
 * @param h Cola que contiene los nodos.
 * @param start Primer nodo de la lista; admite NULL.
 * @param parent Padre esperado; NULL para raíces.
 * @param seen Arreglo que registra los vértices ya visitados.
 * @return Cantidad de nodos visitados; termina la prueba si falla una condición.
 */
static int check_fring(FibonacciHeap *h, FibonacciNode *start, FibonacciNode *parent, bool *seen) {
    if (start == NULL) {
        return 0;
    }
    int count = 0;
    FibonacciNode *x = start;
    do {
        check(x->v >= 0 &&x->v < h->max_capacity &&!seen[x->v], __LINE__);
        seen[x->v] = true;
        check(h->node_map[x->v] == x &&x->parent == parent, __LINE__);
        check(x->right->left == x &&x->left->right == x, __LINE__);
        check(h->min->key <= x->key, __LINE__);
        if (parent != NULL) {
            check(parent->key <= x->key, __LINE__);
        } else {
            check(!x->mark, __LINE__);
        }
        int degree = 0;
        if (x->child != NULL) {
            FibonacciNode *c = x->child;
            do {
                degree++;
                check(degree <= h->num_elements, __LINE__);
                c = c->right;
            } while (c != x->child);
        }
        check(degree == x->degree, __LINE__);
        count += 1 + check_fring(h, x->child, x, seen);
        x = x->right;
    } while (x != start);
    return count;
}

/**
 * @brief Verifica todos los invariantes de una cola de Fibonacci.
 * @param h Cola que se desea verificar.
 */
static void check_f(FibonacciHeap *h) {
    bool *seen = calloc((size_t) h->max_capacity, sizeof(bool));
    check(seen != NULL, __LINE__);
    check(check_fring(h, h->min, NULL, seen) == h->num_elements, __LINE__);
    for (int i = 0; i < h->max_capacity; i++) {
        check(seen[i] == (h->node_map[i] != NULL), __LINE__);
    }
    free(seen);
}

/**
 * @brief Prueba ambas colas con 18000 operaciones y un modelo de referencia.
 */
static void heap_tests(void) {
    BinomialHeap *b = create_binomial_heap(N);
    FibonacciHeap *f = create_fibonacci_heap(N);
    check(b &&f, __LINE__);
    bool live[N] = { 0 };
    double keys[N] = { 0 };
    int count = 0;
    check(!binomial_heap_extract_min(b, NULL, NULL), __LINE__);
    check(!fibonacci_heap_extract_min(f, NULL, NULL), __LINE__);
    for (int step = 0; step < 18000; step++) {
        int v = (int) (draw() % N);
        unsigned op = draw() % 3;
        if (!live[v] &&op == 0) {
            /* La parte fraccionaria única evita empates ambiguos al comparar extracciones. */
            keys[v] = 1000 + (double) (draw() % 10000) + (double) v / 1024;
            live[v] = true;
            count++;
            check(binomial_heap_insert(b, v, keys[v]), __LINE__);
            check(fibonacci_heap_insert(f, v, keys[v]), __LINE__);
            check(!binomial_heap_insert(b, v, keys[v]), __LINE__);
            check(!fibonacci_heap_insert(f, v, keys[v]), __LINE__);
        } else {
            if (live[v] &&op == 1) {
                check(!binomial_heap_decrease_key(b, v, keys[v] + 1), __LINE__);
                check(!fibonacci_heap_decrease_key(f, v, keys[v] + 1), __LINE__);
                check(!binomial_heap_decrease_key(b, v, NAN), __LINE__);
                check(!fibonacci_heap_decrease_key(f, v, NAN), __LINE__);
                keys[v] -= 1 + (double) (draw() % 500);
                check(binomial_heap_decrease_key(b, v, keys[v]), __LINE__);
                check(fibonacci_heap_decrease_key(f, v, keys[v]), __LINE__);
            } else {
                if (count > 0 &&op == 2) {
                    int expected = -1;
                    for (int i = 0; i < N; i++) {
                        if (live[i] &&(expected < 0 || keys[i] < keys[expected])) {
                            expected = i;
                        }
                    }
                    int bv;
                    int fv;
                    double bk;
                    double fk;
                    check(binomial_heap_extract_min(b, &bv, &bk), __LINE__);
                    check(fibonacci_heap_extract_min(f, &fv, &fk), __LINE__);
                    check(bv == expected &&fv == expected &&bk == keys[expected] &&fk == keys[expected], __LINE__);
                    live[expected] = false;
                    count--;
                    check(!binomial_heap_decrease_key(b, expected, 0), __LINE__);
                    check(!fibonacci_heap_decrease_key(f, expected, 0), __LINE__);
                }
            }
        }
        check_b(b);
        check_f(f);
    }
    free_binomial_heap(b);
    free_fibonacci_heap(f);
    check(!create_binomial_heap(0) &&!create_fibonacci_heap(-1), __LINE__);
    puts("PASS heap reference model: 18000 operations + invariants");
}

/**
 * @brief Construye un árbol B10 y fuerza una cascada de dos cortes adicionales.
 */
static void cascade_test(void) {
    FibonacciHeap *h = create_fibonacci_heap(1025);
    check(h != NULL, __LINE__);
    for (int i = 0; i < 1025; i++) {
        check(fibonacci_heap_insert(h, i, (double) i), __LINE__);
    }
    check(fibonacci_heap_extract_min(h, NULL, NULL), __LINE__);
    check_f(h);
    FibonacciNode *y = NULL;
    FibonacciNode *z = NULL;
    FibonacciNode *other = NULL;
    for (int i = 1; i < 1025; i++) {
        FibonacciNode *p = h->node_map[i];
        if (p->parent &&p->parent->parent &&p->degree >= 2 &&p->parent->degree >= 2) {
            y = p;
            z = p->parent;
            break;
        }
    }
    check(y &&z, __LINE__);
    other = z->child;
    while (other == y) {
        other = other->right;
    }
    check(fibonacci_heap_decrease_key(h, other->v, -1), __LINE__);
    check(z->mark, __LINE__);
    check_f(h);
    int c1 = y->child->v;
    check(fibonacci_heap_decrease_key(h, c1, -2), __LINE__);
    check(y->mark, __LINE__);
    check_f(h);
    int c2 = y->child->v;
    unsigned long long before = h->cut_operations;
    check(fibonacci_heap_decrease_key(h, c2, -3), __LINE__);
    check(h->cut_operations - before == 3 &&h->cascading_cuts == 2, __LINE__);
    check(!y->parent &&!z->parent &&!y->mark &&!z->mark, __LINE__);
    check_f(h);
    double last = -INFINITY;
    double key;
    while (fibonacci_heap_extract_min(h, NULL, &key)) {
        check(key >= last, __LINE__);
        last = key;
        check_f(h);
    }
    free_fibonacci_heap(h);
    puts("PASS forced two-level Fibonacci cascade");
}

/**
 * @brief Arista auxiliar utilizada por el oráculo de Kruskal en las pruebas.
 */
typedef struct {
    int u;
    int v;
    double w;
} Edge;

/**
 * @brief Compara los pesos de dos aristas para ordenarlas mediante qsort.
 * @param a Puntero a la primera arista.
 * @param b Puntero a la segunda arista.
 * @return Valor negativo, cero o positivo según el orden de los pesos.
 */
static int compare(const void * a, const void * b) {
    const Edge *first = a;
    const Edge *second = b;
    if (first->w < second->w) {
        return -1;
    }
    if (first->w > second->w) {
        return 1;
    }
    return 0;
}

/**
 * @brief Busca el representante de un conjunto y comprime parcialmente el camino.
 * @param p Arreglo de padres de los conjuntos disjuntos.
 * @param v Elemento cuyo representante se busca.
 * @return Identificador del representante.
 */
static int find(int *p, int v) {
    while (p[v] != v) {
        p[v] = p[p[v]];
        v = p[v];
    }
    return v;
}

/**
 * @brief Calcula un peso de referencia independiente mediante Kruskal.
 * @param g Grafo conexo de entrada.
 * @return Peso del árbol de expansión mínima; termina la prueba si falla una condición.
 */
static double kruskal(const Graph *g) {
    Edge *edges = malloc((size_t) (g->num_edges + 1) * sizeof(Edge));
    int *p = malloc((size_t) g->num_vertices * sizeof(int));
    check(edges &&p, __LINE__);
    size_t n = 0;
    for (int u = 0; u < g->num_vertices; u++) {
        p[u] = u;
        for (AdjNode * a = g->adj_lists[u]; a != NULL; a = a->next) {
            if (u < a->dest) {
                edges[n].u = u;
                edges[n].v = a->dest;
                edges[n].w = a->weight;
                n++;
            }
        }
    }
    check(n == (size_t) g->num_edges, __LINE__);
    qsort(edges, n, sizeof(Edge), compare);
    double w = 0;
    int used = 0;
    for (size_t i = 0; i < n; i++) {
        int u = find(p, edges[i].u);
        int v = find(p, edges[i].v);
        if (u != v) {
            p[u] = v;
            w += edges[i].w;
            used++;
        }
    }
    check(used == g->num_vertices -1, __LINE__);
    free(edges);
    free(p);
    return w;
}

/**
 * @brief Verifica aristas, peso, conectividad y ausencia de ciclos, y libera el resultado.
 * @param g Grafo original.
 * @param root Raíz esperada del árbol.
 * @param r Resultado que se verifica y cuyos arreglos se liberan.
 * @param expected Peso esperado según el oráculo independiente.
 */
static void validate_mst(const Graph *g, int root, MSTResult *r, double expected) {
    check(r->status == MST_OK &&r->num_vertices == g->num_vertices &&r->edge_count == g->num_vertices -1, __LINE__);
    check(r->parent[root] == -1 &&r->key[root] == 0, __LINE__);
    int *p = malloc((size_t) g->num_vertices * sizeof(int));
    check(p != NULL, __LINE__);
    for (int i = 0; i < g->num_vertices; i++) {
        p[i] = i;
    }
    double sum = 0;
    for (int v = 0; v < g->num_vertices; v++) {
        if (v != root) {
            int u = r->parent[v];
            check(u >= 0 &&u < g->num_vertices, __LINE__);
            AdjNode * a = g->adj_lists[u];
            while (a &&a->dest != v) {
                a = a->next;
            }
            check(a &&a->weight == r->key[v], __LINE__);
            sum += r->key[v];
            int x = find(p, u);
            int y = find(p, v);
            check(x != y, __LINE__);
            p[x] = y;
        }
    }
    for (int i = 0; i < g->num_vertices; i++) {
        check(find(p, i) == find(p, root), __LINE__);
    }
    check(close_weight(sum, r->total_weight) &&close_weight(sum, expected), __LINE__);
    free(p);
    free_mst_result(r);
}

/**
 * @brief Ejecuta ambas versiones de Prim y verifica sus resultados.
 * @param g Grafo de entrada.
 * @param root Vértice inicial.
 * @param expected Peso esperado del árbol mínimo.
 */
static void compare_mst(Graph *g, int root, double expected) {
    MSTResult b = prim_binomial(g, root);
    MSTResult f = prim_fibonacci(g, root);
    validate_mst(g, root, &b, expected);
    validate_mst(g, root, &f, expected);
}

/**
 * @brief Carga un grafo desde una cadena utilizando un archivo temporal.
 * @param s Texto que contiene el grafo.
 * @return Grafo leído, o NULL si el texto es inválido.
 */
static Graph *parse(const char *s) {
    FILE *f = tmpfile();
    check(f != NULL, __LINE__);
    check(fputs(s, f) >= 0, __LINE__);
    rewind(f);
    Graph *g = read_graph(f);
    check(fclose(f) == 0, __LINE__);
    return g;
}

/**
 * @brief Prueba generación, lectura, validación y árboles mínimos frente a Kruskal.
 */
static void graph_tests(void) {
    Graph *g = parse("4 5\n0 1 1\n1 2 2\n2 3 3\n0 3 10\n0 2 4\n");
    check(g != NULL, __LINE__);
    for (int root = 0; root < 4; root++) {
        compare_mst(g, root, 6);
    }
    check(!add_edge(g, 0, 1, 2) &&!add_edge(g, 0, 0, 1) &&!add_edge(g, -1, 1, 1), __LINE__);
    check(!add_edge(g, 1, 3, NAN) &&!add_edge(g, 1, 3, INFINITY) &&!add_edge(g, 1, 3, 0), __LINE__);
    free_graph(g);
    g = create_graph(8);
    check(g != NULL, __LINE__);
    for (int i = 0; i < 8; i++) {
        for (int j = i + 1; j < 8; j++) {
            check(add_edge(g, i, j, 0.5), __LINE__);
        }
    }
    compare_mst(g, 5, 3.5);
    free_graph(g);
    g = create_graph(3);
    check(g != NULL, __LINE__);
    check(add_edge(g, 0, 1, 1), __LINE__);
    MSTResult b = prim_binomial(g, 0);
    MSTResult f = prim_fibonacci(g, 0);
    check(b.status == MST_DISCONNECTED &&f.status == MST_DISCONNECTED &&!b.parent &&!f.key, __LINE__);
    free_mst_result(&b);
    free_mst_result(&f);
    b = prim_binomial(g, 3);
    check(b.status == MST_INVALID, __LINE__);
    free_mst_result(&b);
    free_graph(g);
    check(!generate_random_connected_graph_seeded(0, 0, 1), __LINE__);
    check(!generate_random_connected_graph_seeded(3, 1, 1), __LINE__);
    check(!generate_random_connected_graph_seeded(3, 4, 1), __LINE__);
    const char *bad[] = {
        "",
        "-1 0",
        "99999999999999999999999 0",
        "3 4",
        "2 1 0 2 1",
        "2 1 0 0 1",
        "2 1 0 1 nan",
        "2 1 0 1 inf",
        "2 1 0 1 0",
        "2 1 0 1 -1",
        "2 1 0 1",
        "3 2 0 1 1 1 0 2",
        "1 0 junk",
        "2 1 0 1 1e999"
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        check(!parse(bad[i]), __LINE__);
    }
    for (int seed = 0; seed < 240; seed++) {
        int n = 1 + seed % 60;
        long long max = (long long) n * (n -1) / 2;
        long long e;
        if (seed % 3 == 0) {
            e = n -1;
        } else {
            if (seed % 3 == 1) {
                e = max;
            } else {
                e = n -1 + (long long) (draw() % (unsigned) (max - n + 2));
            }
        }
        g = generate_random_connected_graph_seeded(n, e, (uint64_t) seed);
        check(g &&g->num_edges == e, __LINE__);
        Graph *same = generate_random_connected_graph_seeded(n, e, (uint64_t) seed);
        check(same != NULL, __LINE__);
        for (int u = 0; u < n; u++) {
            AdjNode * a = g->adj_lists[u];
            AdjNode * c = same->adj_lists[u];
            bool *seen = calloc((size_t) n, sizeof(bool));
            check(seen != NULL, __LINE__);
            while (a &&c) {
                check(a->dest == c->dest &&a->weight == c->weight &&a->weight > 0 &&a->weight <= 1, __LINE__);
                check(a->dest != u &&!seen[a->dest], __LINE__);
                seen[a->dest] = true;
                a = a->next;
                c = c->next;
            }
            check(!a &&!c, __LINE__);
            free(seen);
        }
        double expected = kruskal(g);
        compare_mst(g, seed % n, expected);
        FILE *tmp = tmpfile();
        check(tmp &&write_graph(g, tmp), __LINE__);
        rewind(tmp);
        Graph *copy = read_graph(tmp);
        check(copy &&copy->num_edges == e, __LINE__);
        fclose(tmp);
        compare_mst(copy, 0, expected);
        free_graph(copy);
        free_graph(same);
        free_graph(g);
    }
    puts("PASS graph validation, ties, disconnection, reproducibility, text round trips");
    puts("PASS 240 generated graphs (tree/sparse/complete) against Kruskal + MST structure");
}

/**
 * @brief Prueba el agrupamiento single-linkage sobre un camino con pesos conocidos.
 */
static void cluster_tests(void) {
    Graph *g = parse("6 5\n0 1 1\n1 2 5\n2 3 1\n3 4 7\n4 5 1\n");
    check(g != NULL, __LINE__);
    MSTResult r = prim_fibonacci(g, 0);
    check(r.status == MST_OK, __LINE__);
    int label[6];
    double cuts[2];
    check(single_linkage_clusters(&r, 3, label, cuts), __LINE__);
    check(cuts[0] == 7 &&cuts[1] == 5, __LINE__);
    check(label[0] == label[1] &&label[2] == label[3] &&label[4] == label[5], __LINE__);
    check(label[0] != label[2] &&label[2] != label[4] &&label[0] != label[4], __LINE__);
    check(single_linkage_clusters(&r, 1, label, NULL), __LINE__);
    for (int v = 1; v < 6; v++) {
        check(label[v] == label[0], __LINE__);
    }
    check(single_linkage_clusters(&r, 6, label, NULL), __LINE__);
    for (int v = 0; v < 6; v++) {
        check(label[v] == v, __LINE__);
    }
    check(!single_linkage_clusters(&r, 0, label, NULL) &&!single_linkage_clusters(&r, 7, label, NULL), __LINE__);
    free_mst_result(&r);
    free_graph(g);
    puts("PASS single-linkage clustering");
}

/**
 * @brief Ejecuta todas las pruebas de corrección del proyecto.
 * @return EXIT_SUCCESS si todas las pruebas terminan correctamente.
 */
int main(void) {
    heap_tests();
    cascade_test();
    graph_tests();
    cluster_tests();
    puts("ALL TESTS PASSED");
    return EXIT_SUCCESS;
}

