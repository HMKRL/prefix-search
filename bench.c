#ifndef BENCH_H
#define BENCH_H

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "bench.h"

#define INFILE "cities.txt"

#define PREFIX_LEN 3
#define SEARCH_TIMES 10000

enum { WRDMAX = 256, STKMAX = 512, LMAX = 1024 };

static inline __attribute__((always_inline))
void get_cycles(unsigned *high, unsigned *low)
{
    asm volatile ("CPUID\n\t"
                  "RDTSC\n\t"
                  "mov %%edx, %0\n\t"
                  "movl %%eax, %1\n\t": "=r" (*high), "=r" (*low)::"%rax","%rbx","%rcx","%rdx"
                 );
}

static inline __attribute__((always_inline))
void get_cycles_end(unsigned *high, unsigned *low)
{
    asm volatile("RDTSCP\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 "CPUID\n\t": "=r" (*high), "=r" (*low)::"%rax","%rbx","%rcx","%rdx"
                );
}

static inline __attribute__((always_inline))
uint64_t diff_in_cycles(unsigned high1, unsigned low1,
                        unsigned high2, unsigned low2)
{
    uint64_t start,end;
    start = (((uint64_t) high1 << 32) | low1);
    end = (((uint64_t) high2 << 32) | low2);
    return end - start;
}


void prefix_search_bench(const tst_node *root, char *result_filename)
{
    char word[WRDMAX] = "";
    char *sgl[LMAX] = {NULL};
    char prefix[PREFIX_LEN + 1];
    int sidx = 0;
    FILE *infp = fopen(INFILE, "r"), *outfp = fopen(result_filename, "w");

    uint64_t timec;
    unsigned timec_high1, timec_low1, timec_high2, timec_low2;


    if (!(infp && outfp)) {
        fprintf(stderr, "%s\n", "Open file failed.");
        if (!infp) fclose(infp);
        if (!outfp) fclose(outfp);

        return;
    }

    for (int i = 0; i < SEARCH_TIMES; ++i) {
        fscanf(infp, "%3s", prefix);
        get_cycles(&timec_high1, &timec_low1);
        tst_search_prefix(root, word, sgl, &sidx, LMAX);
        get_cycles_end(&timec_high2, &timec_low2);
        timec = diff_in_cycles(timec_high1, timec_low1, timec_high2, timec_low2);

        fprintf(outfp, "%d %ld\n", i, timec);
    }

    fclose(infp);
    fclose(outfp);

    return;
}

#endif
