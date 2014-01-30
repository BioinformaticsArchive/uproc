#ifndef UPROC_COMMON_H
#define UPROC_COMMON_H

/** \file uproc/common.h
 *
 * Common definitions
 */

#include <stdint.h>
#include <inttypes.h>
#include <math.h>

/** Epsilon value for comparing floating point numbers */
#define UPROC_EPSILON 1e-5

/** Lowest `n` bits set to one */
#define UPROC_BITMASK(n) (~(~0ULL << (n)))

/** Length of the suffix part of a word */
#define UPROC_PREFIX_LEN 6

/** Length of the prefix part of a word */
#define UPROC_SUFFIX_LEN 12

/** Total word length */
#define UPROC_WORD_LEN (UPROC_PREFIX_LEN + UPROC_SUFFIX_LEN)


/** Type to represent one amino acid. */
typedef int uproc_amino;

/** Bits needed to represent one amino acid */
#define UPROC_AMINO_BITS 5

/** Number of amino acids in the alphabet */
#define UPROC_ALPHABET_SIZE 20


/*----------*
 * PREFIXES *
 *----------*/

/** Type for prefixes
 *
 * Prefixes are (in contrast to suffixes, see below) contiguous, i.e. all values
 * from 0 to #UPROC_PREFIX_MAX represent valid prefixes.
 */
typedef uint_least32_t uproc_prefix;
/** printf() format for prefixes */
#define UPROC_PREFIX_PRI PRIu32
/** scanf() format for prefixes */
#define UPROC_PREFIX_SCN SCNu32

/** Raise `x` to the power of 6 */
#define POW6(x) ((x) * (x) * (x) * (x) * (x) * (x))

/** Maximum value of a prefix */
#define UPROC_PREFIX_MAX (POW6((unsigned long) UPROC_ALPHABET_SIZE) - 1)


/*----------*
 * SUFFIXES *
 *----------*/

/** Type for suffixes
 *
 * Suffixes are represented as a "bit string" of #UPROC_SUFFIX_LEN amino acids,
 * each represented #UPROC_AMINO_BITS bits.
 */
typedef uint_least64_t uproc_suffix;

/** printf() format for suffixes */
#define UPROC_SUFFIX_PRI PRIu64

/** scanf() format for suffixes */
#define UPROC_SUFFIX_SCN SCNu64


/** Identifier of a protein class (or "protein family"). */
typedef uint_least16_t uproc_family;

/** Maximum value for #uproc_family */
#define UPROC_FAMILY_MAX (UINT_LEAST16_MAX - 1)

#define UPROC_FAMILY_INVALID (UINT_LEAST16_MAX)

/** printf() format for #uproc_family */
#define UPROC_FAMILY_PRI PRIu16

/** scanf() format for #uproc_family */
#define UPROC_FAMILY_SCN SCNu16

#endif
