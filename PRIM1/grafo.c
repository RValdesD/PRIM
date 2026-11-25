#include "grafo.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Crea un grafo vacío representado mediante listas de adyacencia.
 * @param v Cantidad de vértices; debe ser positiva.
 * @return Grafo creado, o NULL si el tamaño es inválido o no hay memoria.
 */
Graph *create_graph(int v) {
    if (v <= 0 || (size_t)v > SIZE_MAX / sizeof(AdjNode *)) {
        return NULL;
    }

    Graph *g = malloc(sizeof(Graph));
    if (g == NULL) {
        return NULL;
    }

    g->num_vertices = v;
    g->num_edges = 0;
    g->node_pool = NULL;
    g->pool_capacity = 0;
    g->adj_lists = malloc(v * sizeof(AdjNode *));
    if (g->adj_lists == NULL) {
        free(g);
        return NULL;
    }

    for (int i = 0; i < v; i++) {
        g->adj_lists[i] = NULL;
    }
    return g;
}

/**
 * @brief Reserva de una vez los nodos de adyacencia de un grafo grande.
 * @param g Grafo todavía vacío.
 * @param capacity Cantidad de aristas que se agregarán.
 * @return true si la reserva tuvo éxito.
 */
static bool reserve_nodes(Graph *g, long long capacity) {
    if (capacity == 0) {
        return true;
    }
    if (capacity < 0 || (unsigned long long)capacity > SIZE_MAX / (2 * sizeof(AdjNode))) {
        return false;
    }
    g->node_pool = malloc((size_t)capacity * 2 * sizeof(AdjNode));
    if (g->node_pool == NULL) {
        return false;
    }
    g->pool_capacity = capacity;
    return true;
}

/**
 * @brief Inserta dos nodos ya reservados; no busca duplicados.
 * @param g Grafo que tiene un bloque de nodos suficiente.
 * @param u Primer extremo.
 * @param v Segundo extremo.
 * @param w Peso de la arista.
 */
static void append_pooled_edge(Graph *g, int u, int v, double w) {
    AdjNode *a = &g->node_pool[2 * (size_t)g->num_edges];
    AdjNode *b = a + 1;
    a->dest = v;
    a->weight = w;
    a->next = g->adj_lists[u];
    b->dest = u;
    b->weight = w;
    b->next = g->adj_lists[v];
    g->adj_lists[u] = a;
    g->adj_lists[v] = b;
    g->num_edges++;
}

/* Filtro de Bloom: puede dar falsos positivos, nunca falsos negativos.
   Si los bits ya están presentes, edge_exists confirma la arista. */
typedef struct {
    unsigned char *bits;
    size_t bit_count;
} EdgeFilter;

/**
 * @brief Calcula una clave igual para ambos órdenes de una arista.
 * @param u Primer extremo.
 * @param v Segundo extremo.
 * @return Clave de 64 bits de la arista no dirigida.
 */
static uint64_t edge_key(int u, int v) {
    uint32_t a = (uint32_t)(u < v ? u : v) + 1;
    uint32_t b = (uint32_t)(u < v ? v : u) + 1;
    return ((uint64_t)a << 32) | b;
}

/**
 * @brief Reserva aproximadamente 16 bits por arista en un filtro temporal.
 * @param filter Filtro que se inicializa.
 * @param edges Cantidad máxima de aristas.
 * @return true si la reserva tuvo éxito.
 */
static bool init_edge_filter(EdgeFilter *filter, long long edges) {
    filter->bits = NULL;
    filter->bit_count = 0;
    if (edges == 0) {
        return true;
    }
    if ((unsigned long long)edges > SIZE_MAX / 16) {
        return false;
    }
    size_t needed = (size_t)edges * 16;
    size_t count = 8;
    while (count < needed) {
        if (count > SIZE_MAX / 2) {
            return false;
        }
        count *= 2;
    }
    filter->bits = calloc(count / 8, 1);
    if (filter->bits == NULL) {
        return false;
    }
    filter->bit_count = count;
    return true;
}

/**
 * @brief Busca una arista; consulta la lista solamente ante un posible duplicado.
 * @param filter Filtro temporal.
 * @param g Grafo que contiene las aristas insertadas.
 * @param u Primer extremo.
 * @param v Segundo extremo.
 * @return true si la arista ya existe en el grafo.
 */
static bool filtered_edge_exists(const EdgeFilter *filter, const Graph *g, int u, int v) {
    if (filter->bit_count == 0) {
        return false;
    }
    uint64_t hash = edge_key(u, v);
    hash = (hash ^ (hash >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
    hash ^= hash >> 31;
    size_t mask = filter->bit_count - 1;
    size_t first = (size_t)hash & mask;
    size_t second = (size_t)(hash >> 32) & mask;
    bool found_first = (filter->bits[first / 8] & (1u << (first % 8))) != 0;
    bool found_second = (filter->bits[second / 8] & (1u << (second % 8))) != 0;
    if (!found_first || !found_second) {
        return false;
    }
    return edge_exists(g, u, v);
}

/**
 * @brief Registra en el filtro una arista insertada.
 * @param filter Filtro temporal ya inicializado.
 * @param u Primer extremo.
 * @param v Segundo extremo.
 */
static void mark_edge(EdgeFilter *filter, int u, int v) {
    uint64_t hash = edge_key(u, v);
    hash = (hash ^ (hash >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
    hash ^= hash >> 31;
    size_t mask = filter->bit_count - 1;
    size_t first = (size_t)hash & mask;
    size_t second = (size_t)(hash >> 32) & mask;
    filter->bits[first / 8] |= (unsigned char)(1u << (first % 8));
    filter->bits[second / 8] |= (unsigned char)(1u << (second % 8));
}

/**
 * @brief Comprueba si existe una arista entre dos vértices.
 * @param g Grafo que se consulta.
 * @param u Primer extremo de la arista.
 * @param v Segundo extremo de la arista.
 * @return true si existe la arista; false si no existe o los argumentos son inválidos.
 */
bool edge_exists(const Graph *g, int u, int v) {
    if (g == NULL || u < 0 || v < 0 ||
        u >= g->num_vertices || v >= g->num_vertices) {
        return false;
    }

    AdjNode *p = g->adj_lists[u];
    while (p != NULL) {
        if (p->dest == v) {
            return true;
        }
        p = p->next;
    }
    return false;
}

/**
 * @brief Añade una arista no dirigida; rechaza lazos, duplicados y pesos inválidos.
 * @param g Grafo que se modifica.
 * @param u Primer extremo de la arista.
 * @param v Segundo extremo de la arista.
 * @param w Peso positivo y finito.
 * @return true si se insertó; false si falla, sin modificar el grafo.
 */
bool add_edge(Graph *g, int u, int v, double w) {
    if (g == NULL || u < 0 || v < 0 ||
        u >= g->num_vertices || v >= g->num_vertices) {
        return false;
    }
    if (u == v || !isfinite(w) || w <= 0 || edge_exists(g, u, v)) {
        return false;
    }

    if (g->node_pool != NULL) {
        if (g->num_edges >= g->pool_capacity) {
            return false;
        }
        append_pooled_edge(g, u, v, w);
        return true;
    }

    AdjNode *a = malloc(sizeof(AdjNode));
    AdjNode *b = malloc(sizeof(AdjNode));
    if (a == NULL || b == NULL) {
        free(a);
        free(b);
        return false;
    }

    /* Crear ambas direcciones antes de modificar las listas. */
    a->dest = v;
    a->weight = w;
    a->next = g->adj_lists[u];

    b->dest = u;
    b->weight = w;
    b->next = g->adj_lists[v];

    g->adj_lists[u] = a;
    g->adj_lists[v] = b;
    g->num_edges++;
    return true;
}

/**
 * @brief Genera un entero pseudoaleatorio mediante SplitMix64.
 * @param state Estado del generador; se actualiza en cada llamada.
 * @return Entero pseudoaleatorio de 64 bits sin signo.
 */
static uint64_t random64(uint64_t *state) {
    /* Estas operaciones conservan la misma secuencia para una semilla dada. */
    *state = *state + UINT64_C(0x9e3779b97f4a7c15);
    uint64_t z = *state;
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

/**
 * @brief Genera un entero uniforme en un intervalo mediante muestreo por rechazo.
 * @param s Estado del generador; se actualiza.
 * @param bound Límite superior exclusivo, estrictamente positivo.
 * @return Entero del intervalo [0, bound).
 */
static uint64_t bounded(uint64_t *s, uint64_t bound) {
    uint64_t threshold = (uint64_t)(-bound) % bound;
    uint64_t x = random64(s);

    /* Descartar los valores que introducirían sesgo al calcular el resto. */
    while (x < threshold) {
        x = random64(s);
    }
    return x % bound;
}

/**
 * @brief Genera un peso pseudoaleatorio en el intervalo (0, 1].
 * @param s Estado del generador; se actualiza.
 * @return Peso positivo no mayor que uno.
 */
static double weight(uint64_t *s) {
    uint64_t value = random64(s);
    value = value >> 11;
    value = value + 1;
    return (double)value / 9007199254740992.0;
}

/**
 * @brief Genera un grafo simple y conexo reproducible a partir de una semilla.
 * @param v Cantidad positiva de vértices.
 * @param e Cantidad de aristas, entre v - 1 y v * (v - 1) / 2.
 * @param seed Semilla del generador pseudoaleatorio.
 * @return Grafo generado, o NULL si los argumentos son inválidos o no hay memoria.
 */
Graph *generate_random_connected_graph_seeded(int v, long long e, uint64_t seed) {
    long long max = (long long)v * (v - 1LL) / 2;
    if (v <= 0 || e < v - 1LL || e > max) {
        return NULL;
    }
    Graph *g = create_graph(v);
    if (g == NULL) {
        return NULL;
    }
    EdgeFilter filter;
    if (!reserve_nodes(g, e) || !init_edge_filter(&filter, e)) {
        free_graph(g);
        return NULL;
    }

    /* El árbol inicial conecta todos los vértices. */
    for (int i = 1; i < v; i++) {
        int parent = (int)bounded(&seed, (uint64_t)i);
        double w = weight(&seed);
        append_pooled_edge(g, i, parent, w);
        mark_edge(&filter, i, parent);
    }

    if (e > max / 4) {
        /* En grafos densos, elegir pares sin hacer intentos repetidos. */
        uint64_t available = (uint64_t)(max - g->num_edges);
        uint64_t needed = (uint64_t)(e - g->num_edges);
        for (int u = 0; u < v && needed > 0; u++) {
            for (int dest = u + 1; dest < v && needed > 0; dest++) {
                if (filtered_edge_exists(&filter, g, u, dest)) {
                    continue;
                }
                if (bounded(&seed, available) < needed) {
                    append_pooled_edge(g, u, dest, weight(&seed));
                    mark_edge(&filter, u, dest);
                    needed--;
                }
                available--;
            }
        }
    } else {
        while (g->num_edges < e) {
            int u = (int)bounded(&seed, (uint64_t)v);
            int dest = (int)bounded(&seed, (uint64_t)v);
            if (u == dest) {
                continue;
            }
            if (filtered_edge_exists(&filter, g, u, dest)) {
                continue;
            }
            append_pooled_edge(g, u, dest, weight(&seed));
            mark_edge(&filter, u, dest);
        }
    }
    free(filter.bits);
    return g;
}

/**
 * @brief Genera un grafo simple y conexo utilizando la semilla fija 42.
 * @param v Cantidad positiva de vértices.
 * @param e Cantidad de aristas, entre v - 1 y v * (v - 1) / 2.
 * @return Grafo generado, o NULL en caso de error.
 */
Graph *generate_random_connected_graph(int v, long long e) {
    return generate_random_connected_graph_seeded(v, e, 42);
}

/**
 * @brief Libera las listas de adyacencia y la estructura del grafo.
 * @param g Grafo que se desea liberar; admite NULL.
 */
void free_graph(Graph *g) {
    if (g == NULL) {
        return;
    }

    if (g->node_pool == NULL) {
        for (int u = 0; u < g->num_vertices; u++) {
            AdjNode *p = g->adj_lists[u];
            while (p != NULL) {
                AdjNode *next = p->next;
                free(p);
                p = next;
            }
        }
    }
    free(g->node_pool);
    free(g->adj_lists);
    free(g);
}

/**
 * @brief Lee un token de hasta 127 caracteres separado por espacios en blanco.
 * @param f Archivo de entrada.
 * @param out Arreglo de 128 caracteres donde se almacena el token.
 * @return true si se leyó un token válido; false si termina la entrada o es demasiado largo.
 */
static bool token(FILE *f, char out[128]) {
    int c = fgetc(f);
    int n = 0;

    while (c != EOF && isspace((unsigned char)c)) {
        c = fgetc(f);
    }
    if (c == EOF) {
        return false;
    }

    while (c != EOF && !isspace((unsigned char)c)) {
        if (n == 127) {
            return false;
        }
        out[n] = (char)c;
        n++;
        c = fgetc(f);
    }
    out[n] = '\0';
    return true;
}

/**
 * @brief Lee un entero decimal y comprueba que no desborde long long.
 * @param f Archivo de entrada.
 * @param out Salida para el entero leído.
 * @return true si se leyó un entero válido; false en caso contrario.
 */
static bool integer(FILE *f, long long *out) {
    char s[128];
    char *end;
    if (!token(f, s)) {
        return false;
    }

    errno = 0;
    *out = strtoll(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return false;
    }
    return true;
}

/**
 * @brief Lee V E y luego E ternas u v peso; rechaza datos inválidos o sobrantes.
 * @param f Archivo de entrada; los identificadores comienzan en cero.
 * @return Grafo leído, o NULL si falla la lectura, la validación o la reserva de memoria.
 */
Graph *read_graph(FILE *f) {
    long long n;
    long long e;
    if (f == NULL) {
        return NULL;
    }
    if (!integer(f, &n) || n <= 0 || n > INT_MAX) {
        return NULL;
    }
    if (!integer(f, &e) || e < 0 || e > n * (n - 1) / 2) {
        return NULL;
    }

    Graph *g = create_graph((int)n);
    if (g == NULL) {
        return NULL;
    }

    EdgeFilter filter;
    if (!reserve_nodes(g, e) || !init_edge_filter(&filter, e)) {
        free_graph(g);
        return NULL;
    }

    for (long long i = 0; i < e; i++) {
        long long u;
        long long v;
        char s[128];
        char *end;

        if (!integer(f, &u) || !integer(f, &v)) {
            free(filter.bits);
            free_graph(g);
            return NULL;
        }
        if (u < 0 || v < 0 || u >= n || v >= n || !token(f, s)) {
            free(filter.bits);
            free_graph(g);
            return NULL;
        }

        errno = 0;
        double w = strtod(s, &end);
        if (errno != 0 || end == s || *end != '\0') {
            free(filter.bits);
            free_graph(g);
            return NULL;
        }
        if (u == v || !isfinite(w) || w <= 0) {
            free(filter.bits);
            free_graph(g);
            return NULL;
        }
        if (filtered_edge_exists(&filter, g, (int)u, (int)v)) {
            free(filter.bits);
            free_graph(g);
            return NULL;
        }
        append_pooled_edge(g, (int)u, (int)v, w);
        mark_edge(&filter, (int)u, (int)v);
    }

    free(filter.bits);

    /* Después de las aristas solo se permiten espacios en blanco. */
    int c = fgetc(f);
    while (c != EOF) {
        if (!isspace((unsigned char)c)) {
            free_graph(g);
            return NULL;
        }
        c = fgetc(f);
    }
    if (ferror(f)) {
        free_graph(g);
        return NULL;
    }
    return g;
}

/**
 * @brief Escribe el grafo en texto, incluyendo una sola copia de cada arista.
 * @param g Grafo que se desea guardar.
 * @param f Archivo de salida.
 * @return true si la escritura fue correcta; false en caso de error.
 */
bool write_graph(const Graph *g, FILE *f) {
    if (g == NULL || f == NULL) {
        return false;
    }
    if (fprintf(f, "%d %lld\n", g->num_vertices, g->num_edges) < 0) {
        return false;
    }

    for (int u = 0; u < g->num_vertices; u++) {
        AdjNode *p = g->adj_lists[u];
        while (p != NULL) {
            if (u < p->dest) {
                if (fprintf(f, "%d %d %.17g\n", u, p->dest, p->weight) < 0) {
                    return false;
                }
            }
            p = p->next;
        }
    }
    return ferror(f) == 0;
}
