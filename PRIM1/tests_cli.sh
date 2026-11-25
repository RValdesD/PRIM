#!/bin/sh
set -eu
# Seleccionar el ejecutable recibido o utilizar el nombre predeterminado.
if [ "$#" -gt 0 ]; then
    bin=$1
else
    bin=./prim_mst
fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
# Comprobar que la misma semilla produce el mismo archivo.
"$bin" generate 20 45 42 "$tmp/a.txt"
"$bin" generate 20 45 42 "$tmp/b.txt"
cmp "$tmp/a.txt" "$tmp/b.txt"
"$bin" run "$tmp/a.txt" both 3 --edges > "$tmp/result"
"$bin" run "$tmp/a.txt" binomial > /dev/null
"$bin" run "$tmp/a.txt" fibonacci --edges > /dev/null
"$bin" generate 1 0 0 "$tmp/single.txt"
"$bin" run "$tmp/single.txt" both > /dev/null
# @brief Comprueba que una entrada inválida termine con código 1.
# @param Argumentos que se pasan al ejecutable bajo prueba.
reject() {
    status=0
    if "$bin" "$@" > "$tmp/rejected.out" 2> "$tmp/rejected.err"; then
        status=0
    else
        status=$?
    fi
    if [ "$status" -ne 1 ]; then
        echo "FALLO: se esperaba código 1, se obtuvo $status para $*" >&2
        cat "$tmp/rejected.err" >&2
        exit 1
    fi
}
reject generate 3 4 42 "$tmp/invalid.txt"
reject generate -3 2 42 "$tmp/invalid.txt"
reject generate 3 2 -1 "$tmp/invalid.txt"
reject run "$tmp/a.txt" both 20
reject run "$tmp/a.txt" unknown
reject run "$tmp/missing.txt" both
printf '3 1\n0 1 1\n' > "$tmp/disconnected.txt"
reject run "$tmp/disconnected.txt" both
printf '3 2\n0 1 1\n1 0 1\n' > "$tmp/duplicate.txt"
reject run "$tmp/duplicate.txt" both
echo 'PASS CLI generation, parsing, root, errors and both heaps'
