#!/usr/bin/env bash
# Bir .c ya da .cpp dosyasini derleyip calistirir.
# Bilgisayarda g++ yoksa PX4'un derleme container'indaki g++'i kullanir.
#   ./calistir.sh 01-class/cpp_versiyon.cpp
set -e
DOSYA="$1"
case "$DOSYA" in
    *.c)   DERLE="gcc -Wall -std=c11   -o /tmp/prog '$DOSYA' && /tmp/prog" ;;
    *.cpp) DERLE="g++ -Wall -std=c++17 -o /tmp/prog '$DOSYA' && /tmp/prog" ;;
    *)     echo "Kullanim: $0 <dosya.c|dosya.cpp>"; exit 1 ;;
esac
if command -v g++ >/dev/null; then
    bash -c "$DERLE"
else
    docker run --rm -v "$PWD:$PWD" -w "$PWD" px4io/px4-dev:v1.16.0-rc1-258-g0369abd556 \
        bash -c "$DERLE" 2>&1 | grep -v docker-entrypoint
fi
