// optimised vfc miner
// clang -Ofast -fopenmp vfcsuperminer.c sha3.c ecc.c base58.c -lm -o vfcsuperminer
#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include "ecc.h"
#include "base58.h"
#include "sha3.h"

#define uint unsigned int

const float min = 0.180f; // difficulty
uint8_t rpriv[ECC_BYTES];
uint8_t rpub[ECC_BYTES+1];

struct vec3
{
    uint16_t x, y, z;
};
typedef struct vec3 vec3;

static inline double toDB(const uint64_t b)
{
    return (double)(b) / 1000;
}

static inline float gNa(const vec3 *a, const vec3 *b)
{
    const float dot = ((float)(a->x) * (float)(b->x)) + ((float)(a->y) * (float)(b->y)) + (float)((a->z) * (float)(b->z));
    const float m1 = sqrtf((float)((a->x) * (float)(a->x)) + (float)((a->y) * (float)(a->y)) + (float)((a->z) * (float)(a->z)));
    const float m2 = sqrtf((float)((b->x) * (float)(b->x)) + (float)((b->y) * (float)(b->y)) + (float)((b->z) * (float)(b->z)));

    if((m1 == 0.f && m2 == 0.f) || dot == 0.f)
        return 1.f;

    return dot / (m1 * m2);
}

static inline uint64_t isSubGenesisAddress(uint8_t *a)
{
    vec3 v[5];

    uint8_t *ofs = a;
    memcpy(&v[0].x, ofs, sizeof(uint16_t));
    memcpy(&v[0].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[0].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[1].x, ofs, sizeof(uint16_t));
    memcpy(&v[1].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[1].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[2].x, ofs, sizeof(uint16_t));
    memcpy(&v[2].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[2].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[3].x, ofs, sizeof(uint16_t));
    memcpy(&v[3].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[3].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[4].x, ofs, sizeof(uint16_t));
    memcpy(&v[4].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[4].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    const float a1 = gNa(&v[0], &v[3]);
    const float a2 = gNa(&v[3], &v[2]);
    const float a3 = gNa(&v[2], &v[1]);
    const float a4 = gNa(&v[1], &v[4]);

    if(a1 < min && a2 < min && a3 < min && a4 < min)
    {
        const float at = (a1+a2+a3+a4);
        const float ra = at*0.25f;
        const float mn = 4.166666667f;
        const uint64_t rv = (uint64_t)floorf(( 1000.f + ( 10000.f*(1.f-(ra*mn)) ) )+0.5f);
        printf("\nsubG: %.8f - %.8f - %.8f - %.8f - %.3f VFC < %.3f\n", a1, a2, a3, a4, toDB(rv), ra);
        return rv;
    }

    return 0;
}

double subDiff(uint8_t *a)
{
    vec3 v[5];

    uint8_t *ofs = a;
    memcpy(&v[0].x, ofs, sizeof(uint16_t));
    memcpy(&v[0].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[0].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[1].x, ofs, sizeof(uint16_t));
    memcpy(&v[1].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[1].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[2].x, ofs, sizeof(uint16_t));
    memcpy(&v[2].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[2].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[3].x, ofs, sizeof(uint16_t));
    memcpy(&v[3].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[3].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t) * 3);
    memcpy(&v[4].x, ofs, sizeof(uint16_t));
    memcpy(&v[4].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[4].z, ofs + (sizeof(uint16_t) * 2), sizeof(uint16_t));

    const double a1 = gNa(&v[0], &v[3]);
    const double a2 = gNa(&v[3], &v[2]);
    const double a3 = gNa(&v[2], &v[1]);
    const double a4 = gNa(&v[1], &v[4]);

    double diff = a1;
    if(a2 > diff)
        diff = a2;
    if(a3 > diff)
        diff = a3;
    if(a4 > diff)
        diff = a4;
    return diff;
}

void mine()
{
    uint8_t priv[ECC_BYTES];
    uint8_t pub[ECC_BYTES+1];
    ecc_make_key(pub, priv);
    uint64_t r = isSubGenesisAddress(pub);
    if(r != 0)
    {
        char bpriv[256];
        memset(bpriv, 0, sizeof(bpriv));
        size_t len = 256;
        b58enc(bpriv, &len, priv, ECC_BYTES);

        char bpub[256];
        memset(bpub, 0, sizeof(bpub));
        len = 256;
        b58enc(bpub, &len, pub, ECC_BYTES+1);

        char brpriv[256];
        memset(brpriv, 0, sizeof(brpriv));
        len = 256;
        b58enc(brpriv, &len, rpriv, ECC_BYTES);

        char brpub[256];
        memset(brpub, 0, sizeof(brpub));
        len = 256;
        b58enc(brpub, &len, rpub, ECC_BYTES+1);

        const double diff = subDiff(pub);
        const double fr = toDB(r);

        FILE *f = fopen("trans.txt", "a");
        if (f != NULL)
        {
            fprintf(f, "https://vfcash.uk/rest.php?fromprivfast=%s&frompub=%s&topub=%s&amount=%.3f\n", bpriv, bpub, brpub, fr);
            fclose(f);
        }

        printf("Private Key: %s (%.3f DIFF) (%.3f VFC)\n\n", bpriv, diff, fr);

        f = fopen("minted.txt", "a");
        if(f != NULL)
        {
            fprintf(f, "%s / %.3f / %.3f\n", bpriv, diff, fr);
            fclose(f);
        }
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

    // save reward addr used for this session
    ecc_make_key(rpub, rpriv);
    FILE *f = fopen("reward.txt", "a");
    if(f != NULL)
    {
        char bpriv[256];
        memset(bpriv, 0, sizeof(bpriv));
        size_t len = sizeof(bpriv);
        b58enc(bpriv, &len, rpriv, ECC_CURVE);
        fprintf(f, "%s\n", bpriv);
        fclose(f);
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
                const uint hs = (c * nthreads) / 333;

                char title[256];
                sprintf(title, "H/s: %u / C: %d", hs, nthreads);
                printf("%s\n", title);

                c = 0;
                nt = time(0) + 333;
            }

            // mine for a key
            mine();
            c++;
        }
    }

    // done.
    return 0;
}
