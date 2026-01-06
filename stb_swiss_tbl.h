#ifndef ST_INCLUDE_SWISSTABLE_H
#define ST_INCLUDE_SWISSTABLE_H

#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct st_str {
    char* data;
    size_t len;
} st_str;

#define GROUP_SIZE 16
typedef struct hash_entry {
    st_str key;
    int64_t value;
	size_t hash;
} hash_entry;

typedef struct swiss_tbl {
    uint8_t* ctrl;
    hash_entry* entries;
    size_t capacity;
    size_t size;
} swiss_tbl;

swiss_tbl* st_new(void);
swiss_tbl* st_insert(swiss_tbl* self, st_str* key, int64_t value);
bool st_find(swiss_tbl* self, st_str* key, int64_t* out);
swiss_tbl* st_merge(swiss_tbl* dest, swiss_tbl* src);
void st_free(swiss_tbl* self);

#ifdef __cplusplus
}
#endif

#endif // ST_INCLUDE_SWISSTABLE_H

#ifdef ST_IMPLEMENTATION
#include <stdlib.h>
#include <immintrin.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#ifndef ST_HASH_FUNC
#define ST_HASH_FUNC(key, keysize) stbds_hash_bytes(key, keysize, 0x2d358dccaa6c78a5ull)
#endif

#ifndef ST_PROBE_FUNC
#define ST_PROBE_FUNC st_linear_probe
#endif

#ifndef ST_MALLOC
#define ST_MALLOC(sz) malloc(sz)
#endif

#ifndef ST_FREE
#define ST_FREE(p) free(p)
#endif

#ifndef ST_MEMCMP
#include <string.h>
#define ST_MEMCMP(a,b,sz) memcmp(a,b,sz)
#endif

enum Ctrl: uint8_t {
	ST_EMPTY    = 0b10000000, // -128 in two's complement
	ST_DELETED  = 0b11111110, // -2 in two's complement
	ST_SENTINAL = 0b11111111 // -1  in two's complement
};


#define hash_str(key) ST_HASH_FUNC(key->data, key->len);

size_t hm_h1(size_t hash) {
    return hash >> 7;
}
uint8_t hm_h2(size_t hash) {
    return (uint8_t)(hash & 0x7F);
}
    
swiss_tbl* st_new() {
    return NULL;
}

size_t st_linear_probe(size_t group, size_t num_groups) {
    return (group + 1) % num_groups;
}

int st_match_byte(uint8_t* ctrl_group, uint8_t h2_val) {
    __m128i group = _mm_loadu_si128((const __m128i_u *) ctrl_group);
    __m128i target = _mm_set1_epi8(h2_val);
    __m128i cmp = _mm_cmpeq_epi8(group, target);
    return _mm_movemask_epi8(cmp);
}

swiss_tbl* st_resize(swiss_tbl* self) {
	if (self == NULL) {
		self = ST_MALLOC(sizeof(swiss_tbl));
		self->capacity = 0;
		self->size = 0;
		self->entries = NULL;
		self->ctrl = NULL;
	}

    size_t old_capacity = self->capacity;
    uint8_t* old_ctrl = self->ctrl;
    hash_entry* old_entries = self->entries;
    
    size_t new_capacity = self->capacity == 0  ? GROUP_SIZE : old_capacity * 2;
    uint8_t* ctrl = NULL;
    hash_entry* entries = NULL;
    for (int i = 0; i < new_capacity; ++i) {
        arrpush(ctrl, ST_EMPTY);
        arrpush(entries, ((hash_entry) { .key = {0}, .value = 0 }));
    }
    for (int i = 0; i < GROUP_SIZE; ++i) {
        arrpush(ctrl, ST_EMPTY);
    }
    
    self->ctrl = ctrl;
    self->entries = entries;
    self->capacity = new_capacity;
    self->size = 0;
    
	size_t num_of_groups = self->capacity / GROUP_SIZE;
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_ctrl[i] != ST_EMPTY && old_ctrl[i] != ST_DELETED) {
            hash_entry entry = old_entries[i];
            
            size_t hash = entry.hash;
            uint8_t h2_val = hm_h2(hash);
            size_t group = (hm_h1(hash) % self->capacity) / GROUP_SIZE;
            
            while (true) {
                size_t group_start = group * GROUP_SIZE;
                
                int empties = st_match_byte(&self->ctrl[group_start], ST_EMPTY);
                if (empties != 0) {
                    int offset = __builtin_ctz(empties);
                    int slot = group_start + offset;
                    
                    self->ctrl[slot] = h2_val;
                    self->entries[slot] = entry;
                    self->size++;
                    break;
                }
                group = ST_PROBE_FUNC(group, num_of_groups);
            }
        }
    }
    
    arrfree(old_ctrl);
    arrfree(old_entries);
	return self;
}


bool st_should_resize(swiss_tbl* self) {
	if (self == NULL) return true;
    return self->size * 8 >= self->capacity * 7;
}


swiss_tbl* st_insert(swiss_tbl* self, st_str* key, int64_t value) {
    if (st_should_resize(self)) {
        self = st_resize(self);
    }
    
    size_t num_of_groups = self->capacity / GROUP_SIZE;
    size_t hash = hash_str(key);
    uint8_t h2_val = hm_h2(hash);
    size_t group = (hm_h1(hash) % self->capacity) / GROUP_SIZE;
    
    while (true) {
        size_t group_start = group * GROUP_SIZE;
        
        int matches = st_match_byte(&self->ctrl[group_start], h2_val);
        
        while (matches != 0) {
            int offset = __builtin_ctz(matches);
            int slot = group_start + offset;
            hash_entry* prospect = &self->entries[slot];
            
            if (prospect->key.len == key->len && 
                ST_MEMCMP(key->data, prospect->key.data, key->len) == 0) {
                prospect->value = value;
                return self;
            }
            
            matches = matches & (matches - 1);
        }
        
        int empties = st_match_byte(&self->ctrl[group_start], ST_EMPTY);
        if (empties != 0) {
            int offset = __builtin_ctz(empties);
            int slot = group_start + offset;
            
            self->ctrl[slot] = h2_val;
            self->entries[slot] = (hash_entry) { .key = *key, .value = value, .hash = hash };
            self->size++;
            return self;
        }

        group = ST_PROBE_FUNC(group, num_of_groups);
    }
    
    __builtin_unreachable();
}

swiss_tbl* st_merge(swiss_tbl* dest, swiss_tbl* src) {
    if (dest == NULL || src == NULL) {
        return dest;
    }
    
    for (size_t i = 0; i < src->capacity; i++) {
        if (src->ctrl[i] != ST_EMPTY && src->ctrl[i] != ST_DELETED) {
            hash_entry* entry = &src->entries[i];
            dest = st_insert(dest, &entry->key, entry->value);
        }
    }
	return dest;
}

    
bool st_find(swiss_tbl* self, st_str* key, int64_t* out) {
    if (self == NULL) return false;

    size_t hash = hash_str(key);
    uint8_t h2_val = hm_h2(hash);
    size_t num_of_groups = self->capacity / GROUP_SIZE;
    size_t group = (hm_h1(hash) % self->capacity) / GROUP_SIZE;
    
    while (true) {
        size_t group_start = group * GROUP_SIZE;
        
        int matches = st_match_byte(&self->ctrl[group_start], h2_val);
        while (matches != 0) {
            int offset = __builtin_ctz(matches);
            hash_entry* prospect = &self->entries[group_start + offset];
            
            if (prospect->key.len == key->len &&
                ST_MEMCMP(key->data, prospect->key.data, key->len) == 0) {
                *out = prospect->value;
                return true;
            }
            
            matches = matches & (matches - 1);
        }
        
        if (st_match_byte(&self->ctrl[group_start], ST_EMPTY) != 0) {
            return false;
        }
        group = ST_PROBE_FUNC(group, num_of_groups);
    }

    __builtin_unreachable();
}

void st_free(swiss_tbl* self) {
	if (self == NULL) return;
	arrfree(self->ctrl);
	arrfree(self->entries);
	ST_FREE(self);
}
#endif