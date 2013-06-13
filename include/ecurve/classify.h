#ifndef EC_CLASSIFY_H
#define EC_CLASSIFY_H

/** \file ecurve/classify.h
 *
 * Classify amino acid sequences
 */

#include "ecurve/common.h"
#include "ecurve/distmat.h"
#include "ecurve/ecurve.h"


/** Classify amino acid sequence
 *
 * The following algorithm is used:
 *
 *   - For all words in the sequence `seq`:
 *
 *         -# Look up forward input word in forward ecurve.
 *         -# Align suffix of input and found word(s) using position-sentive
 *            alignment using the distance matrices in `distmat`.
 *         -# Insert alignment distances into the score vector associated with the
 *            found protein class.
 *         -# Repeat for reverse word and ecurve.
 *
 *   -  Sum up score vectors of all protein classes and find the class with
 *      maximum score.
 *
 *  The score vector of a protein class (virtually) holds the highest alignment
 *  score obtained at a given position.
 *
 * _One_ of `fwd_ecurve` and `rev_ecurve` may be a null pointer to "ignore"
 * that direction.
 * If both are non-null, they should use the same alphabet.
 *
 * \param seq           amino acid sequence
 * \param distmat       distance matrices for aligning the suffixes
 * \param fwd_ecurve    ecurve for looking up forward words
 * \param rev_ecurve    ecurve for looking up reverse words
 * \param predict_cls   _OUT_: protein class with highest score
 * \param predict_score _OUT_: score
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int
ec_classify(const char *seq, const ec_distmat distmat[static EC_SUFFIX_LEN],
            const ec_ecurve *fwd_ecurve, const ec_ecurve *rev_ecurve,
            ec_class *predict_cls, ec_dist *predict_score);

#endif