// a basic bare minimum example of Proof-of-Key on a 256 bit ECDSA public key.
// this is to serve as an example of how PoK is operating in the most basic terms.
// this will not generate valid VFC keys.
// clang -Ofast -fopenmp basicpok.c ecc.c base58.c -lm -o basicpok
// https://github.com/vfcash

#include <omp.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include "ecc.h"
#include "base58.h"

#define uint unsigned int
typedef unsigned __int128 uint128_t;

const uint128_t max = -1; // little trick to get the max range of this datatype
const uint128_t chance = max / 3033033; // the higher the divisor the higher the difficulty in finding a match

static inline uint isSubGenesisAddress(uint8_t *a)
{
    // split the public key uint256_t (32 bytes) back into two uint128_t (16 bytes each)
    // in ecc.c the key was originally split but the ecc_make_key() function returns
    // both __int128 as one single byte array so we need to resplit it.
    uint128_t b,c;
    uint8_t *ofs = a;
    memcpy(&b, ofs, sizeof(uint128_t));
    memcpy(&c, ofs + sizeof(uint128_t), sizeof(uint128_t));
    
    // check
    if(b+c < chance) // doesn't matter if the uint128_t overflows and wraps, it's still faster than two less than operators.
        return 1;

    return 0;
}

void mine()
{
    uint8_t priv[ECC_BYTES];
    uint8_t pub[ECC_BYTES+1];
    ecc_make_key(pub, priv);
    const uint r = isSubGenesisAddress(pub);
    if(r != 0)
    {
        char bpriv[256];
        memset(bpriv, 0, sizeof(bpriv));
        size_t len = 256;
        b58enc(bpriv, &len, priv, ECC_BYTES);
        printf("Private Key: %s\n", bpriv);
    }
}

int main(int argc, char *args[])
{
    // set highest nice
    errno = 0;
    if(nice(-20) < 0)
    {
        while(errno != 0)
        {
            errno = 0;
            if(nice(-20) < 0)
                printf("Attempting to set process to nice of -20 (run with sudo)...\n");
            sleep(1);
        }
    }

// run the miner !
#pragma omp parallel
    while(1)
    {
        int tid = omp_get_thread_num();
        int nthreads;
        if(tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads: %d\n", nthreads);
        }

        time_t nt = time(0);
        uint32_t c = 0;
        while(1)
        {
            // update every x seconds
            if(tid == 0 && time(0) > nt)
            {
                const uint hs = (c * nthreads) / 16;

                char title[256];
                sprintf(title, "H/s: %u / C: %d", hs, nthreads);
                printf("%s\n", title);

                c = 0;
                nt = time(0) + 16;
            }

            // mine for a key
            mine();
            c++;
        }
    }

    // done.
    return 0;
}
