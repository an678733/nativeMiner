apt install --assume-yes libsdl2-net-2.0-0
apt install --assume-yes libsdl2-net-dev
apt install --assume-yes libsdl2-2.0-0
apt install --assume-yes libsdl2-dev
apt install --assume-yes libomp-dev
apt install --assume-yes mingw-w64
apt install --assume-yes gcc
apt install --assume-yes clang

gcc -Ofast -fopenmp miner.c ecc.c base58.c -lm -o bin/gcc-miner-linux-64
clang -Ofast -fopenmp miner.c ecc.c base58.c -lm -o bin/clang-miner-linux-64
i686-w64-mingw32-gcc -Ofast -fopenmp miner.c ecc.c base58.c -static -static-libgcc -lm -o bin/miner-win-64.exe

gcc -Ofast -fopenmp gminer.c sha3.c ecc.c base58.c -lSDL2 -lm -o bin/gcc-gui-miner-linux-64
clang -Ofast -fopenmp gminer.c sha3.c ecc.c base58.c -lSDL2 -lm -o bin/clang-gui-miner-linux-64
i686-w64-mingw32-gcc -Ofast -fopenmp gminer.c sha3.c ecc.c base58.c -static -static-libgcc -lSDL2 -lm -o bin/gui-miner-win-64.exe


