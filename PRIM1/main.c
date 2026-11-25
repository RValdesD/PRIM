#include "grafo.h"
#include "cluster.h"
#include "experiments.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

/**
 * @brief Convierte una cadena de dígitos a unsigned long long y valida cotas.
 * @param s Cadena que contiene los caracteres numéricos a procesar.
 * @param max Valor máximo permitido para evitar desbordes o entradas fuera de rango.
 * @param n Puntero a la variable receptora del resultado numérico parseado.
 * @return true si la conversión fue exitosa y n <= max; false en caso contrario.
 */
static bool number(const char *s, unsigned long long max, unsigned long long *n) {
    if (!s || !*s) return false;
    for (const char *p = s; *p; p++) if (*p < '0' || *p > '9') return false;
    char *end; errno = 0; *n = strtoull(s, &end, 10);
    return !errno && !*end && *n <= max;
}

/**
 * @brief Muestra las instrucciones de uso del programa por stderr.
 * @param name Nombre o ruta del ejecutable actual (argv[0]).
 * @return Constante de salida EXIT_FAILURE para retorno inmediato.
 */
static int usage(const char *name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s generate V E SEED graph.txt\n", name);
    fprintf(stderr, "  %s run graph.txt binomial|fibonacci|both [ROOT] [--edges]\n", name);
    fprintf(stderr, "  %s bench V E SEED binomial|fibonacci|both [ROOT]\n", name);
    fprintf(stderr, "  %s experiments out.csv [SEEDS=10] [SHIFT=0]\n", name);
    fprintf(stderr, "  %s cluster graph.txt K [--labels]\n", name);
    return EXIT_FAILURE;
}

/**
 * @brief Ejecuta el algoritmo de Prim con la cola elegida, valida estado e imprime métricas.
 * @param g Puntero al grafo sobre el cual se calcula el árbol cobertor mínimo.
 * @param root Vértice inicial desde donde se inicia la expansión de Prim.
 * @param fib Indica si se usa cola de Fibonacci (true) o cola binomial (false).
 * @param edges Si es true, imprime las aristas resultantes del MST por stdout.
 * @param weight Puntero receptor donde se almacena el costo total del árbol obtenido.
 * @return true si la ejecución fue exitosa (MST_OK); false si ocurrió un error en Prim.
 */
static bool report(const Graph *g, int root, bool fib, bool edges, double *weight) {
    MSTResult r = fib ? prim_fibonacci(g, root) : prim_binomial(g, root);
    if (r.status != MST_OK) {
        fprintf(stderr, "Prim: %s\n", mst_status_string(r.status));
        free_mst_result(&r);
        return false;
    }
    printf("%s: vertices=%d edges=%d weight=%.17g time_ms=%.6f decrease_calls=%llu decrease_time_ms=%.6f %s=%llu cascading_cuts=%llu\n",
        fib ? "fibonacci" : "binomial", r.num_vertices, r.edge_count, r.total_weight, r.execution_time_ms,
        r.decrease_calls, r.decrease_time_ms, fib ? "cuts" : "swaps", r.operations_count, r.cascading_cuts);
    if (edges) {
        for (int v = 0; v < r.num_vertices; v++) {
            if (r.parent[v] >= 0) {
                printf("%d %d %.17g\n", r.parent[v], v, r.key[v]);
            }
        }
    }
    *weight = r.total_weight;
    free_mst_result(&r);
    return true;
}

/**
 * @brief Punto de entrada principal para generación de grafos, benchmarks y ejecución de Prim.
 * @param argc Cantidad de argumentos pasados por línea de comandos.
 * @param argv Arreglo de cadenas de argumentos de entrada.
 * @return EXIT_SUCCESS si la operación finaliza correctamente; EXIT_FAILURE ante errores de validación, entrada o memoria.
 */
int main(int argc, char **argv) {
    if (argc == 6 && !strcmp(argv[1], "generate")) {
        unsigned long long n, e, seed;
        if (!number(argv[2], INT_MAX, &n) || !n || !number(argv[3], LLONG_MAX, &e) || !number(argv[4], UINT64_MAX, &seed)) {
            return usage(argv[0]);
        }
        Graph *g = generate_random_connected_graph_seeded((int)n, (long long)e, (uint64_t)seed);
        if (!g) {
            fprintf(stderr, "Invalid V/E or insufficient memory. Require V>=1 and V-1<=E<=V(V-1)/2.\n");
            return EXIT_FAILURE;
        }
        FILE *f = fopen(argv[5], "w");
        if (!f) {
            perror(argv[5]);
            free_graph(g);
            return EXIT_FAILURE;
        }
        bool ok = write_graph(g, f);
        if (fclose(f)) ok = false;
        free_graph(g);
        if (!ok) {
            fprintf(stderr, "Graph write failed.\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* Subcomando directo en memoria RAM para manejar casos grandes (a una VM se le corrompio el journal con la version anterior) */
    if (argc >= 6 && argc <= 7 && !strcmp(argv[1], "bench")) {
        unsigned long long n, e, seed, root = 0;
        if (!number(argv[2], INT_MAX, &n) || !n || !number(argv[3], LLONG_MAX, &e) || !number(argv[4], UINT64_MAX, &seed)) {
            return usage(argv[0]);
        }
        bool both = !strcmp(argv[5], "both");
        bool fib = !strcmp(argv[5], "fibonacci");
        if (!both && !fib && strcmp(argv[5], "binomial")) return usage(argv[0]);
        if (argc == 7 && !number(argv[6], INT_MAX, &root)) return usage(argv[0]);

        Graph *g = generate_random_connected_graph_seeded((int)n, (long long)e, (uint64_t)seed);
        if (!g) {
            fprintf(stderr, "Failed to generate graph or out of memory.\n");
            return EXIT_FAILURE;
        }

        double w1 = 0, w2 = 0;
        bool ok = true;
        if (both) {
            ok = report(g, (int)root, false, false, &w1);
            if (ok) ok = report(g, (int)root, true, false, &w2);
            if (ok && fabs(w1 - w2) > 1e-10 * fmax(1.0, fmax(fabs(w1), fabs(w2)))) {
                fprintf(stderr, "MST weights differ.\n");
                ok = false;
            }
            if (ok) puts("MST weights agree (relative/absolute tolerance 1e-10).");
        } else {
            ok = report(g, (int)root, fib, false, &w1);
        }

        free_graph(g);
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* Bateria completa de experimentos (series A-D) en memoria; escribe un CSV. */
    if (argc >= 3 && argc <= 5 && !strcmp(argv[1], "experiments")) {
        unsigned long long seeds = 10, shift = 0;
        if ((argc >= 4 && (!number(argv[3], 1000, &seeds) || !seeds)) || (argc == 5 && !number(argv[4], 30, &shift))) {
            return usage(argv[0]);
        }
        return run_experiments(argv[2], (int)seeds, (int)shift) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* Aplicacion (bonus): agrupamiento single-linkage a partir del MST calculado con Prim + Fibonacci. */
    if (argc >= 4 && argc <= 5 && !strcmp(argv[1], "cluster")) {
        unsigned long long k;
        bool labels = false;
        if (!number(argv[3], INT_MAX, &k) || !k) return usage(argv[0]);
        if (argc == 5) {
            if (strcmp(argv[4], "--labels")) return usage(argv[0]);
            labels = true;
        }
        FILE *f = fopen(argv[2], "r");
        if (!f) { perror(argv[2]); return EXIT_FAILURE; }
        Graph *g = read_graph(f);
        fclose(f);
        if (!g) { fprintf(stderr, "Invalid graph file or allocation failure.\n"); return EXIT_FAILURE; }
        MSTResult r = prim_fibonacci(g, 0);
        int *label = NULL, *size = NULL;
        double *cuts = NULL;
        bool ok = r.status == MST_OK && k <= (unsigned long long)g->num_vertices;
        if (ok) {
            label = malloc((size_t)g->num_vertices * sizeof *label);
            size = calloc((size_t)k, sizeof *size);
            cuts = malloc((size_t)k * sizeof *cuts);
            ok = label && size && cuts && single_linkage_clusters(&r, (int)k, label, cuts);
        }
        if (!ok) {
            fprintf(stderr, "Cluster: %s\n", r.status == MST_OK ? "K invalido (1 <= K <= V) o sin memoria" : mst_status_string(r.status));
        } else {
            for (int v = 0; v < g->num_vertices; v++) size[label[v]]++;
            printf("k=%llu mst_weight=%.17g\n", k, r.total_weight);
            for (unsigned long long c = 0; c + 1 < k; c++) printf("aristas eliminadas (de mayor a menor): %.17g\n", cuts[c]);
            for (unsigned long long c = 0; c < k; c++) printf("cluster %llu: size=%d\n", c, size[c]);
            if (labels) for (int v = 0; v < g->num_vertices; v++) printf("%d %d\n", v, label[v]);
        }
        free(label); free(size); free(cuts);
        free_mst_result(&r);
        free_graph(g);
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (argc < 4 || argc > 6 || strcmp(argv[1], "run")) return usage(argv[0]);
    bool both = !strcmp(argv[3], "both"), fib = !strcmp(argv[3], "fibonacci"), edges = false;
    if (!both && !fib && strcmp(argv[3], "binomial")) return usage(argv[0]);
    unsigned long long root = 0;
    if (argc >= 5) {
        if (!strcmp(argv[4], "--edges")) {
            if (argc != 5) return usage(argv[0]);
            edges = true;
        } else if (!number(argv[4], INT_MAX, &root)) {
            return usage(argv[0]);
        }
    }
    if (argc == 6) {
        if (strcmp(argv[5], "--edges")) return usage(argv[0]);
        edges = true;
    }

    FILE *f = fopen(argv[2], "r");
    if (!f) {
        perror(argv[2]);
        return EXIT_FAILURE;
    }
    Graph *g = read_graph(f);
    fclose(f);
    if (!g) {
        fprintf(stderr, "Invalid graph file or allocation failure.\n");
        return EXIT_FAILURE;
    }

    double w1 = 0, w2 = 0;
    bool ok = report(g, (int)root, fib, edges, &w1);
    if (both && ok) {
        ok = report(g, (int)root, true, edges, &w2);
        if (ok && fabs(w1 - w2) > 1e-10 * fmax(1.0, fmax(fabs(w1), fabs(w2)))) {
            fprintf(stderr, "MST weights differ.\n");
            ok = false;
        }
        if (ok) puts("MST weights agree (relative/absolute tolerance 1e-10).");
    }
    free_graph(g);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
