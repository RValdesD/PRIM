#include "experiments.h"
#include "grafo.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/**
 * @brief Una configuración (i, j): V = 2^i nodos y E = 2^j aristas.
 */
typedef struct {
    char series;  /* 'A'..'D'. */
    int i, j;     /* Exponentes de V y E. */
    bool timed;   /* true si además se mide el tiempo de cada decreaseKey (6.3.2). */
} Config;

/**
 * @brief Ejecuta Prim con una cola y escribe una fila del CSV.
 * @param out Archivo CSV.
 * @param c Configuración actual.
 * @param g Grafo de entrada.
 * @param seed Semilla del grafo.
 * @param fib true para Fibonacci; false para binomial.
 * @param mode "total" (sin cronometrar decreaseKey) o "amortized" (cronometrado).
 * @param weight Salida: peso total del MST.
 * @return true si Prim terminó correctamente.
 */
static bool one_run(FILE *out, const Config *c, const Graph *g, int seed, bool fib, const char *mode, double *weight, int shift) {
    MSTResult r = fib ? prim_fibonacci(g, 0) : prim_binomial(g, 0);
    if (r.status != MST_OK) {
        fprintf(stderr, "Prim fallo: %s\n", mst_status_string(r.status));
        free_mst_result(&r);
        return false;
    }
    fprintf(out, "%c,%d,%d,%d,%lld,%d,%s,%s,%.6f,%llu,%.6f,%llu,%llu,%.17g\n", c->series, c->i - shift, c->j - shift,
            g->num_vertices, g->num_edges, seed, fib ? "fibonacci" : "binomial", mode, r.execution_time_ms,
            r.decrease_calls, r.decrease_time_ms, r.operations_count, r.cascading_cuts, r.total_weight);
    fflush(out);
    *weight = r.total_weight;
    free_mst_result(&r);
    return true;
}

/**
 * @brief Ejecuta ambas colas sobre un grafo, alternando cuál va primero, y compara los pesos.
 * @param out Archivo CSV.
 * @param c Configuración actual.
 * @param g Grafo de entrada.
 * @param seed Semilla del grafo; su paridad decide el orden.
 * @param mode Etiqueta del modo de medición.
 * @param shift Reducción de exponentes aplicada (solo para etiquetar).
 * @return true si ambas ejecuciones fueron válidas y los pesos coinciden.
 */
static bool pair(FILE *out, const Config *c, const Graph *g, int seed, const char *mode, int shift) {
    double w[2];
    bool fib_first = seed % 2 == 0;  /* Alternar el orden evita sesgo por caché/asignador. */
    for (int k = 0; k < 2; k++) {
        bool fib = (k == 0) == fib_first;
        if (!one_run(out, c, g, seed, fib, mode, &w[fib ? 1 : 0], shift)) return false;
    }
    if (fabs(w[0] - w[1]) > 1e-10 * fmax(1.0, fmax(fabs(w[0]), fabs(w[1])))) {
        fprintf(stderr, "Los MST difieren (serie %c, semilla %d): %.17g vs %.17g\n", c->series, seed, w[0], w[1]);
        return false;
    }
    return true;
}

bool run_experiments(const char *csv_path, int seeds, int shift) {
    Config cfg[20];
    int n = 0;
    for (int j = 20; j <= 24; j++) cfg[n++] = (Config){'A', 20, j, false};
    for (int i = 18; i <= 22; i++) cfg[n++] = (Config){'B', i, 24, false};
    for (int j = 18; j <= 22; j++) cfg[n++] = (Config){'C', 18, j, true};
    for (int i = 14; i <= 18; i++) cfg[n++] = (Config){'D', i, 22, true};

    FILE *out = fopen(csv_path, "w");
    if (!out) { perror(csv_path); return false; }
    fputs("serie,i,j,V,E,seed,cola,modo,tiempo_ms,decrease_calls,decrease_time_ms,operaciones,cascading_cuts,peso\n", out);

    bool ok = true;
    for (int k = 0; k < n && ok; k++) {
        int i = cfg[k].i - shift, j = cfg[k].j - shift;
        if (shift > 5) { fprintf(stderr, "SHIFT debe estar entre 0 y 5 (con mas, la serie D pide mas aristas que las posibles)\n"); ok = false; break; }
        fprintf(stderr, "Serie %c: i=%d j=%d (%d semillas)\n", cfg[k].series, i, j, seeds);
        for (int seed = 1; seed <= seeds && ok; seed++) {
            Graph *g = generate_random_connected_graph_seeded(1 << i, 1LL << j, (uint64_t)seed);
            if (!g) { fprintf(stderr, "No se pudo generar el grafo (memoria o V/E invalidos)\n"); ok = false; break; }
            /* Pasada 1: sin instrumentar -> tiempo total (6.3.1) y conteos de operaciones. */
            prim_set_decrease_timing(false);
            ok = pair(out, &cfg[k], g, seed, "total", shift);
            /* Pasada 2 (solo series C y D): cronometra cada decreaseKey (6.3.2a). */
            if (ok && cfg[k].timed) {
                prim_set_decrease_timing(true);
                ok = pair(out, &cfg[k], g, seed, "amortized", shift);
                prim_set_decrease_timing(false);
            }
            free_graph(g);
        }
    }
    if (fclose(out)) ok = false;
    fputs(ok ? "Experimentos terminados.\n" : "Experimentos terminaron con errores.\n", stderr);
    return ok;
}
