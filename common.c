#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <uproc.h>

#include "common.h"


uproc_io_stream *
open_read(const char *path)
{
    if (!strcmp(path, "-")) {
        return uproc_stdin;
    }
    return uproc_io_open("r", UPROC_IO_GZIP, "%s", path);
}

uproc_io_stream *
open_write(const char *path, enum uproc_io_type type)
{
    if (!strcmp(path, "-")) {
        return type == UPROC_IO_GZIP ? uproc_stdout_gz : uproc_stdout;
    }
    return uproc_io_open("w", type, "%s", path);
}

void
print_version(const char *progname)
{
    fprintf(stderr,
        "%s, version " PACKAGE_VERSION "\n"
        "Copyright 2014 Peter Meinicke, Robin Martinjak\n"
        "License GPLv3+: GNU GPL version 3 or later " /* no line break! */
        "<http://gnu.org/licenses/gpl.html>\n"
        "\n"
        "This is free software; you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Please send bug reports to " PACKAGE_BUGREPORT "\n",
        progname);
}


int
parse_int(const char *arg, int *x)
{
    char *end;
    int tmp = strtol(arg, &end, 10);
    if (!*arg || *end) {
        return -1;
    }
    *x = tmp;
    return 0;
}

int
parse_prot_thresh_level(const char *arg, int *x)
{
    int tmp;
    if (parse_int(arg, &tmp)) {
        return -1;
    }
    if (tmp != 0 && tmp != 2 && tmp != 3) {
        return -1;
    }
    *x = tmp;
    return 0;
}

int
parse_orf_thresh_level(const char *arg, int *x)
{
    int tmp;
    if (parse_int(arg, &tmp)) {
        return -1;
    }
    if (tmp != 0 && tmp != 1 && tmp != 2) {
        return -1;
    }
    *x = tmp;
    return 0;
}


void
errhandler_bail(enum uproc_error_code num, const char *msg, const char *loc)
{
    (void)num;
    (void)msg;
    (void)loc;
    uproc_perror("");
    exit(EXIT_FAILURE);
}


void
trim_header(char *s)
{
    char *start = s, *end;
    while (isspace(*start) || *start == ',') {
        start++;
    }
    end = strpbrk(start, ", \f\n\r\t\v");
    if (end) {
        *end = '\0';
    }
}


int
database_load(struct database *db, const char *path, int prot_thresh_level,
              enum uproc_ecurve_format format)
{
    db->fwd = db->rev = NULL;
    db->idmap = NULL;
    db->prot_thresh = NULL;

    switch (prot_thresh_level) {
        case 2:
        case 3:
            db->prot_thresh = uproc_matrix_load(
                UPROC_IO_GZIP, "%s/prot_thresh_e%d", path, prot_thresh_level);
            if (!db->prot_thresh) {
                goto error;
            }
            break;

        case 0:
            break;

        default:
            uproc_error_msg(
                UPROC_EINVAL, "protein threshold level must be 0, 2, or 3");
            goto error;
    }

    db->idmap = uproc_idmap_load(UPROC_IO_GZIP, "%s/idmap", path);
    if (!db->idmap) {
        goto error;
    }
    db->fwd = uproc_ecurve_load(format, UPROC_IO_GZIP, "%s/fwd.ecurve", path);
    if (!db->fwd) {
        goto error;
    }
    db->rev = uproc_ecurve_load(format, UPROC_IO_GZIP, "%s/rev.ecurve", path);
    if (!db->rev) {
        goto error;
    }

    return 0;
error:
    database_free(db);
    return -1;
}


void
database_free(struct database *db)
{
    uproc_ecurve_destroy(db->fwd);
    uproc_ecurve_destroy(db->rev);
    uproc_idmap_destroy(db->idmap);
    uproc_matrix_destroy(db->prot_thresh);
    *db = (struct database)DATABASE_INITIALIZER;
}


int
model_load(struct model *m, const char *path, int orf_thresh_level)
{
    m->substmat = NULL;
    m->codon_scores = m->orf_thresh = NULL;

    m->substmat = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat", path);
    if (!m->substmat) {
        goto error;
    }

    m->codon_scores = uproc_matrix_load(UPROC_IO_GZIP, "%s/codon_scores",
                                      path);
    if (!m->codon_scores) {
        goto error;
    }

    switch (orf_thresh_level) {
        case 1:
        case 2:
            m->orf_thresh = uproc_matrix_load(UPROC_IO_GZIP, "%s/orf_thresh_e%d",
                                            path, orf_thresh_level);
            if (!m->orf_thresh) {
                goto error;
            }
            break;

        case 0:
            break;

        default:
            uproc_error_msg(
                UPROC_EINVAL, "ORF threshold level must be 0, 1, or 2");
            goto error;
    }

    return 0;
error:
    model_free(m);
    return -1;
}


void
model_free(struct model *m)
{
    uproc_substmat_destroy(m->substmat);
    uproc_matrix_destroy(m->codon_scores);
    uproc_matrix_destroy(m->orf_thresh);
    *m = (struct model)MODEL_INITIALIZER;
}


static bool
prot_filter(const char *seq, size_t len, uproc_family family,
            double score, void *opaque)
{
    (void)seq;
    (void)family;
    unsigned long rows, cols;
    uproc_matrix *thresh = opaque;
    if (!thresh) {
        return score > UPROC_EPSILON;
    }
    uproc_matrix_dimensions(thresh, &rows, &cols);
    if (len >= rows) {
        len = rows - 1;
    }
    return score >= uproc_matrix_get(thresh, len, 0);
}


static bool
orf_filter(const struct uproc_orf *orf, const char *seq, size_t seq_len,
           double seq_gc, void *opaque)
{
    unsigned long r, c, rows, cols;
    uproc_matrix *thresh = opaque;
    (void) seq;
    if (orf->length < 20) {
        return false;
    }
    if (!thresh) {
        return true;
    }
    uproc_matrix_dimensions(thresh, &rows, &cols);
    r = seq_gc * 100;
    c = seq_len;
    if (r >= rows) {
        r = rows - 1;
    }
    if (c >= cols) {
        c = cols - 1;
    }
    return orf->score >= uproc_matrix_get(thresh, r, c);
}


int
create_classifiers(uproc_protclass **pc, uproc_dnaclass **dc,
                   const struct database *db, const struct model *model,
                   bool short_read_mode)
{
    enum uproc_protclass_mode pc_mode = UPROC_PROTCLASS_ALL;
    enum uproc_dnaclass_mode dc_mode = UPROC_DNACLASS_ALL;

    if (dc && short_read_mode) {
        pc_mode = UPROC_PROTCLASS_MAX;
        dc_mode = UPROC_DNACLASS_MAX;
    }
    *pc = uproc_protclass_create(pc_mode, db->fwd, db->rev, model->substmat,
                                 prot_filter, db->prot_thresh);
    if (!*pc) {
        return -1;
    }
    if (!dc) {
        return 0;
    }

    *dc = uproc_dnaclass_create(dc_mode, *pc, model->codon_scores, orf_filter,
                                model->orf_thresh);

    if (!*dc) {
        uproc_protclass_destroy(*pc);
        return -1;
    }
    return 0;
}


#if !defined(NDEBUG) && HAVE_CLOCK_GETTIME
void timeit_start(timeit *t)
{
    if (t->running) {
        return;
    }
    clock_gettime(CLOCK_REALTIME, &t->start);
    t->running = 1;
}

void timeit_stop(timeit *t)
{
    struct timespec stop;
    if (!t->running) {
        return;
    }
    clock_gettime(CLOCK_REALTIME, &stop);
    t->total += (stop.tv_sec - t->start.tv_sec) + (stop.tv_nsec - t->start.tv_nsec) / 1e9;
    t->running = 0;
}

void timeit_print(timeit *t, const char *s)
{
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fprintf(stderr, "%.5f\n", t->total);
}
#endif