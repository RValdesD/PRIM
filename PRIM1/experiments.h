#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H
#include <stdbool.h>
/**
 * @brief Ejecuta la batería completa de experimentos (series A, B, C y D) y escribe un CSV.
 * @param csv_path Ruta del CSV de salida (se sobrescribe).
 * @param seeds Repeticiones por configuración, cada una con un grafo distinto (el enunciado pide 10).
 * @param shift Se resta a todos los exponentes i y j; 0 reproduce el enunciado, valores de 1 a 5 sirven de prueba rápida.
 * @return true si todo terminó bien y ambos MST pesaron lo mismo en todas las ejecuciones.
 */
bool run_experiments(const char *csv_path, int seeds, int shift);
#endif
