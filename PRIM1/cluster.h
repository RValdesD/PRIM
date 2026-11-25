#ifndef CLUSTER_H
#define CLUSTER_H
#include "grafo.h"
/**
 * @brief Agrupamiento jerárquico single-linkage: elimina las k-1 aristas más pesadas del MST.
 * @param mst Resultado de Prim con estado MST_OK.
 * @param k Cantidad de grupos deseada, en [1, mst->num_vertices].
 * @param label Arreglo de mst->num_vertices enteros; recibe la etiqueta [0, k) de cada vértice.
 * @param cut_weights Arreglo opcional de k-1 doubles; recibe los pesos eliminados, de mayor a menor. Admite NULL.
 * @return true si el agrupamiento se calculó; false si los argumentos son inválidos o falta memoria.
 */
bool single_linkage_clusters(const MSTResult *mst, int k, int *label, double *cut_weights);
#endif
