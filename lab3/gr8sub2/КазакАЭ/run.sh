#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

make all

echo "== Race: unsync =="
./bin/thread_race 4 1000000 unsync || true

echo "== Race: mutex =="
./bin/thread_race 4 1000000 mutex

echo "== Race: atomic =="
./bin/thread_race 4 1000000 atomic

echo "== ProdCons =="
./bin/prodcons -P 2 -C 2 -N 100000 -B 64
