#include <omp.h>
#include <SDL2/SDL.h>
// #include <SDL2/SDL_net.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "ecc.h"
#include "base58.h"
#include "sha3.h"

SDL_Window *window;
const int width = 320;
const int height = 120;
uint minted = 0;
uint8_t rpriv[ECC_BYTES];
uint8_t rpub[ECC_BYTES + 1];
int autoclaim = 0;

double toDB(const uint64_t b)
{
    return (double)(b) / 1000;
}
uint32_t fromDB(const double b)
{
    return (uint32_t)(b * 1000);
}

double mfloor(double i)
{
    if (i < 0)
        return (int)i - 1;
    else
        return (int)i;
}

struct vec3
{
    uint16_t x, y, z;
};
typedef struct vec3 vec3;

double gNa(const vec3 *a, const vec3 *b)
{
    const double dot = ((double)(a->x) * (double)(b->x)) + ((double)(a->y) * (double)(b->y)) + (double)((a->z) * (double)(b->z));
    const double m1 = sqrt((double)((a->x) * (double)(a->x)) + (double)((a->y) * (double)(a->y)) + (double)((a->z) * (double)(a->z)));
    const double m2 = sqrt((double)((b->x) * (double)(b->x)) + (double)((b->y) * (double)(b->y)) + (double)((b->z) * (double)(b->z)));

    if ((m1 == 0 && m2 == 0) || dot == 0)
        return 1;

    return dot / (m1 * m2);
}

uint64_t isSubGenesisAddress(uint8_t *a)
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

    const double min = 0.180;

    if (a1 < min && a2 < min && a3 < min && a4 < min)
    {
        const double at = (a1 + a2 + a3 + a4);
        if (at <= 0)
            return 0;
        const double ra = at / 4;
        const double mn = 4.166666667;
        const uint64_t rv = (uint64_t)mfloor((1000 + (10000 * (1 - (ra * mn)))) + 0.5);

        printf("\nsubG: %.8f - %.8f - %.8f - %.8f - %.3f VFC < %.3f\n", a1, a2, a3, a4, toDB(rv), ra);

        return rv;
    }

    const double soft = 0.1;
    if (a1 < min + soft && a2 < min + soft && a3 < min + soft && a4 < min + soft)
        printf("x: %.8f - %.8f - %.8f - %.8f\n", a1, a2, a3, a4);

    return 0;
}

double subDiff(uint8_t *a)
{
    vec3 v[5]; //Vectors

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

    //printf("%.3f - %.3f - %.3f - %.3f\n", a1,a2,a3,a4);
    double diff = a1;
    if (a2 > diff)
        diff = a2;
    if (a3 > diff)
        diff = a3;
    if (a4 > diff)
        diff = a4;
    return diff;
}

void setpixel(SDL_Surface *surface, const int x, const int y, const Uint8 r, const Uint8 g, const Uint8 b)
{
    if (x < 0 || x > width || y < 0 || y > height)
        return;
    const Uint32 color = SDL_MapRGB(surface->format, r, g, b);
    Uint8 *pixel = (Uint8 *)surface->pixels;
    pixel += (y * surface->pitch) + (x * sizeof(Uint32));
    *((Uint32 *)pixel) = color;
}

void line(SDL_Surface *surface, int x0, int y0, int x1, int y1, const Uint8 r, const Uint8 g, const Uint8 b)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    while (1)
    {
        setpixel(surface, x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1)
        {
            break;
        }
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}

int ool = 0;
int lv = 0;
int lv1 = 0;
void render(SDL_Surface *surface, const uint ihs)
{
    const uint hs = ihs / 700;

    if (lv != 0)
    {
        line(surface, ool - 3, lv, ool, hs, 255, 191, 0);
        line(surface, ool - 3, height - 1 - lv1, ool, height - 1 - (minted * 3), 220, 107, 229);
    }

    ool += 3;
    lv = hs;
    lv1 = minted * 3;
    if (ool > width)
    {
        ool = 0;
        SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
    }
}

// uint64_t rand64()
// {
//     return rand() ^ ((uint64_t)rand() << 15) ^ ((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 45) ^ ((uint64_t)rand() << 60);
// }

// struct addr
// {
//     uint8_t key[ECC_CURVE + 1];
// };
// typedef struct addr addr;

// struct sig
// {
//     uint8_t key[ECC_CURVE * 2];
// };
// typedef struct sig sig;

// struct trans
// {
//     uint64_t uid;
//     addr from;
//     addr to;
//     uint32_t amount;
//     sig owner;
// };

// void sendTransaction(const uint8_t *from_priv, const uint8_t *to_pub, const uint32_t amount)
// {
//     //Open Socket
//     UDPsocket sd;
//     if (!(sd = SDLNet_UDP_Open(0)))
//     {
//         printf("Could not create socket\n");
//         return;
//     }

//     //Resolve vfcash.uk
//     IPaddress srvHost;
//     IPaddress *ip = &srvHost;
//     SDLNet_ResolveHost(ip, "vfcash.uk", 8787);

//     //Generate Packet
//     const size_t packet_len = 1 + sizeof(uint32_t) + sizeof(uint64_t) + ECC_CURVE + 1 + ECC_CURVE + 1 + sizeof(uint32_t) + ECC_CURVE + ECC_CURVE;
//     UDPpacket *p = SDLNet_AllocPacket(packet_len);
//     if (!p)
//     {
//         printf("Could not allocate packet\n");
//         return;
//     }

//     //Gen Public Key
//     uint8_t from_pub[ECC_BYTES + 1];
//     ecc_get_pubkey(from_pub, from_priv);

//     //Transaction
//     struct trans t;
//     memset(&t, 0, sizeof(struct trans));
//     //
//     memcpy(t.from.key, from_pub, ECC_CURVE + 1);
//     memcpy(t.to.key, to_pub, ECC_CURVE + 1);
//     t.amount = amount;

//     //Sign the block
//     uint8_t thash[ECC_CURVE];
//     sha3_context c;
//     sha3_Init256(&c);
//     sha3_Update(&c, &t, sizeof(struct trans));
//     sha3_Finalize(&c);
//     memcpy(thash, &c.sb, ECC_CURVE);
//     if (ecdsa_sign(from_priv, thash, t.owner.key) == 0)
//     {
//         printf("\nSorry you're client failed to sign the Transaction.\n\n");
//         exit(0);
//     }

//     const uint origin = 0;
//     const uint64_t uid = rand64();
//     uint8_t *pc = p->data;
//     pc[0] = 't';
//     uint8_t *ofs = pc + 1;
//     memcpy(ofs, &origin, sizeof(uint32_t));
//     ofs += sizeof(uint32_t);
//     memcpy(ofs, &t.uid, sizeof(uint64_t));
//     ofs += sizeof(uint64_t);
//     memcpy(ofs, t.from.key, ECC_CURVE + 1);
//     ofs += ECC_CURVE + 1;
//     memcpy(ofs, t.to.key, ECC_CURVE + 1);
//     ofs += ECC_CURVE + 1;
//     memcpy(ofs, &t.amount, sizeof(uint32_t));
//     ofs += sizeof(uint32_t);
//     memcpy(ofs, t.owner.key, ECC_CURVE * 2);

//     //Send Packet
//     p->address.host = srvHost.host;
//     p->address.port = srvHost.port;
//     SDLNet_UDP_Send(sd, -1, p);
// }

void mine()
{
    uint8_t priv[ECC_BYTES];
    uint8_t pub[ECC_BYTES + 1];
    ecc_make_key(pub, priv);
    uint64_t r = isSubGenesisAddress(pub);
    if (r != 0)
    {
        char bpriv[256];
        memset(bpriv, 0, sizeof(bpriv));
        size_t len = 256;
        b58enc(bpriv, &len, priv, ECC_BYTES);

        char bpub[256];
        memset(bpub, 0, sizeof(bpub));
        len = 256;
        b58enc(bpub, &len, pub, ECC_BYTES + 1);

        char brpriv[256];
        memset(brpriv, 0, sizeof(brpriv));
        len = 256;
        b58enc(brpriv, &len, rpriv, ECC_BYTES);

        char brpub[256];
        memset(brpub, 0, sizeof(brpub));
        len = 256;
        b58enc(brpub, &len, rpub, ECC_BYTES + 1);

        const double diff = subDiff(pub);
        const double fr = toDB(r);

        //Try to claim
        if (autoclaim == 1)
        {
            //sendTransaction(priv, rpub, r);
            char cmd[2048];
            sprintf(cmd, "wget -qO- \"https://vfcash.uk/rest.php?fromprivfast=%s&frompub=%s&topub=%s&amount=%.3f\"", bpriv, bpub, brpub, fr);
            if (system(cmd) != -1)
                printf("\n%s\n", cmd);
        }

        FILE *f = fopen("trans.txt", "a");
        if (f != NULL)
        {
            fprintf(f, "https://vfcash.uk/rest.php?fromprivfast=%s&frompub=%s&topub=%s&amount=%.3f\n", bpriv, bpub, brpub, fr);
            fclose(f);
        }

        //Log
        printf("Private Key: %s (%.3f DIFF) (%.3f VFC)\n\n", bpriv, diff, fr);
        minted++;

        //Save to file
        f = fopen("minted.txt", "a");
        if (f != NULL)
        {
            fprintf(f, "%s / %.3f / %.3f\n", bpriv, diff, fr);
            fclose(f);
        }
    }
}

int main(int argc, char *args[])
{
    // enable auto claim?
    if (argc == 2 && strcmp(args[1], "autoclaim") == 0)
    {
        autoclaim = 1;
        printf("Auto claim will attempt to claim all minted.txt to the last key that has been appended to rewards.txt.\n");
    }

    //Init SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "ERROR: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    //Create window
    window = SDL_CreateWindow("vfcminergui", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        fprintf(stderr, "ERROR: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_Surface *screenSurface = SDL_GetWindowSurface(window);

    //Save reward addr used today
    ecc_make_key(rpub, rpriv);
    FILE *f = fopen("reward.txt", "a");
    if (f != NULL)
    {
        char bpriv[256];
        memset(bpriv, 0, sizeof(bpriv));
        size_t len = sizeof(bpriv);
        b58enc(bpriv, &len, rpriv, ECC_CURVE);
        fprintf(f, "%s\n", bpriv);
        fclose(f);
    }

//Run the miner !
#pragma omp parallel
    while (1)
    {
        int tid = omp_get_thread_num();
        int nthreads;
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads: %d\n", nthreads);
        }

        time_t nt = time(0);
        uint32_t c = 0;
        while (1)
        {
            if (tid == 0) //UI Thread
            {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                    case SDL_QUIT:
                    {
                        exit(0);
                    }
                    break;
                    }
                }
            }

            //Update every x seconds
            if (tid == 0 && time(0) > nt)
            {
                const uint hs = (c * nthreads) / 3;
                render(screenSurface, hs);
                SDL_UpdateWindowSurface(window);

                char title[256];
                sprintf(title, "H/s: %u / C: %d", hs, nthreads);
                SDL_SetWindowTitle(window, title);

                c = 0;
                minted = 0;
                nt = time(0) + 3;
            }

            //Mine for a key
            mine();
            c++;
        }
    }

    //Done.
    SDL_DestroyWindow(window);
    //SDLNet_Quit();
    SDL_Quit();
    return 0;
}
