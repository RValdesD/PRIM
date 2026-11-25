#include "cluster.h"
#include <stdlib.h>

/**
 * @brief Arista del MST identificada por su extremo hijo.
 */
typedef struct {
    int v;     /* Extremo hijo; el padre es mst->parent[v]. */
    double w;  /* Peso de la arista. */
} TreeEdge;

/**
 * @brief Ordena aristas de mayor a menor peso (desempate por vértice, para ser determinista).
 * @param a Primera arista (TreeEdge *).
 * @param b Segunda arista (TreeEdge *).
 * @return Negativo, cero o positivo según el orden descendente.
 */
static int heavier_first(const void *a, const void *b) {
    const TreeEdge *x = a;
    const TreeEdge *y = b;
    if (x->w != y->w) return x->w < y->w ? 1 : -1;
    return (x->v > y->v) - (x->v < y->v);
}

/**
 * @brief Busca el representante de un conjunto con compresión de caminos.
 * @param p Arreglo de padres de los conjuntos disjuntos.
 * @param v Elemento consultado.
 * @return Representante de v.
 */
static int find(int *p, int v) {
    while (p[v] != v) { p[v] = p[p[v]]; v = p[v]; }
    return v;
}

bool single_linkage_clusters(const MSTResult *mst, int k, int *label, double *cut_weights) {
    if (!mst || mst->status != MST_OK || !label || k < 1 || k > mst->num_vertices) return false;
    int n = mst->num_vertices;
    TreeEdge *edges = malloc((size_t)n * sizeof *edges);
    int *uf = malloc((size_t)n * sizeof *uf);
    int *rename_to = malloc((size_t)n * sizeof *rename_to);
    if (!edges || !uf || !rename_to) { free(edges); free(uf); free(rename_to); return false; }

    int m = 0;
    for (int v = 0; v < n; v++) {
        uf[v] = v;
        rename_to[v] = -1;
        if (mst->parent[v] >= 0) { edges[m].v = v; edges[m].w = mst->key[v]; m++; }
    }
    qsort(edges, (size_t)m, sizeof *edges, heavier_first);
    /* edges[0..k-2] son las aristas eliminadas; el resto se conserva. */
    for (int i = 0; i < k - 1; i++) if (cut_weights) cut_weights[i] = edges[i].w;
    for (int i = k - 1; i < m; i++) {
        int a = find(uf, edges[i].v), b = find(uf, mst->parent[edges[i].v]);
        if (a != b) uf[a] = b;
    }
    int next = 0;
    for (int v = 0; v < n; v++) {
        int r = find(uf, v);
        if (rename_to[r] < 0) rename_to[r] = next++;
        label[v] = rename_to[r];
    }
    free(edges); free(uf); free(rename_to);
    return next == k;
}
