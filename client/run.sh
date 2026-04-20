#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
BUILD_TYPE="${1:-Release}"

cmake  "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

echo "Сборка завершена."
echo "Команда запуска:"
echo "cd build && ./client -ip <ip> -port <port> -msisdn <msisdn> -imsi <imsi> - imei <imei> -position <position>"