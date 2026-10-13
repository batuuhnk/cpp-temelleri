#!/usr/bin/env bash
# Bir .c ya da .cpp dosyasini STM32 (Cortex-M4) icin derleyip assembly ciktisini gosterir.
#   ./asm.sh 01-class/asm/gps_c.c
set -e
DOSYA="$1"
case "$DOSYA" in
    *.c)   CC=arm-none-eabi-gcc ;;
    *.cpp) CC=arm-none-eabi-g++ ;;
    *)     echo "Kullanim: $0 <dosya.c|dosya.cpp>"; exit 1 ;;
esac
KOMUT="$CC -O1 -fno-inline -mcpu=cortex-m4 -mthumb -c '$DOSYA' -o /tmp/asm.o && arm-none-eabi-objdump -d --no-show-raw-insn /tmp/asm.o | sed -n '/^[0-9a-f]* </,\$p'"
docker run --rm -v "$PWD:$PWD" -w "$PWD" px4io/px4-dev:v1.16.0-rc1-258-g0369abd556 \
    bash -c "$KOMUT" 2>&1 | grep -v docker-entrypoint
