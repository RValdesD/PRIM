#!/usr/bin/env bash
# Ejecuta toda la bateria del enunciado (series A-D, 10 grafos por configuracion) sin modificar codigo.
# Uso: ./run_experiments.sh [resultados.csv]     Variables opcionales: SEEDS (default 10), SHIFT (default 0; prueba rapida: SHIFT=8)
set -euo pipefail
cd "$(dirname "$0")"
OUT=${1:-resultados.csv}

make prim_mst

# Setup experimental para el informe.
{
    echo "# Fecha: $(date -Is)"
    echo "# Kernel / SO"; uname -a; cat /etc/os-release 2>/dev/null | head -2 || true
    echo "# CPU y caches"; lscpu 2>/dev/null || true
    echo "# Memoria"; free -h 2>/dev/null || true
    echo "# Compilador"; gcc --version | head -1
    echo "# Comando de compilacion"; make -n -B prim_mst | head -1
} > setup_experimental.txt

./prim_mst experiments "$OUT" "${SEEDS:-10}" "${SHIFT:-0}"
echo "Resultados en $OUT; setup en setup_experimental.txt"
