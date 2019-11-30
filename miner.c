//VF CASH - Standalone Miner - August 2019
//James William Fletcher

#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "ecc.h"
#include "base58.h"

uint8_t rpriv[ECC_BYTES];
uint8_t rpub[ECC_BYTES+1];

float approx_sqrt(float n)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;
    x2 = n * 0.5F;
    y  = n;
    i  = * ( long * ) &y;
    i  = 0x5f3759df - ( i >> 1 );
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );
    y  = y * ( threehalfs - ( x2 * y * y ) );
    return n*y;
}

double toDB(const uint64_t b)
{
    return (double)(b) / 1000;
}

double mfloor(double i)
{
    if(i < 0)
        return (int)i - 1;
    else
        return (int)i;
}

struct vec3
{
    uint16_t x,y,z;
};
typedef struct vec3 vec3;

double gNa(const vec3* a, const vec3* b)
{
    const double dot = ((double)(a->x) * (double)(b->x)) + ((double)(a->y) * (double)(b->y)) + (double)((a->z) * (double)(b->z));
    const double m1 = sqrt((double)((a->x) * (double)(a->x)) + (double)((a->y) * (double)(a->y)) + (double)((a->z) * (double)(a->z)));
    const double m2 = sqrt((double)((b->x) * (double)(b->x)) + (double)((b->y) * (double)(b->y)) + (double)((b->z) * (double)(b->z)));

    if((m1 == 0 && m2 == 0) || dot == 0)
        return 1;

    return dot / (m1*m2);
}

uint64_t isSubGenesisAddress(uint8_t *a)
{
    vec3 v[5];

    uint8_t *ofs = a;
    memcpy(&v[0].x, ofs, sizeof(uint16_t));
    memcpy(&v[0].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[0].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[1].x, ofs, sizeof(uint16_t));
    memcpy(&v[1].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[1].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[2].x, ofs, sizeof(uint16_t));
    memcpy(&v[2].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[2].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[3].x, ofs, sizeof(uint16_t));
    memcpy(&v[3].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[3].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[4].x, ofs, sizeof(uint16_t));
    memcpy(&v[4].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[4].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    const double a1 = gNa(&v[0], &v[3]);
    const double a2 = gNa(&v[3], &v[2]);
    const double a3 = gNa(&v[2], &v[1]);
    const double a4 = gNa(&v[1], &v[4]);

    const double min = 0.180;
    
    if(a1 < min && a2 < min && a3 < min && a4 < min)
    {
        const double at = (a1+a2+a3+a4);
        if(at <= 0)
            return 0;
        const double ra = at/4;
        const double mn = 4.166666667;
        const uint64_t rv = (uint64_t)mfloor(( 1000 + ( 10000*(1-(ra*mn)) ) )+0.5);
        
        printf("\nsubG: %.8f - %.8f - %.8f - %.8f - %.3f VFC < %.3f\n", a1, a2, a3, a4, toDB(rv), ra);
        
        return rv;
    }
    
    const double soft = 0.1;
    if(a1 < min+soft && a2 < min+soft && a3 < min+soft && a4 < min+soft)
        printf("x: %.8f - %.8f - %.8f - %.8f\n", a1, a2, a3, a4);

    return 0;

}

double subDiff(uint8_t *a)
{
    vec3 v[5]; //Vectors

    uint8_t *ofs = a;
    memcpy(&v[0].x, ofs, sizeof(uint16_t));
    memcpy(&v[0].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[0].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[1].x, ofs, sizeof(uint16_t));
    memcpy(&v[1].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[1].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[2].x, ofs, sizeof(uint16_t));
    memcpy(&v[2].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[2].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[3].x, ofs, sizeof(uint16_t));
    memcpy(&v[3].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[3].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    ofs = ofs + (sizeof(uint16_t)*3);
    memcpy(&v[4].x, ofs, sizeof(uint16_t));
    memcpy(&v[4].y, ofs + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&v[4].z, ofs + (sizeof(uint16_t)*2), sizeof(uint16_t));

    const double a1 = gNa(&v[0], &v[3]);
    const double a2 = gNa(&v[3], &v[2]);
    const double a3 = gNa(&v[2], &v[1]);
    const double a4 = gNa(&v[1], &v[4]);

    //printf("%.3f - %.3f - %.3f - %.3f\n", a1,a2,a3,a4);
    double diff = a1;
    if(a2 > diff)
        diff = a2;
    if(a3 > diff)
        diff = a3;
    if(a4 > diff)
        diff = a4;
    return diff;
}


int main()
{
    printf("Please wait, minted keys are saved to minted.txt, difficulty 0.24 ...\n");

    //Save reward addr used today
    ecc_make_key(rpub, rpriv);
    FILE* f = fopen("reward.txt", "a");
    if(f != NULL)
    {
        char bpriv[256];
        memset(bpriv, 0, sizeof(bpriv));
        size_t len = sizeof(bpriv);
        b58enc(bpriv, &len, rpriv, ECC_CURVE);
        fprintf(f, "%s\n", bpriv);
        fclose(f);
    }
    
    #pragma omp parallel
    //#pragma omp target teams distribute parallel for
    //for(int i=0; i < 2048; ++i) 
    while(1)
    {
        //i=0;
        int tid = omp_get_thread_num();
        int nthreads;
        if(tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads: %d\n", nthreads);
        }
        
        time_t nt = time(0)+16;
        uint64_t c = 0;
        while(1)
        {
            if(time(0) > nt)
            {
                if(tid == 0)
                    printf("HASH/s: %lu\n", (c*nthreads)/16);

                if(c > 0)
                    printf("T-%i: %lu\n", omp_get_thread_num(), c/16);

                c = 0;
                nt = time(0)+16;
            }

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

                //Log in console
                printf("Private Key: %s (%.3f DIFF) (%.3f VFC)\n\n", bpriv, diff, fr);

                //Try to claim
                char cmd[2048];
                sprintf(cmd, "wget -qO- \"https://vfcash.uk/rest.php?fromprivfast=%s&frompub=%s&topub=%s&amount=%.3f\"", bpriv, bpub, brpub, fr);
                if(system(cmd) != -1)
                    printf("\n%s\n", cmd);

                //Log claim url
                FILE* f = fopen("trans.txt", "a");
                if(f != NULL)
                {
                    fprintf(f, "https://vfcash.uk/rest.php?fromprivfast=%s&frompub=%s&topub=%s&amount=%.3f\n", bpriv, bpub, brpub, fr);
                    fclose(f);
                }
                
                //Log in minted
                f = fopen("minted.txt", "a");
                if(f != NULL)
                {
                    fprintf(f, "%s / %.3f / %.3f\n", bpriv, diff, fr);
                    fclose(f);
                }
            }

            c++;
        }
    }
    
    return 0;
}
