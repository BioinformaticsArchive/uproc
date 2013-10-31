#ifndef UPRO_ALPHABET_H
#define UPRO_ALPHABET_H

/** \file upro/alphabet.h
 *
 * Translate characters to/from amino acids
 */

#include "upro/common.h"
#include <limits.h>

/** Struct defining an amino acid alphabet */
struct upro_alphabet {
    /** Original alphabet string */
    char str[UPRO_ALPHABET_SIZE + 1];

    /** Lookup table for mapping characters to amino acids */
    upro_amino aminos[UCHAR_MAX + 1];
};

/** Initialize an alphabet object with the given string
 *
 * The string must be exactly `#UPRO_ALPHABET_SIZE` characters long and consist
 * only of uppercase letters. No letter shall be included twice.
 *
 */
int upro_alphabet_init(struct upro_alphabet *alpha, const char *s);

/** Translate character to amino acid
 *
 * \param alpha     alphabet to use
 * \param c         character to translate
 *
 * \return
 * Corresponding amino acid or -1 if `c` is a non-alphabetic character.
 */
upro_amino upro_alphabet_char_to_amino(const struct upro_alphabet *alpha, int c);

/** Translate amino acid to character
 *
 * \param alpha     alphabet to use
 * \param a         amino acid to translate
 *
 * \return
 * Corresponding character, or -1 if `a` does not represent a valid amino acid.
 */
int upro_alphabet_amino_to_char(const struct upro_alphabet *alpha, upro_amino a);

#endif