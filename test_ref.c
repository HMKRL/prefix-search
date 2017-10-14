#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <stdint.h>

#include "tst.h"
#include "bench.h"

/** constants insert, delete, max word(s) & stack nodes */
enum { INS, DEL, WRDMAX = 256, STKMAX = 512, LMAX = 1024 };
#define REF INS
#define CPY DEL

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

/* timing helper function */
static double tvgetf(void)
{
    struct timespec ts;
    double sec;

    clock_gettime(CLOCK_REALTIME, &ts);
    sec = ts.tv_nsec;
    sec /= 1e9;
    sec += ts.tv_sec;

    return sec;
}

/* simple trim '\n' from end of buffer filled by fgets */
static void rmcrlf(char *s)
{
    size_t len = strlen(s);
    if (len && s[len - 1] == '\n')
        s[--len] = 0;
}

#define IN_FILE "cities.txt"
#define OUT_FILE "insert_result_ref.txt"

int main(int argc, char **argv)
{
    char word[WRDMAX] = "";
    char *sgl[LMAX] = {NULL};
    tst_node *root = NULL, *res = NULL;
    int rtn = 0, idx = 0, sidx = 0;
    FILE *fp, *outfp;
    double t1, t2;

    uint64_t timec;
    unsigned timec_high1, timec_low1, timec_high2, timec_low2;

    const int pool_size = 10000000;
    int pool_used = 0;
    char *pHead = (char*) malloc(pool_size * sizeof(char));
    if (!pHead) {
        fprintf(stderr, "error: pool allocation failed.\n");
        return 1;
    }
    char *pCurr = pHead;

    fp = fopen(IN_FILE, "r");
    if (!fp) { /* prompt, open, validate file for reading */
        fprintf(stderr, "error: file open failed '%s'.\n", argv[1]);
        return 1;
    }

    outfp = fopen(OUT_FILE, "w");
    if (!outfp) { /* prompt, open, validate file for reading */
        fprintf(stderr, "error: result file open failed '%s'.\n", argv[1]);
        fclose(fp);
        return 1;
    }

    t1 = tvgetf();
    while ((rtn = fscanf(fp, "%s", pCurr)) != EOF) {
        get_cycles(&timec_high1, &timec_low1);
        char *p = pCurr;
        if (!tst_ins_del(&root, &p, INS, REF)) {
            fprintf(stderr, "error: memory exhausted, tst_insert.\n");
            fclose(fp);
            fclose(outfp);

            return 1;
        }
        idx++;
        int add_len = strlen(pCurr) + 1;
        pCurr += add_len;
        pool_used += add_len;
        if (pool_size - pool_used <= WRDMAX) { /* remaining pool size not enough for next word */
            fprintf(stderr, "error: pool size not enough.\n");
            fclose(fp);
            fclose(outfp);

            return 1;
        }
        get_cycles_end(&timec_high2, &timec_low2);
        timec = diff_in_cycles(timec_high1, timec_low1, timec_high2, timec_low2);
        fprintf(outfp, "%d %ld\n", idx, timec);
    }
    t2 = tvgetf();

    fclose(fp);
    fclose(outfp);
    printf("ternary_tree, loaded %d words in %.6f sec\n", idx, t2 - t1);

    /* process --bench flag */
    int bench_flag = 0;

    struct option long_opt[] = {
        {"bench", no_argument, &bench_flag, 1},
        {0, 0, 0, 0}
    };

    getopt_long(argc, argv, "", long_opt, NULL);

    if (bench_flag) {
        prefix_search_bench(root, "search_result_ref.txt");
        tst_free(root);
        free(pHead);

        return 0;
    }

    for (;;) {
        printf(
            "\nCommands:\n"
            " a  add word to the tree\n"
            " f  find word in tree\n"
            " s  search words matching prefix\n"
            " d  delete word from the tree\n"
            " q  quit, freeing all data\n\n"
            "choice: ");
        fgets(word, sizeof word, stdin);

        switch (*word) {
            char *p = NULL;
        case 'a':
            printf("enter word to add: ");
            if (!fgets(pCurr, WRDMAX, stdin)) {
                fprintf(stderr, "error: insufficient input.\n");
                break;
            }
            rmcrlf(pCurr);
            p = pCurr;
            t1 = tvgetf();
            res = tst_ins_del(&root, &p, INS, REF);
            t2 = tvgetf();
            if (res) {
                idx++;
                int add_len = strlen(pCurr) + 1;
                pCurr += add_len;
                pool_used += add_len;
                if (pool_size - pool_used <= WRDMAX) { /* pool memory not enough for next word */
                    fprintf(stderr, "error: pool size not enough.\n");
                    return 1;
                }

                printf("  %s - inserted in %.6f sec. (%d words in tree)\n",
                       (char *) res, t2 - t1, idx);
            } else
                printf("  %s - already exists in list.\n", (char *) res);
            break;
        case 'f':
            printf("find word in tree: ");
            if (!fgets(word, sizeof word, stdin)) {
                fprintf(stderr, "error: insufficient input.\n");
                break;
            }
            rmcrlf(word);
            t1 = tvgetf();
            res = tst_search(root, word);
            t2 = tvgetf();
            if (res)
                printf("  found %s in %.6f sec.\n", (char *) res, t2 - t1);
            else
                printf("  %s not found.\n", word);
            break;
        case 's':
            printf("find words matching prefix (at least 1 char): ");
            if (!fgets(word, sizeof word, stdin)) {
                fprintf(stderr, "error: insufficient input.\n");
                break;
            }
            rmcrlf(word);
            t1 = tvgetf();
            res = tst_search_prefix(root, word, sgl, &sidx, LMAX);
            t2 = tvgetf();
            if (res) {
                printf("  %s - searched prefix in %.6f sec\n\n", word, t2 - t1);
                for (int i = 0; i < sidx; i++)
                    printf("suggest[%d] : %s\n", i, sgl[i]);
            } else
                printf("  %s - not found\n", word);
            break;
        case 'd':
            printf("enter word to del: ");
            if (!fgets(word, sizeof word, stdin)) {
                fprintf(stderr, "error: insufficient input.\n");
                break;
            }
            rmcrlf(word);
            p = word;
            printf("  deleting %s\n", word);
            t1 = tvgetf();
            /* FIXME: remove reference to each string */
            res = tst_ins_del(&root, &p, DEL, REF);
            t2 = tvgetf();
            if (res)
                printf("  delete failed.\n");
            else {
                printf("  deleted %s in %.6f sec\n", word, t2 - t1);
                idx--;
            }
            break;
        case 'q':
            tst_free(root);
            free(pHead);
            return 0;
            break;
        default:
            fprintf(stderr, "error: invalid selection.\n");
            break;
        }
    }

    return 0;
}
