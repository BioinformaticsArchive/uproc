#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* MATLAB headers */
#if 0
#include <matrix.h>
#include <mat.h>
#endif

#include "ecurve.h"

#define SEQ_COUNT_MULTIPLIER 10000
#define POW_MIN 5
#define POW_MAX 11
#define POW_DIFF (POW_MAX - POW_MIN)
#define LEN_MAX (1 << POW_MAX)
#define ALPHABET "ARNDCQEGHILKMFPSTWYV"

#define INTERP_MIN 20
#define INTERP_MAX 5000

#define OUT_PREFIX_DEFAULT "prot_thresh_e"

#define ELEMENTS_OF(x) (sizeof (x) / sizeof (x)[0])

void *
xrealloc(void *ptr, size_t sz)
{
    ptr = realloc(ptr, sz);
    if (!ptr) {
        fprintf(stderr, "memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}
#define xmalloc(sz) xrealloc(NULL, sz)


int
choice(const struct ec_matrix *p, size_t n)
{
    double sum, c;
    unsigned i;
    c = (double)rand() / RAND_MAX;
    for (sum = 0, i = 0; sum < c && i < n; ++i) {
        if (p) {
            sum += ec_matrix_get(p, 0, i);
        }
        else {
            sum += 1.0 / n;
        }
    }
    return i - 1;
}

void
randseq(char *buf, size_t len, const struct ec_alphabet *alpha,
        const struct ec_matrix *probs)
{
    size_t i;
    static bool rand_seeded = false;
    if (!rand_seeded) {
        srand(time(NULL));
        rand_seeded = true;
    }
    for (i = 0; i < len; i++) {
        ec_amino a = choice(probs, EC_ALPHABET_SIZE);
        buf[i] = ec_alphabet_amino_to_char(alpha, a);
    }
    buf[i] = '\0';
}

void
append(double **dest, size_t *dest_n, size_t *dest_sz, double *src, size_t n)
{
    double *d = *dest;
    if (!d) {
        *dest_sz = 1000000;
        d = xrealloc(d, *dest_sz * sizeof *d);
    }
    while (*dest_n + n > *dest_sz) {
        *dest_sz += 100000;
        d = xrealloc(d, *dest_sz * sizeof *d);
    }
    memcpy(d + *dest_n, src, n * sizeof *d);
    *dest = d;
    *dest_n += n;
}

int
double_cmp(const void *p1, const void *p2)
{
    double delta = *(const double *)p1 - *(const double*)p2;
    if (fabs(delta) < EC_EPSILON) {
        return 0;
    }
    /* we want to sort in descending order */
    return delta < 0.0 ? 1 : -1;
}


int
csinterp(const double *xa, const double *ya, int m, const double *x, double *y, int n)
{
    int i, low, high, mid;
    double h, b, a, u[m], ya2[m];

    ya2[0] = ya2[m-1] = u[0] = 0.0;

    for (i = 1; i < m - 1; i++) {
        a = (xa[i] - xa[i-1]) / (xa[i+1] - xa[i-1]);
        b = a * ya2[i-1] + 2.0;
        ya2[i] = (a - 1.0) / b;
        u[i] = (ya[i+1] - ya[i]) / (xa[i+1] - xa[i]) - (ya[i] - ya[i-1]) / (xa[i] - xa[i-1]);
        u[i] = (6.0 * u[i] / (xa[i+1] - xa[i-1]) - a * u[i-1]) / b;
    }

    for (i = m - 1; i > 0; i--) {
        ya2[i-1] *= ya2[i];
        ya2[i-1] += u[i-1];
    }

    low = 0;
    high = m - 1;

    for (i = 0; i < n; i++) {
        if (i && (xa[low] > x[i] || xa[high] < x[i])) {
            low = 0;
            high = m - 1;
        }

        while (high - low > 1) {
            mid = (high + low) / 2;
            if (xa[mid] > x[i]) {
                high = mid;
            }
            else {
                low = mid;
            }
        }

        h = xa[high] - xa[low];
        if (h == 0.0) {
            return EC_EINVAL;
        }

        a = (xa[high] - x[i]) / h;
        b = (x[i] - xa[low]) / h;
        y[i] = a * ya[low]
            + b * ya[high]
            + ((a*a*a - a) * ya2[low] + (b*b*b - b) * ya2[high])
            * (h*h) / 6.0;
    }
    return 0;
}


int
store_interpolated(double thresh[static POW_DIFF + 1],
        const char *prefix, size_t number)
{
    size_t i;
    double xa[POW_DIFF + 1],
           x[INTERP_MAX], y[INTERP_MAX];
    char filename[1024];

    struct ec_matrix thresh_interp = { .rows = 1, .cols = INTERP_MAX, .values = y };

    for (i = 0; i < ELEMENTS_OF(xa); i++) {
        xa[i] = i;
    }

    for (i = 0; i < ELEMENTS_OF(x); i++) {
        double xi = i;
        if (i < INTERP_MIN) {
            xi = INTERP_MIN;
        }
        x[i] = log2(xi) - POW_MIN;
    }

    csinterp(xa, thresh, POW_DIFF + 1, x, y, INTERP_MAX);
    sprintf(filename, "%.100s%zu", prefix, number);
    return ec_matrix_store_file(&thresh_interp, filename, EC_STORAGE_GZIP);
}


enum args
{
    FWD = 1,
    REV,
    SUBSTMAT,
    AA_PROBS,
    ARGC,
    OUT_PREFIX = ARGC,
};

int main(int argc, char **argv)
{
    int res;
    struct ec_alphabet alpha;
    struct ec_matrix alpha_probs;
    struct ec_substmat substmat[EC_SUFFIX_LEN];
    struct ec_ecurve fwd, rev;
    double thresh2[POW_DIFF + 1], thresh3[POW_DIFF + 1];

    char *outfile_prefix = OUT_PREFIX_DEFAULT;

    if (argc < ARGC) {
        fprintf(stderr, "usage: %s"
                " forward.ecurve"
                " reverse.ecurve"
                " substmat.matrix"
                " aa_prob.matrix"
                " [output_file_prefix]"
                "\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc > OUT_PREFIX) {
        outfile_prefix = argv[OUT_PREFIX];
    }

    res = ec_substmat_load_many(substmat, EC_SUFFIX_LEN, argv[SUBSTMAT]);
    if (res != EC_SUCCESS) {
        return EXIT_FAILURE;
    }

    res = ec_matrix_load_file(&alpha_probs, argv[AA_PROBS]);
    ec_alphabet_init(&alpha, ALPHABET);

    res = ec_mmap_map(&fwd, argv[FWD]);
    if (res != EC_SUCCESS) {
        return EXIT_FAILURE;
    }
    res = ec_mmap_map(&rev, argv[REV]);
    if (res != EC_SUCCESS) {
        ec_ecurve_destroy(&fwd);
        return EXIT_FAILURE;
    }

#pragma omp parallel private(res)
    {
        char seq[LEN_MAX + 1];

        double *pred_scores = NULL;
        ec_class *pred_classes = NULL;
        size_t pred_count;

        size_t all_pred_n, all_pred_sz;
        double *all_pred = NULL;

        unsigned power;
        unsigned long i, seq_count;
        size_t seq_len;

#pragma omp for
        for (power = POW_MIN; power <= POW_MAX; power++) {

            all_pred_n = 0;
            seq_len = 1 << power;
            seq_count = (1 << (POW_MAX - power)) * SEQ_COUNT_MULTIPLIER;
            seq[seq_len] = '\0';
            for (i = 0; i < seq_count; i++) {
                randseq(seq, seq_len, &alpha, &alpha_probs);
                res = ec_classify_protein_all(seq, substmat, &fwd, &rev, &pred_count,
                        &pred_classes, &pred_scores);
                if (res != EC_SUCCESS) {
                    fprintf(stderr, "error in ec_classify_protein\n");
                }
                append(&all_pred, &all_pred_n, &all_pred_sz, pred_scores, pred_count);
            }
            qsort(all_pred, all_pred_n, sizeof *all_pred, &double_cmp);

#define MIN(a, b) ((a) < (b) ? (a) : (b))
            thresh2[power - POW_MIN] = all_pred[MIN(seq_count / 100, all_pred_n - 1)];
            thresh3[power - POW_MIN] = all_pred[MIN(seq_count / 1000, all_pred_n - 1)];
        }
        free(all_pred);
    }

    store_interpolated(thresh2, outfile_prefix, 2);
    store_interpolated(thresh3, outfile_prefix, 3);

    return EXIT_SUCCESS;
}
