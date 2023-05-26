
Type-safe generic SwissTable implementation in C

## About
This is a generic C port of flat_hash_map from Google's C++ Abseil 
library, a hash table made to utilize SIMD instructions found on many modern 
CPUs. What makes this project a bit special is that i managed (with a certain 
amount of macro-trickery) to avoid having code in macros and make it all look 
like normal monomorphic C code, while allowing it to be instantiated possibly 
multiple times with different Key and Value types. While exact method is 
imperfect in some ways it isn't cumbersome to use and results in real C functions and 
types being generated accepting and containing exact Key and Value types the 
user provides.

For documentation and usage examples see [Usage](#usage)

## Project Status
Hash table functionality is done and all covered by tests. SIMD optimizations are done through intrinsics for
x86 and Aarch64. Exactly the same algorithms are used as found in Abseil's raw_hash_map and will be updated as Abseil updates.
The testing process is to be improved and proper benchmarking to be introduced.


## Usage
### Setting up
Just drop hash_map.c and hash_map.h into your include path. These files contain generic "template" code. To instantiate it
for your use-case you must provide typename to which hashmap type will be declared, typename of hashmap's key and typename
of hashmap's value, prefix for the function names, name of the hash function and a name for key comparison function.
This is done by declaring `HASHMAP` macro and then `#include`ing hash_map.c for example like this:
```c
#define HASHMAP (my_map_t, key_t, value_t, my_map_, hash_key_fn, key_eq_fn)
#include "hash_map.c"
```
where `(my_map_t, key_t, value_t, my_map_, hash_key_fn, key_eq_fn)` are parameters you specify to the "template". 
These parameters must be valid C identifiers, `key_t` and `value_t` must refer to defined types, `hash_key_fn` must be declared with signature like `size_t hash_key_fn(key_t)`, `key_eq_fn` must be declared with signature like `int key_eq_fn(key_t, key_t)`. As result of that `#include` a hashmap struct type with name `my_map_t` will be declared, it will store `key_t` keys and `value_t` values. All functions working with hashmap like `find` and `assign` will be declared with `my_map_` prefix like `my_map_find` and `my_map_assign`. Internally `hash_key_fn` will be called to calculate hashes of keys and `key_eq_fn` to compare them. `HASHMAP` macro will be `#undef`ed by hash_map.c. You can instantiate multiple different hashmap types for your program as long as you specify distinct prefixes and names of hashmap type.
`hash_map.h` file is the same as `hash_map.c` except it does not need `hash_key_fn` and `key_eq_fn` to be provided and will only make prototype declarations for hashmap functions making it suitable for `#include`ing from other header files.

### API
Function prototypes and struct definitions are given here as if hash_map.c was included with `my_map_t` as a hashmap typename, `key_t` as a key type, `value_t` as a value and `hm_` as a function prefix_

```c
typedef struct {
	/* some fields ommited */
	size_t size;
} my_map_t;
```
```c
typedef struct {
	key_t k;
	value_t v;
} my_kv_pair;
```
* `my_map_t hm_new(void);`
  - returns newly created empty hashmap, does not perform allocations.
* `my_map_t hm_new_with_cap(size_t sz);`
  - returns newly created hashmap with preallocated space that will be likely enough to hold `sz` number of elements.
* `hm_kv_pair *hm_find(my_map_t *h, key_t k);`
  - if `k` is present in `h` returns a pointer to that pair, otherwise returns `NULL`.
* `int hm_find_or_insert(my_map_t *h, key_t k, value_t v, hm_kv_pair **res)`
  - if `k` is present in `h` sets `*res` to point to that pair and returns 0, otherwise inserts `k` with `v`, sets `*res` to point to new pair and returns 1.
* `int hm_assign(my_map_t *h, key_t k, value_t v)`
  - if `k` is present in `h` updates it's value to `v` and returns 0, otherwise adds `k` with `v` and returns 1.
* `int hm_remove(my_map_t *h, key_t k)`
  - if `k` is present in `h` removes it and returns 1, otherwise returns 0.
* `void hm_remove_by_ptr(my_map_t *h, hm_kv_pair *kv)`
  - removes `*kv` from `h`. useful when key is deallocated and cant be hashed.
* `void hm_free(my_map_t *h)`
  - frees memory allocated by `h` making `h` unusable. if elements need deallocation you can deallocate them all by iterator and then call `hm_free`.
```c
typedef struct {
	my_kv_pair *p; // points to current kv pair or NULL
	/* some fields ommited */
} my_iter;
```
* `my_iter hm_iter_new(my_map_t *h)`
  - returns newly created iterator for `h`. Field `p` of the returned struct will point to some kv pair inside hashmap or will be NULL if there are no elements in the hashmap.
* `void hm_iter_next(my_iter *i)`
  - advances iterator `i` to point to yet-unvisited element in no meaningful order or `NULL` if all elements have been visited. It is invalid to call this function if hashmap had new element inserted since corresponding `hm_iter_new` was called (modifying or removing elements is ok).

### Examples

#### Sample string-to-int hashmap:
```c
uint64_t fnv_hash(char *s) {
	uint64_t hash = 0xcbf29ce484222325;
	while (*s) {
		hash *= 0x100000001b3;
		hash ^= (uint8_t)*s++;
	}
	return hash;
}

#define HASHMAP (strmap, char*, int, strmap_, fnv_hash, !strcmp)
#include <hash_map.c>

// since this is still C with manual memory management this wrapper
// is useful to handle freeing key string before removing the element
// itself as remove() wont do it for us 
int strmap_remove_string(strmap *m, char *s) {
	strmap_kv_pair *kv = strmap_find(m, s);
	if (kv == NULL) {
		return 0;
	}
	free(kv->k);
	strmap_remove_by_ptr(m, kv);
	return 1;
}

// free keys and hashmap memory 
void strmap_dealloc(strmap *m) {
	strmap_iter i = strmap_iter_new(m);
	for (; i.p != NULL; strmap_iter_next(&i)) {
		free(i.p->k);
	}
	strmap_free(m);
}
```

#### Iterator API is made such that in can be used with common for-loop pattern:
```c
for (my_iter i = hm_iter_new(h); i.p != NULL; hm_iter_next(&i)) {
	do_something_with_value(i.p->v);
}
```

## Possible future additions/improvements
These might be implemented in future. If you'd like to see some of them, let me know!
* additional set of functions that would take key or value arguments by pointers 
* porting parallel_hash_map, a map that builds upon raw_hash_map to provide some
concurrency and smooth memory-usage spikes on growth
* bundling hash functions for integers and strings optimized for 32 and 64 bit
hashes
* hardening against HashDoS, possibly as a separate implementation
* API to shrink hashmap's capacity
* integration with address sanitizer like in Abseil

