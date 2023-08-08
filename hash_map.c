// SPDX-License-Identifier: Apache-2.0

#ifndef HASHMAP
#error "this file needs HASHMAP defined"
#endif

#define HM1(a, b, c, d, e, f) a
#define HM2(a, b, c, d, e, f) b
#define HM3(a, b, c, d, e, f) c
#define HM4(a, b, c, d, e, f) d
#define HM5(a, b, c, d, e, f) e
#define HM6(a, b, c, d, e, f) f
#define HCAT1(a, b) a ## b
#define HCAT2(a, b) HCAT1(a, b)
#define HCAT3(a, b) HCAT2(a, b)
#define HASHNAME(a) HM1 a
#define HASHKEY(a) HM2 a
#define HASHVALUE(a) HM3 a
#define HASHPREF_(a) HM4 a
#define HASHPREF HASHPREF_(HASHMAP)
#define HASHFN(a) HM5 a
#define HASHKEYEQ(a) HM6 a
#define kv_pair HCAT3(HASHPREF, kv_pair)
#define hash_map_t HASHNAME(HASHMAP)
#define hash_map_iter HCAT3(HASHPREF, iter)
#define hkey_t HASHKEY(HASHMAP)
#define value_t HASHVALUE(HASHMAP)
#define probe_t HCAT3(HASHPREF, probe_t)
#define group_mask_t HCAT3(HASHPREF, group_mask_t)
#define hashfn HASHFN(HASHMAP)
#define hashkeyeq HASHKEYEQ(HASHMAP)
#define kEmpty HCAT3(HASHPREF, kEmpty)
#define kDeleted HCAT3(HASHPREF, kDeleted)
#define kSentinel HCAT3(HASHPREF, kSentinel)
#define kWidth HCAT3(HASHPREF, kWidth)
#define hm_new HCAT3(HASHPREF, new)
#define hm_cap_to_growth HCAT3(HASHPREF, cap_to_growth)
#define hm_new_with_length HCAT3(HASHPREF, new_with_length)
#define hm_new_with_cap HCAT3(HASHPREF, new_with_cap)
#define probe_new HCAT3(HASHPREF, probe_new)
#define probe_next HCAT3(HASHPREF, probe_next)
#define mask_get_lsb HCAT3(HASHPREF, mask_get_lsb)
#define mask_get_msb HCAT3(HASHPREF, mask_get_msb)
#define loadu_64le HCAT3(HASHPREF, loadu_64le)
#define store_64le HCAT3(HASHPREF, store_64le)
#define group_match HCAT3(HASHPREF, group_match)
#define group_match_empty HCAT3(HASHPREF, group_match_empty)
#define group_match_empty_or_deleted HCAT3(HASHPREF, group_match_empty_or_deleted)
#define hm_convert_deleted_to_empty_and_full_to_deleted HCAT3(HASHPREF, convert_deleted_to_empty_and_full_to_deleted)
#define group_count_empty_or_deleted HCAT3(HASHPREF, group_count_empty_or_deleted)
#define hm_set_ctrl HCAT3(HASHPREF, set_ctrl)
#define hm_find_offset_with_hash HCAT3(HASHPREF, find_offset_with_hash)
#define hm_find_with_hash HCAT3(HASHPREF, find_with_hash)
#define hm_find HCAT3(HASHPREF, find)
#define hm_find_first_non_full HCAT3(HASHPREF, find_first_non_full)
#define hm_remove_by_ptr HCAT3(HASHPREF, remove_by_ptr)
#define hm_remove HCAT3(HASHPREF, remove)
#define hm_resize HCAT3(HASHPREF, resize)
#define hm_is_in_same_group HCAT3(HASHPREF, is_in_same_group)
#define hm_drop_deletes_without_resize HCAT3(HASHPREF, drop_deletes_without_resize)
#define hm_rehash_and_grow_if_necessary HCAT3(HASHPREF, rehash_and_grow_if_necessary)
#define hm_prepare_insert HCAT3(HASHPREF, prepare_insert)
#define hm_find_or_insert_with_hash HCAT3(HASHPREF, find_or_insert_with_hash)
#define hm_find_or_insert HCAT3(HASHPREF, find_or_insert)
#define hm_assign HCAT3(HASHPREF, assign)
#define hm_free HCAT3(HASHPREF, free)
#define hm_iter_skip_empty_or_deleted HCAT3(HASHPREF, iter_skip_empty_or_deleted)
#define hm_iter_new HCAT3(HASHPREF, iter_new)
#define hm_iter_next HCAT3(HASHPREF, iter_next)

#include <stdalign.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	hkey_t k;
	value_t v;
} kv_pair;


// hashtable stores data in one buffer of key-value pairs and one sequence of
// control bytes which describe the state of respective slot (that is ctrl[i] describes slots[i]).
// control byte values in range [0..127] inclusive indicate that the respective slot
// contains an element and first seven bits of a hash of a key of that elemant are
// equal to the control byte value. other possible control byte values are 
// 0x80 which means an empty slot, 0xFE which means deleted slot and 0xFF which is sentinel byte
// that has no slot associated with it.
// "size" field is a number of elements in a hashtable. "capacity" is an amount of elements
// slots buffer is allocated for, "capacity" is always equal to some power of 2 minus 1 (0,1,3,7,15,...)
// growth_left indicates an amount of empty slots that can be filled until hashtable has to be resized
typedef struct {
	uint8_t *ctrl;
	kv_pair *slots;
	size_t size, capacity, growth_left;
} hash_map_t;

typedef struct {
	kv_pair *p;
	uint8_t *ictrl;
} hash_map_iter;

enum {
	kEmpty = 0x80,
	kDeleted = 0xFE,
	kSentinel = 0xFF,
#ifdef __SSE2__
	kWidth = 16,
#else
	kWidth = 8,
#endif
};

alignas(kWidth) static uint8_t empty_group[] = {
	kSentinel, kEmpty, kEmpty, kEmpty,
	kEmpty,    kEmpty, kEmpty, kEmpty
#ifdef __SSE2__
	,kEmpty,   kEmpty, kEmpty, kEmpty,
	kEmpty,    kEmpty, kEmpty, kEmpty
#endif
};

hash_map_t hm_new(void) {
	return (hash_map_t){empty_group, 0, 0, 0, 0};
}

size_t hm_cap_to_growth(size_t cap) {
	if (kWidth == 8 && cap == 7) {
		return 6;
	}
	return cap - cap/8;
}

static hash_map_t hm_new_with_length(size_t sz) {
	hash_map_t h = {0};
	size_t ctrl_space = sz + kWidth + alignof(kv_pair) - 1 & ~alignof(kv_pair) + 1;
	size_t slot_space = sizeof(kv_pair) * sz;
	h.ctrl = malloc(ctrl_space + slot_space);
	h.slots = (kv_pair*)(h.ctrl + ctrl_space);
	h.capacity = sz;
	h.growth_left = hm_cap_to_growth(sz);
	
	memset(h.ctrl, kEmpty, sz + kWidth);
	h.ctrl[sz] = kSentinel;
	
	return h;
}

// cap in the name actualy means how many elements can be inserted. that is growth
hash_map_t hm_new_with_cap(size_t sz) {
	if (kWidth == 8 && sz == 7) {
		sz = 8;
	} else {
		sz += ((int64_t)sz - 1) / 7;
	}
	if (sz == 0) {
		return hm_new();
	}
	sz = ~(size_t)0 >> __builtin_clzll(sz);
	return hm_new_with_length(sz);
}

typedef struct {
	size_t index, offset;
} probe_t;

static probe_t probe_new(size_t cap, size_t hash) {
	return (probe_t){0, hash >> 7 & cap};
}

static void probe_next(probe_t *p, size_t mask) {
	p->index += kWidth;
	p->offset += p->index;
	p->offset &= mask;
}

#ifdef __SSE2__
#include <emmintrin.h>
#ifdef __SSSE3__
#   include <tmmintrin.h>
#endif
typedef uint32_t group_mask_t;

static size_t mask_get_lsb(group_mask_t m) {
	return __builtin_ctz(m);
}

static size_t mask_get_msb(group_mask_t m) {
	return __builtin_clz((uint16_t)m)-16;
}

static uint32_t group_match(uint8_t *c, uint8_t n) {
	__m128i na = _mm_set1_epi8(n);
	__m128i ctrl = _mm_loadu_si128((__m128i*)c);
	return _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl, na));
}

static uint32_t group_match_empty(uint8_t *c) {
	__m128i ctrl = _mm_loadu_si128((__m128i*)c);
#   ifdef __SSSE3__
	return _mm_movemask_epi8(_mm_sign_epi8(ctrl, ctrl));
#   else
	__m128i na = _mm_set1_epi8((int8_t)kEmpty);
	return _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl, na));
#   endif
}

static uint32_t group_match_empty_or_deleted(uint8_t *c) {
	__m128i sents = _mm_set1_epi8((int8_t)kSentinel);
	__m128i ctrl = _mm_loadu_si128((__m128i*)c);
	return _mm_movemask_epi8(_mm_cmpgt_epi8(sents, ctrl));
}

static void hm_convert_deleted_to_empty_and_full_to_deleted(hash_map_t *h) {
	hash_map_t hc = *h;
	uint8_t *end = hc.ctrl + hc.capacity;
	for (uint8_t *c = hc.ctrl; c < end; c += kWidth) {
		__m128i ctrl = _mm_load_si128((__m128i*)c);
		__m128i msbs = _mm_set1_epi8(-128);
		__m128i x126 = _mm_set1_epi8(126);
#           ifdef __SSSE3__
		__m128i res = _mm_or_si128(_mm_shuffle_epi8(x126, ctrl), msbs);
#           else
		__m128i zero = _mm_setzero_si128();
		__m128i special_mask = _mm_cmpgt_epi8(zero, ctrl);
		__m128i part = _mm_andnot_si128(special_mask, x126);
		__m128i res = _mm_or_si128(msbs, part);
#           endif
		_mm_store_si128((__m128i*)c, res);
	}
	memcpy(end + 1, hc.ctrl, kWidth-1);
	hc.ctrl[hc.capacity] = kSentinel;
}

// rewriting this to use _tzcnt_u16 to get rid of + 1 does not improve much bc
// its harder to set1 kSentinel-1 (0xfe) than kSentinel (0xff)
static uint32_t group_count_empty_or_deleted(uint8_t *c) {
	__m128i ctrl = _mm_loadu_si128((__m128i*)c);
	__m128i s = _mm_set1_epi8((int8_t)kSentinel);
	uint32_t mask = _mm_movemask_epi8(_mm_cmpgt_epi8(s, ctrl));
	return __builtin_ctz(mask + 1);
}

#elif defined(__ARM_NEON) && !defined(__CUDA_ARCH__) && defined(__BYTE_ORDER__)\
   && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#include <arm_neon.h>

#define HASHMAP_NEON_OPTS 1

typedef uint64_t group_mask_t;

static size_t mask_get_lsb(group_mask_t m) {
	return __builtin_ctzll(m) / 8;
}

static size_t mask_get_msb(group_mask_t m) {
	return __builtin_clzll(m) / 8;
}

static uint64_t group_match(uint8_t *c, uint8_t n) {
	uint64_t msbs = 0x8080808080808080ULL;
	uint8x8_t dup = vdup_n_u8(n);
	uint8x8_t cm = vceq_u8(dup, vld1_u8(c));
	return vget_lane_u64(vreinterpret_u64_u8(cm), 0);
}

static uint64_t group_match_empty(uint8_t *c) {
	uint8x8_t x = vld1_u8(c);
	uint8x8_t na = vdup_n_u8(kEmpty);
	uint8x8_t cx = vceq_u8(x, na);
	return vget_lane_u64(vreinterpret_u64_u8(cx), 0);
}

static uint64_t group_match_empty_or_deleted(uint8_t *c) {
	int8x8_t se = vdup_n_s8((int8_t)kSentinel);
	int8x8_t ctrl = vld1_s8((int8_t*)c);
	uint8x8_t cx = vcgt_s8(se, ctrl);
	return vget_lane_u64(vreinterpret_u64_u8(cx), 0);
}

static void hm_convert_deleted_to_empty_and_full_to_deleted(hash_map_t *h) {
	uint64_t msbs = 0x8080808080808080ULL;
	uint64_t lsbs = 0x0101010101010101ULL;
	hash_map_t hc = *h;
	uint8_t *c = hc.ctrl;
	for (;c < hc.ctrl + hc.capacity; c += kWidth) {
		uint64_t x = *(uint64_t*)c & msbs;
		*(uint64_t*)c = (~x + (x >> 7)) & ~lsbs;
	}
	memcpy(hc.ctrl + hc.capacity + 1, hc.ctrl, kWidth-1);
	hc.ctrl[hc.capacity] = kSentinel;
}

static uint32_t group_count_empty_or_deleted(uint8_t *c) {
	uint64_t lsbs = 0x0101010101010101ULL;
	uint64_t ctrl = *(uint64_t*)c;
	uint64_t mask = (ctrl | ~(ctrl >> 7)) & lsbs;
	return mask == 0 ? 8 : mask_get_lsb(mask);
}

#else /* no NEON and no SSE2 */
typedef uint64_t group_mask_t;

static size_t mask_get_lsb(group_mask_t m) {
	return __builtin_ctzll(m) / 8;
}

static size_t mask_get_msb(group_mask_t m) {
	return __builtin_clzll(m) / 8;
}

static uint64_t loadu_64le(uint8_t *c) {
	uint64_t r = 0;
	r |= (uint64_t) c[0];
	r |= (uint64_t) c[1] << 8;
	r |= (uint64_t) c[2] << 8 * 2;
	r |= (uint64_t) c[3] << 8 * 3;
	r |= (uint64_t) c[4] << 8 * 4;
	r |= (uint64_t) c[5] << 8 * 5;
	r |= (uint64_t) c[6] << 8 * 6;
	r |= (uint64_t) c[7] << 8 * 7;
	return r;
}

static void store_64le(uint8_t *dst, uint64_t src) {
	dst[0] = src;
	dst[1] = src >> 8;
	dst[2] = src >> 8 * 2;
	dst[3] = src >> 8 * 3;
	dst[4] = src >> 8 * 4;
	dst[5] = src >> 8 * 5;
	dst[6] = src >> 8 * 6;
	dst[7] = src >> 8 * 7;
}

// this function is different from abseil's. it does not produce false positives
// and compiles into better code for 64-bit riscv.
static uint64_t group_match(uint8_t *c, uint8_t n) {
	uint64_t x = loadu_64le(c);
	uint64_t msbs = 0x8080808080808080ULL;
	uint64_t x2 = ~x & msbs;
	x = x ^ ((x2 >> 7) * (n^127));
	x += x2 >> 7;
	return x & x2;
}

static uint64_t group_match_empty(uint8_t *c) {
	uint64_t x = loadu_64le(c);
	uint64_t msbs = 0x8080808080808080ULL;
	return x & ~x << 6 & msbs;
}

static uint64_t group_match_empty_or_deleted(uint8_t *c) {
	uint64_t x = loadu_64le(c);
	uint64_t msbs = 0x8080808080808080ULL;
	return x & ~x << 7 & msbs;
}

static void hm_convert_deleted_to_empty_and_full_to_deleted(hash_map_t *h) {
	uint64_t msbs = 0x8080808080808080ULL;
	uint64_t lsbs = 0x0101010101010101ULL;
	hash_map_t hc = *h;
	uint8_t *c = hc.ctrl;
	for (;c < hc.ctrl + hc.capacity; c += kWidth) {
		uint64_t x = loadu_64le(c) & msbs;
		store_64le(c, (~x + (x >> 7)) & ~lsbs);
	}
	memcpy(hc.ctrl + hc.capacity + 1, hc.ctrl, kWidth-1);
	hc.ctrl[hc.capacity] = kSentinel;
}

static uint32_t group_count_empty_or_deleted(uint8_t *c) {
	uint64_t lsbs = 0x0101010101010101ULL;
	uint64_t ctrl = loadu_64le(c);
	uint64_t mask = (ctrl | ~(ctrl >> 7)) & lsbs;
	return mask == 0 ? 8 : mask_get_lsb(mask);
}
#endif /* */

static void hm_set_ctrl(hash_map_t *h, size_t i, uint8_t c) {
	h->ctrl[i] = c;
	h->ctrl[(i - (kWidth-1) & h->capacity) + (kWidth-1 & h->capacity)] = c;
}

static int hm_find_offset_with_hash(hash_map_t *h, hkey_t k, size_t hash, size_t *pos) {
	probe_t p = probe_new(h->capacity, hash);
	while (1) {
		group_mask_t hits = group_match(h->ctrl+p.offset, hash & 0x7F);
#ifdef HASHMAP_NEON_OPTS
		if (hits) {
			hits &= 0x8080808080808080ULL;
			goto skip_cond;
		}
#endif
		for (; hits; hits &= hits - 1) {
		skip_cond:;
			size_t i = mask_get_lsb(hits) + p.offset & h->capacity;
			if (hashkeyeq(k, h->slots[i].k)) {
				*pos = i;
				return 1;
			}
		}
		if (group_match_empty(h->ctrl+p.offset)) {
			return 0;
		}
		probe_next(&p, h->capacity);
	}
	return 0;
}

kv_pair *hm_find_with_hash(hash_map_t *h, hkey_t k, size_t hash) {
	uint64_t pos;
	if (hm_find_offset_with_hash(h, k, hash, &pos)) {
		return h->slots+pos;
	}
	return NULL;
}

kv_pair *hm_find(hash_map_t *h, hkey_t k) {
	return hm_find_with_hash(h, k, hashfn(k));
}

static size_t hm_find_first_non_full(hash_map_t *h, size_t hash) {
	probe_t p = probe_new(h->capacity, hash);
	while (1) {
		group_mask_t hits = group_match_empty_or_deleted(h->ctrl+p.offset);
		if (hits) {
			return mask_get_lsb(hits) + p.offset & h->capacity;
		}
		probe_next(&p, h->capacity);
	}
	return -1;
}

void hm_remove_by_ptr(hash_map_t *h, kv_pair *kv) {
	size_t pos = kv - h->slots;
	uint8_t *p = h->ctrl + pos;
	
	size_t index_before = pos - kWidth & h->capacity;
	group_mask_t empty_before = group_match_empty(h->ctrl + index_before);
	group_mask_t empty_after = group_match_empty(p);
	
	int x = empty_after && empty_before &&
	      (mask_get_msb(empty_before) + mask_get_lsb(empty_after) < kWidth);
	
	hm_set_ctrl(h, pos, x ? kEmpty : kDeleted);
	
	h->growth_left += x;
	h->size--;
}

int hm_remove(hash_map_t *h, hkey_t k) {
	size_t hash = hashfn(k);
	size_t pos;
	if (!hm_find_offset_with_hash(h, k, hash, &pos)) {
		return 0;
	}
	hm_remove_by_ptr(h, h->slots + pos);
	return 1;
}

static void hm_resize(hash_map_t *h, size_t new_cap) {
	hash_map_t hn = hm_new_with_length(new_cap);
	for (size_t i = 0; i < h->capacity; i++) {
		if (h->ctrl[i] & 0x80) {
			continue;
		}
		size_t hash = hashfn(h->slots[i].v);
		size_t ta = hm_find_first_non_full(&hn, hash);
		hn.slots[ta] = h->slots[i];
		hm_set_ctrl(&hn, ta, hash & 0x7F);
	}
	hn.size = h->size;
	hn.growth_left -= h->size;
	if (h->capacity > 0) {
		free(h->ctrl);
	}
	*h = hn;
}

static int hm_is_in_same_group(size_t hash, size_t cap, size_t i1, size_t i2) {
	i1 = i1 - hash & cap;
	i2 = i2 - hash & cap;
	return i2 / kWidth == i1 / kWidth;
}

static void hm_drop_deletes_without_resize(hash_map_t *h) {
	hm_convert_deleted_to_empty_and_full_to_deleted(h);
	kv_pair kv;
	for (size_t i = 0; i < h->capacity; i++) {
		if (h->ctrl[i] != kDeleted) {
			continue;
		}
		size_t hash = hashfn(h->slots[i].k);
		size_t new_i = hm_find_first_non_full(h, hash);
		if (hm_is_in_same_group(hash, h->capacity, i, new_i)) {
			hm_set_ctrl(h, i, hash & 0x7F);
		} else if (h->ctrl[new_i] == kEmpty) {
			hm_set_ctrl(h, new_i, hash & 0x7F);
			h->slots[new_i] = h->slots[i];
			hm_set_ctrl(h, i, kEmpty);
		} else if (h->ctrl[new_i] == kDeleted) {
			hm_set_ctrl(h, new_i, hash & 0x7F);
			kv = h->slots[new_i];
			h->slots[new_i] = h->slots[i];
			h->slots[i] = kv;
			i--;
		}
	}
	h->growth_left = hm_cap_to_growth(h->capacity) - h->size;
}

static void hm_rehash_and_grow_if_necessary(hash_map_t *h) {
	if (h->capacity == 0) {
		hm_resize(h, 1);
	} else if (h->capacity > kWidth
	        && h->size * (uint64_t){32} <= h->capacity * (uint64_t){25}) {
		hm_drop_deletes_without_resize(h);
	} else {
		hm_resize(h, h->capacity * 2 + 1);
	}
}

static size_t hm_prepare_insert(hash_map_t *h, size_t hash) {
	size_t t = hm_find_first_non_full(h, hash);
	if (h->growth_left == 0 && h->ctrl[t] != kDeleted) {
		hm_rehash_and_grow_if_necessary(h);
		t = hm_find_first_non_full(h, hash);
	}
	h->size++;
	h->growth_left -= h->ctrl[t] == kEmpty;
	hm_set_ctrl(h, t, hash & 0x7F);
	return t;
}

int hm_find_or_insert_with_hash(hash_map_t *h, hkey_t k, size_t hash, value_t v, kv_pair **res) {
	*res = hm_find_with_hash(h, k, hash);
	if (*res != NULL) {
		return 0;
	}
	size_t t = hm_prepare_insert(h, hash);
	h->slots[t].k = k;
	h->slots[t].v = v;
	*res = &h->slots[t];
	return 1;
}

int hm_find_or_insert(hash_map_t *h, hkey_t k, value_t v, kv_pair **res) {
	size_t hash = hashfn(k);
	return hm_find_or_insert_with_hash(h, k, hash, v, res);
}

int hm_assign(hash_map_t *h, hkey_t k, value_t v) {
	size_t hash = hashfn(k);
	kv_pair *loc = hm_find_with_hash(h, k, hash);
	if (loc != NULL) {
		loc->v = v;
		return 0;
	}
	size_t t = hm_prepare_insert(h, hash);
	h->slots[t].k = k;
	h->slots[t].v = v;
	return 1;
}

void hm_free(hash_map_t *h) {
	free(h->ctrl);
	h->ctrl = NULL;
	h->slots = NULL;
	h->size = h->capacity = h->growth_left = 0;
}

static hash_map_iter hm_iter_skip_empty_or_deleted(hash_map_iter hi) {
	size_t n;
	do {
		n = group_count_empty_or_deleted(hi.ictrl);
		hi.p += n;
		hi.ictrl += n;
	} while (n == kWidth);
	if (*hi.ictrl == kSentinel) {
		hi.p = NULL;
		hi.ictrl = NULL;
	}
	return hi;
}

hash_map_iter hm_iter_new(hash_map_t *h) {
	hash_map_iter hi = {h->slots, h->ctrl};
	return hm_iter_skip_empty_or_deleted(hi);
}

void hm_iter_next(hash_map_iter *i) {
	i->ictrl++;
	i->p++;
	*i = hm_iter_skip_empty_or_deleted(*i);
}

#undef HASHMAP
#undef HM1
#undef HM2
#undef HM3
#undef HM4
#undef HM5
#undef HM6
#undef HCAT1
#undef HCAT2
#undef HCAT3
#undef HASHNAME
#undef HASHKEY
#undef HASHVALUE
#undef HASHPREF_
#undef HASHPREF
#undef HASHFN
#undef HASHKEYEQ
#undef HASHMAP_NEON_OPTS
#undef kv_pair
#undef hash_map_t
#undef hash_map_iter
#undef hkey_t
#undef value_t
#undef probe_t
#undef group_mask_t
#undef hashfn
#undef hashkeyeq
#undef kEmpty
#undef kDeleted
#undef kSentinel
#undef kWidth
#undef hm_new
#undef hm_cap_to_growth
#undef hm_new_with_length
#undef hm_new_with_cap
#undef probe_new
#undef probe_next
#undef mask_get_lsb
#undef mask_get_msb
#undef loadu_64le
#undef store_64le
#undef group_match
#undef group_match_empty
#undef group_match_empty_or_deleted
#undef hm_convert_deleted_to_empty_and_full_to_deleted
#undef group_count_empty_or_deleted
#undef hm_set_ctrl
#undef hm_find_offset_with_hash
#undef hm_find_with_hash
#undef hm_find
#undef hm_find_first_non_full
#undef hm_remove_by_ptr
#undef hm_remove
#undef hm_resize
#undef hm_is_in_same_group
#undef hm_drop_deletes_without_resize
#undef hm_rehash_and_grow_if_necessary
#undef hm_prepare_insert
#undef hm_find_or_insert_with_hash
#undef hm_find_or_insert
#undef hm_assign
#undef hm_free
#undef hm_iter_skip_empty_or_deleted
#undef hm_iter_new
#undef hm_iter_next
