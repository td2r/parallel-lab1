#!/bin/bash

set -euxo pipefail

cd "$(dirname "$0")"

/opt/opencilk/bin/clang++ -fopencilk -O3 main.cpp -o main

CILK_NWORKERS=4 ./main "$@"

