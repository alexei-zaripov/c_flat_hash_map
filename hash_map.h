// SPDX-License-Identifier: Apache-2.0

#ifndef HASHMAP
#error "this file needs HASHMAP defined"
#endif

#define HM1(a, b, c, d) a
#define HM2(a, b, c, d) b
#define HM3(a, b, c, d) c
#define HM4(a, b, c, d) d
#define HCAT1(a, b) a ## b
#define HCAT2(a, b) HCAT1(a, b)
#define HCAT3(a, b) HCAT2(a, b)
#define HASHNAME(a) HM1 a
#define HASHKEY(a) HM2 a
#define HASHVALUE(a) HM3 a
#define HASHPREF_(a) HM4 a
#define HASHPREF HASHPREF_(HASHMAP)
#define kv_pair HCAT3(HASHPREF, kv_pair)
#define hash_map_t HASHNAME(HASHMAP)
#define hash_map_iter HCAT3(HASHPREF, iter)
#define hkey_t HASHKEY(HASHMAP)
#define value_t HASHVALUE(HASHMAP)
#define hm_new HCAT3(HASHPREF, new)
#define hm_new_with_cap HCAT3(HASHPREF, new_with_cap)
#define hm_find_with_hash HCAT3(HASHPREF, find_with_hash)
#define hm_find HCAT3(HASHPREF, find)
#define hm_remove_by_ptr HCAT3(HASHPREF, remove_by_ptr)
#define hm_remove HCAT3(HASHPREF, remove)
#define hm_find_or_insert_with_hash HCAT3(HASHPREF, find_or_insert_with_hash)
#define hm_find_or_insert HCAT3(HASHPREF, find_or_insert)
#define hm_assign HCAT3(HASHPREF, assign)
#define hm_free HCAT3(HASHPREF, free)
#define hm_iter_new HCAT3(HASHPREF, iter_new)
#define hm_iter_next HCAT3(HASHPREF, iter_next)

typedef struct {
	hkey_t k;
	value_t v;
} kv_pair;

typedef struct {
	uint8_t *ctrl;
	kv_pair *slots;
	size_t size, capacity, growth_left;
} hash_map_t;

typedef struct {
	kv_pair *p;
	uint8_t *ictrl;
} hash_map_iter;

hash_map_t hm_new(void);
hash_map_t hm_new_with_cap(size_t sz);

kv_pair *hm_find_with_hash(hash_map_t *h, hkey_t k, size_t hash);
kv_pair *hm_find(hash_map_t *h, hkey_t k);
int hm_find_or_insert_with_hash(hash_map_t *h, hkey_t k, size_t hash, value_t v, kv_pair **res);
int hm_find_or_insert(hash_map_t *h, hkey_t k, value_t v, kv_pair **res);

void hm_remove_by_ptr(hash_map_t *h, kv_pair *kv);
int hm_remove(hash_map_t *h, hkey_t k);
int hm_assign(hash_map_t *h, hkey_t k, value_t v);
void hm_free(hash_map_t *h);

hash_map_iter hm_iter_new(hash_map_t *h);
void hm_iter_next(hash_map_iter *i);

#undef HASHMAP
#undef HM1
#undef HM2
#undef HM3
#undef HM4
#undef HCAT1
#undef HCAT2
#undef HCAT3
#undef HASHNAME
#undef HASHKEY
#undef HASHVALUE
#undef HASHPREF_
#undef HASHPREF
#undef kv_pair
#undef hash_map_t
#undef hash_map_iter
#undef hkey_t
#undef value_t
#undef hm_new
#undef hm_new_with_cap
#undef hm_find_with_hash
#undef hm_find
#undef hm_remove_by_ptr
#undef hm_remove
#undef hm_find_or_insert_with_hash
#undef hm_find_or_insert
#undef hm_assign
#undef hm_free
#undef hm_iter_new
#undef hm_iter_next
