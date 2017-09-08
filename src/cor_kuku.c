#include <stdio.h>
#include "cor_kuku.h"
#include "xxhash.h"

#define COR_KUKU_SLOT_ALIGNMENT 64
#define COR_KUKU_DEFAULT_KEY_SIZE 8
#define COR_KUKU_MAX_MOVE_STEPS_COUNT 32

cor_kuku_t *
cor_kuku_new(int nels)
{
    cor_kuku_t *kuku = (cor_kuku_t *) malloc(sizeof(cor_kuku_t));
    if (!kuku) {
        return NULL;
    }
    memset(kuku, 0, sizeof(cor_kuku_t));
    kuku->pool = cor_pool_new(nels * (sizeof(cor_kuku_el_t) + COR_KUKU_DEFAULT_KEY_SIZE));
    if (!kuku->pool) {
        cor_kuku_delete(kuku);
        return NULL;
    }
    int slot_size = (sizeof(cor_kuku_slot_t) + COR_KUKU_SLOT_ALIGNMENT - 1) & ~(COR_KUKU_SLOT_ALIGNMENT - 1);
    kuku->nslots = (nels + nels / 8) / COR_KUKU_DEFAULT_NSLOTS;
    kuku->slots = (cor_kuku_slot_t *) cor_pool_calloc(kuku->pool, (kuku->nslots + 1) * slot_size);
    if (!kuku->slots) {
        cor_kuku_delete(kuku);
        return NULL;
    }
    kuku->slots = (cor_kuku_slot_t *) (((uintptr_t) kuku->slots + ((uintptr_t) COR_KUKU_SLOT_ALIGNMENT - 1)) & ~((uintptr_t) COR_KUKU_SLOT_ALIGNMENT - 1));
    return kuku;
}

void
cor_kuku_delete(cor_kuku_t *kuku)
{
    if (kuku) {
        if (kuku->pool) {
            cor_pool_delete(kuku->pool);
        }
        free(kuku);
    }
}

static inline cor_kuku_el_t *
cor_kuku_set_new_el(cor_kuku_t *kuku, const char *key, int key_size)
{
    cor_kuku_el_t *el = cor_pool_calloc(kuku->pool, sizeof(cor_kuku_el_t) + key_size);
    if (!el) {
        return NULL;
    }
    el->key_size = key_size;
    memcpy(el->key, key, key_size);
    return el;
}

static inline cor_kuku_el_t *
cor_kuku_set_search_for_el_in_slot(cor_kuku_t *kuku, cor_kuku_slot_t *slot, uint8_t tag, const char *key, int key_size)
{
    for (int i = 0; i < COR_KUKU_DEFAULT_NSLOTS; i++) {
        if (slot->tags[i]) {
            if (slot->tags[i] == tag) {
                cor_kuku_el_t *el = slot->els[i];
                if (el->key_size == key_size && strncmp(el->key, key, key_size) == 0) {
                    return el;
                }
            }
        } else {
            cor_kuku_el_t *el = cor_kuku_set_new_el(kuku, key, key_size);
            if (!el) {
                return NULL;
            }
            slot->tags[i] = tag;
            slot->els[i] = el;
            return el;
        }
    }
    return NULL;
}

static inline void
cor_kuku_make_slot_id_and_tag(cor_kuku_t *kuku, const char *key, int key_size, unsigned int *slot_id, uint8_t *tag)
{
    XXH64_hash_t key_hash = XXH64(key, key_size, 13);
    *slot_id = (key_hash & 0xffffffff) % kuku->nslots;
    unsigned int hash = (key_hash >> 32) & 0xffffffff;
    *tag = hash & 0xff;
    if (!*tag) {
        *tag = (hash >> 8) & 0xff;
        if (!*tag) {
            *tag = (hash >> 16) & 0xff;
            if (!*tag) {
                *tag = (hash >> 24) & 0xff;
                if (!*tag) {
                    *tag = 1;
                }
            }
        }
    }
}

static inline int
cor_kuku_set_move_el(cor_kuku_t *kuku, int slot_id1, uint8_t tag, cor_kuku_el_t *el, int step)
{
    if (step == COR_KUKU_MAX_MOVE_STEPS_COUNT) {
        /*  extend hash  */
        printf("extending not implemented yet\n");
        return -1;
    }
    unsigned int slot_id2 = (slot_id1 ^ (tag * 0x5bd1e995)) % kuku->nslots;
    cor_kuku_slot_t *slot2 = &kuku->slots[slot_id2];
    /*  search for empty place in slot2  */
    for (int i = 0; i < COR_KUKU_DEFAULT_NSLOTS; i++) {
        if (slot2->tags[i] == 0) {
            /*  big luck, founded empty place in slot  */
            slot2->tags[i] = tag;
            slot2->els[i] = el;
            slot2->reallocated |= 1U << i;
            return 0;
        }
    }
    /*  search place for realloc in slot 2  */
    int id = __builtin_ffs(~slot2->reallocated);
    if (id < COR_KUKU_DEFAULT_NSLOTS) {
        /*  found element which used first slot, move it to second, and replace with current element  */
        if (cor_kuku_set_move_el(kuku, slot_id2, slot2->tags[id], slot2->els[id], step + 1) == 0 && !slot2->els[id]) {
            slot2->tags[id] = tag;
            slot2->els[id] = el;
            slot2->reallocated |= 1U << id;
            return 0;
        }
    }
    /*  extend hash  */
    printf("extending not implemented yet\n");
    return -1;
}

int
cor_kuku_set(cor_kuku_t *kuku, const char *key, int key_size, void *value)
{
    unsigned int slot_id1;
    uint8_t tag;
    cor_kuku_make_slot_id_and_tag(kuku, key, key_size, &slot_id1, &tag);
    /*  search for place in slot 1  */
    cor_kuku_slot_t *slot1 = &kuku->slots[slot_id1];
    cor_kuku_el_t *el = cor_kuku_set_search_for_el_in_slot(kuku, slot1, tag, key, key_size);
    if (el) {
        el->value = value;
        return 0;
    }
    /*  search for place in slot 2  */
    unsigned int slot_id2 = (slot_id1 ^ (tag * 0x5bd1e995)) % kuku->nslots;
    cor_kuku_slot_t *slot2 = &kuku->slots[slot_id2];
    el = cor_kuku_set_search_for_el_in_slot(kuku, slot2, tag, key, key_size);
    if (el) {
        el->value = value;
        return 0;
    }
    /*  search place for realloc in slot 1  */
    int id = __builtin_ffs(~slot1->reallocated);
    if (id < COR_KUKU_DEFAULT_NSLOTS) {
        /*  found element which used first slot, move it to second, and replace with current element  */
        if (cor_kuku_set_move_el(kuku, slot_id1, slot1->tags[id], slot1->els[id], 1) == 0 && !slot1->els[id] /*  test for slot used with move chain  */) {
            cor_kuku_el_t *el = cor_kuku_set_new_el(kuku, key, key_size);
            if (!el) {
                return -1;
            }
            slot1->tags[id] = tag;
            slot1->els[id] = el;
            return 0;
        }
    } else {
        /*  search place for realloc in slot 2 */
        id = __builtin_ffs(~slot2->reallocated);
        if (id < COR_KUKU_DEFAULT_NSLOTS) {
            /*  found element which used first slot, move it to second, and replace with current element  */
            if (cor_kuku_set_move_el(kuku, slot_id2, slot2->tags[id], slot2->els[id], 1) == 0 && !slot2->els[id]) {
                cor_kuku_el_t *el = cor_kuku_set_new_el(kuku, key, key_size);
                if (!el) {
                    return -1;
                }
                slot2->tags[id] = tag;
                slot2->els[id] = el;
                return 0;
            }
        }
    }
    /*  extend hash  */
    printf("extending not implemented yet\n");
    return -1;
}

void *
cor_kuku_get(cor_kuku_t *kuku, const char *key, int key_size)
{
    unsigned int slot_id1;
    uint8_t tag;
    cor_kuku_make_slot_id_and_tag(kuku, key, key_size, &slot_id1, &tag);
    /*  search for place in slot 1  */
    cor_kuku_slot_t *slot1 = &kuku->slots[slot_id1];
    for (int i = 0; i < COR_KUKU_DEFAULT_NSLOTS; i++) {
        if (slot1->tags[i]) {
            if (slot1->tags[i] == tag) {
                cor_kuku_el_t *el = slot1->els[i];
                if (el->key_size == key_size && strncmp(el->key, key, key_size) == 0) {
                    return el->value;
                }
            }
        } else {
            break;
        }
    }
    /*  search for place in slot 2  */
    unsigned int slot_id2 = (slot_id1 ^ (tag * 0x5bd1e995)) % kuku->nslots;
    cor_kuku_slot_t *slot2 = &kuku->slots[slot_id2];
    for (int i = 0; i < COR_KUKU_DEFAULT_NSLOTS; i++) {
        if (slot2->tags[i]) {
            if (slot2->tags[i] == tag) {
                cor_kuku_el_t *el = slot2->els[i];
                if (el->key_size == key_size && strncmp(el->key, key, key_size) == 0) {
                    return el->value;
                }
            }
        } else {
            break;
        }
    }
    return NULL;
}

void
cor_kuku_del(cor_kuku_t *kuku, const char *key, int key_size)
{
    unsigned int slot_id1;
    uint8_t tag;
    cor_kuku_make_slot_id_and_tag(kuku, key, key_size, &slot_id1, &tag);
    /*  search for place in slot 1  */
    cor_kuku_slot_t *slot1 = &kuku->slots[slot_id1];
    for (int i = 0; i < COR_KUKU_DEFAULT_NSLOTS; i++) {
        if (slot1->tags[i]) {
            if (slot1->tags[i] == tag) {
                cor_kuku_el_t *el = slot1->els[i];
                if (el->key_size == key_size && strncmp(el->key, key, key_size) == 0) {
                    slot1->tags[i] = 0;
                    slot1->els[i] = NULL;
                    slot1->reallocated &= ~(1U << i);
                    return;
                }
            }
        } else {
            break;
        }
    }
    /*  search for place in slot 2  */
    unsigned int slot_id2 = (slot_id1 ^ (tag * 0x5bd1e995)) % kuku->nslots;
    cor_kuku_slot_t *slot2 = &kuku->slots[slot_id2];
    for (int i = 0; i < COR_KUKU_DEFAULT_NSLOTS; i++) {
        if (slot2->tags[i]) {
            if (slot2->tags[i] == tag) {
                cor_kuku_el_t *el = slot2->els[i];
                if (el->key_size == key_size && strncmp(el->key, key, key_size) == 0) {
                    slot2->tags[i] = 0;
                    slot2->els[i] = NULL;
                    slot2->reallocated &= ~(1U << i);
                    return;
                }
            }
        } else {
            break;
        }
    }
}

