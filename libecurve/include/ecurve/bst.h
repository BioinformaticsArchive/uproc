#ifndef EC_BST_H
#define EC_BST_H

/** \file ecurve/bst.h
 *
 * Simple binary search tree implementation
 */

#include <stdlib.h>
#include <stdint.h>
#include "ecurve/common.h"
#include "ecurve/word.h"

union ec_bst_key {
    uintmax_t uint;
    struct ec_word word;
};

enum ec_bst_type {
    EC_BST_UINT,
    EC_BST_WORD,
};

/** Binary search tree */
struct ec_bst {
    /** The root node (struct ec_bst_node is defined in the implementation) */
    struct ec_bst_node *root;

    /** Number of nodes */
    size_t size;

    /** Function to compare keys */
    int (*cmp)(union ec_bst_key, union ec_bst_key);
};


/** Initialize an empty binary search tree */
void ec_bst_init(struct ec_bst *t, enum ec_bst_type type);

/** Remove all nodes from tree; leaves an empty tree
 *
 * Takes a callback function pointer similar to ec_bst_remove()
 *
 * \param t         bst instance
 * \param callback  callback function or null pointer
 */
void ec_bst_clear(struct ec_bst *t, void (*callback)(void*));

/** Return non-zero if the tree is empty */
int ec_bst_isempty(struct ec_bst *t);

/** Obtain the number of nodes */
size_t ec_bst_size(const struct ec_bst *t);

/** Insert item
 *
 * \param t     bst instance
 * \param key   search key
 * \param data  pointer to be stored
 *
 * \retval #EC_SUCCESS  item was inserted
 * \retval #EC_FAILURE  `key` was already present or memory allocation failed
 */
int ec_bst_insert(struct ec_bst *t, union ec_bst_key key, void *data);
int ec_bst_insert_uint(struct ec_bst *t, uintmax_t key, void *data);
int ec_bst_insert_word(struct ec_bst *t, struct ec_word key, void *data);

/** Get item
 *
 * \param t     bst instance
 * \param key   search key
 *
 * \return stored pointer, or null pointer if key not found
 */
void *ec_bst_get(struct ec_bst *t, union ec_bst_key key);
void *ec_bst_get_uint(struct ec_bst *t, uintmax_t key);
void *ec_bst_get_word(struct ec_bst *t, struct ec_word key);

/** Remove item
 *
 * Takes a callback function pointer to which the stored pointer will be
 * passed (e.g. `&free`)
 *
 * \param t         bst instance
 * \param key       key of the item to remove
 * \param callback  callback function or null pointer
 *
 * \retval #EC_SUCCESS  an item was removed
 * \retval #EC_FAILURE  `key` not found in the tree
 */
int ec_bst_remove(struct ec_bst *t, union ec_bst_key key, void (*callback)(void*));
int ec_bst_remove_uint(struct ec_bst *t, uintmax_t key, void (*callback)(void*));
int ec_bst_remove_word(struct ec_bst *t, struct ec_word key, void (*callback)(void*));

/** In-order iteration
 *
 * Iterate over all nodes, passing key, value and a caller-provided `opaque`
 * pointer to the function pointed to by `callback`. If the callback function
 * does _not_ return #EC_SUCCESS, iteration is aborted and that value is
 * returned.
 *
 * \param t         bst instance
 * \param callback  callback function pointer
 * \param opaque    third argument to `callback`
 *
 * \return
 * #EC_SUCCESS if the iteration completed successfully, else whatever the
 * callback function returned.
 */
int ec_bst_walk(struct ec_bst *t, int (*callback)(union ec_bst_key, void*, void*),
                void *opaque);

#endif
