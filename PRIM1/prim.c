#include "grafo.h"
#include "binomial_heap.h"
#include "fibonacci_heap.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

/**
 * @brief Devuelve el instante actual del reloj monotónico en nanosegundos.
 */
static inline long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* Cronometrar cada decreaseKey agrega dos lecturas del reloj por llamada; por eso
   está apagado por omisión y el costo total se mide sin instrumentar. */
static bool g_time_decrease = false;

/**
 * @brief Activa o desactiva el cronometraje individual de cada decreaseKey.
 * @param on true para acumular decrease_time_ms (distorsiona execution_time_ms); false para medir sin instrumentar.
 */
void prim_set_decrease_timing(bool on) { g_time_decrease = on; }

/**
 * @brief Calcula un árbol de expansión mínima utilizando la cola seleccionada.
 * @param g Grafo de entrada.
 * @param root Vértice inicial.
 * @param fib true para Fibonacci; false para binomial.
 * @return Resultado con estado explícito y arreglos propios; liberar con free_mst_result.
 */
static MSTResult prim(const Graph *g, int root, bool fib) {
    MSTResult r = {.status=MST_INVALID};
    if (!g || root<0 || root>=g->num_vertices) return r;
    struct timespec begin,end;
    bool clock_failed=false;
    if (clock_gettime(CLOCK_MONOTONIC,&begin)) { r.status=MST_CLOCK_ERROR; return r; }
    long long decrease_ns = 0; /* tiempo acumulado dentro de decrease_key */
    int n=g->num_vertices;
    r.status=MST_NO_MEMORY;
    r.parent=malloc((size_t)n*sizeof *r.parent);
    r.key=malloc((size_t)n*sizeof *r.key);
    BinomialHeap *b=fib ? NULL : create_binomial_heap(n);
    FibonacciHeap *f=fib ? create_fibonacci_heap(n) : NULL;
    if (!r.parent || !r.key || (fib ? !f : !b)) goto done;
    for (int v=0; v<n; v++) {
        r.parent[v]=-1; r.key[v]= v==root ? 0 : INFINITY;
        if (fib) { if (!fibonacci_heap_insert(f,v,r.key[v])) goto done; }
        else if (!binomial_heap_insert(b,v,r.key[v])) goto done;
    }
    r.num_vertices=n;
    for (int i=0; i<n; i++) {
        int u; double cost;
        if (fib) fibonacci_heap_extract_min(f,&u,&cost);
        else binomial_heap_extract_min(b,&u,&cost);
        if (!isfinite(cost)) { r.status=MST_DISCONNECTED; goto done; }
        r.total_weight+=cost;
        if (u!=root) r.edge_count++;
        for (AdjNode *p=g->adj_lists[u]; p; p=p->next) {
            int v=p->dest;
            bool resident=fib ? f->node_map[v]!=NULL : b->node_map[v]!=NULL;
            if (resident && p->weight<r.key[v]) {
                r.key[v]=p->weight; r.parent[v]=u; r.decrease_calls++;
                if (g_time_decrease) {
                    long long t0 = now_ns();
                    if (fib) fibonacci_heap_decrease_key(f,v,p->weight);
                    else binomial_heap_decrease_key(b,v,p->weight);
                    decrease_ns += now_ns() - t0;
                } else if (fib) fibonacci_heap_decrease_key(f,v,p->weight);
                else binomial_heap_decrease_key(b,v,p->weight);
            }
        }
    }
    r.status=MST_OK;
done:
    /* El reloj se lee antes de liberar las colas: liberar memoria no es parte del algoritmo. */
    clock_failed = clock_gettime(CLOCK_MONOTONIC,&end) != 0;
    r.decrease_time_ms = decrease_ns / 1e6;
    r.operations_count=fib ? (f ? f->cut_operations : 0) : (b ? b->swap_operations : 0);
    r.cascading_cuts=f ? f->cascading_cuts : 0;
    free_binomial_heap(b); free_fibonacci_heap(f);
    if (clock_failed) r.status=MST_CLOCK_ERROR;
    else r.execution_time_ms=(end.tv_sec-begin.tv_sec)*1000.0+(end.tv_nsec-begin.tv_nsec)/1e6;
    if (r.status!=MST_OK) {
        free(r.parent); free(r.key); r.parent=NULL; r.key=NULL;
        r.total_weight=0; r.edge_count=0;
    }
    return r;
}

/**
 * @brief Calcula un árbol de expansión mínima mediante Prim y una cola binomial.
 * @param g Grafo de entrada.
 * @param root Vértice inicial, dentro del rango del grafo.
 * @return Resultado con estado, peso, padres, claves y contadores; liberar con free_mst_result.
 */
MSTResult prim_binomial(const Graph *g,int root) { return prim(g,root,false); }

/**
 * @brief Calcula un árbol de expansión mínima mediante Prim y una cola de Fibonacci.
 * @param g Grafo de entrada.
 * @param root Vértice inicial, dentro del rango del grafo.
 * @return Resultado con estado, peso, padres, claves y contadores; liberar con free_mst_result.
 */
MSTResult prim_fibonacci(const Graph *g,int root) { return prim(g,root,true); }

/**
 * @brief Libera los arreglos del resultado y deja sus punteros en NULL.
 * @param r Resultado cuyos arreglos se desean liberar; admite NULL.
 */
void free_mst_result(MSTResult *r) {
    if (!r) return;
    free(r->parent); free(r->key); r->parent=NULL; r->key=NULL;
}
/**
 * @brief Obtiene una descripción textual del estado de ejecución de Prim.
 * @param s Estado que se desea describir.
 * @return Cadena constante; no debe liberarse.
 */
const char *mst_status_string(MSTStatus s) {
    switch(s) {
    case MST_OK: return "ok";
    case MST_INVALID: return "invalid graph/root";
    case MST_NO_MEMORY: return "allocation failed";
    case MST_DISCONNECTED: return "graph is disconnected";
    case MST_CLOCK_ERROR: return "monotonic clock failed";
    }
    return "unknown status";
}
