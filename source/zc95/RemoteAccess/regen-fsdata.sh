#!/bin/sh

if [ ! -f makefsdata ]; then
    # Doing this outside cmake as we don't want it cross-compiled but for host
    echo Compiling makefsdata
    gcc -o makefsdata -I../ -I$PICO_SDK_PATH/lib/lwip/src/include -I$PICO_SDK_PATH/lib/lwip/contrib/ports/unix/port/include -I. -DMAKEFS_SUPPORT_DEFLATE=1 -DMAKEFS_SUPPORT_DEFLATE_ZLIB=1 $PICO_SDK_PATH/lib/lwip/src/apps/http/makefsdata/makefsdata.c -lz
fi

echo Regenerating fsdata.c
./makefsdata -e -defl:9 -svr:zc95
echo Done
