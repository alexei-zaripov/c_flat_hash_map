#include "libc.h"

#define HASHMAP (iimap, uint32_t, uint32_t, iimap_)
#include "hash_map.h"

void test1(iimap *h) {
	for (int i = 0; i < 20000; i++) {
		iimap_assign(h, i, 0);
	}
	puts("stage1");
	for (int i = 0; i < 20006; i += 2) {
		iimap_remove(h, i);
	}
	puts("stage2");
	for (int i = 19000; i < 30000; i++) {
		iimap_assign(h, i, 2);
	}
	for (int i = 60000; i < 90000; i++) {
		iimap_assign(h, i, 3);
	}
	puts("stage3");
	for (int i = 19000; i < 30001; i++) {
		iimap_kv_pair *s = iimap_find(h, i);
		if (s && s->v != 2) {
			errx(1, "found non 2");
		}
	}
}

void test2(iimap *h) {
	size_t big = 268435456/8;
//	size_t big = 512 * 512 -1;
	size_t lim = big - big/8;
	puts(" -- stage1:");
	for (size_t i = 0; i < lim; i++) {
		iimap_assign(h, i, i);
	}
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	puts(" -- stage2:");
	for (size_t i = 0; i < big/32*7+100; i++) {
		iimap_remove(h, i);
	}
	printf("cond=%d\n", h->size * (uint64_t){32} <= h->capacity * (uint64_t){25});
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	puts(" -- stage3:");
	for (size_t i = 0; i < big/32*7+100; i++) {
		iimap_assign(h, i, i);
	}
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	for (size_t i = 0; i < lim; i++) {
		iimap_kv_pair *v = iimap_find(h, i);
		if (!v || v->v != i) {
			puts("corrupt!");
			break;
		}
	}
}

void test3(iimap *h) {
	puts("test3:");
	size_t big = 268435456/512 - 1;
	size_t lim = big - big/8;
	size_t rlim = big * 25 / 32;
	size_t counter = 0;
	for (size_t i = 0; i < lim; i++) {
		iimap_assign(h, i, i);
	}
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	for (size_t i = 0; h->size > rlim - 100; i++) {
		iimap_remove(h, i);
	}
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	for (size_t i = 0; h->size != rlim; i++) {
		iimap_assign(h, i+big, i);
	}
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	for (size_t i = 0; i < big*10; i++) {
		iimap_assign(h, i+big+1, i);
		iimap_remove(h, i+big);
	}
	printf("size=%zd, cap=%zd, gl=%zd\n", h->size, h->capacity, h->growth_left);
	
	iimap_iter it = iimap_iter_new(h);
	for (; it.p != NULL; iimap_iter_next(&it)) {
		counter++;
	}
	printf("counter=%zd\n", counter);
}

int main() {
	iimap h = iimap_new_with_cap(7);
	iimap_kv_pair *f;
	if (!iimap_find_or_insert(&h, 1, 1, &f)) {
		puts("wrong!");
		return 0;
	}
	if (iimap_find_or_insert(&h, 1, 2, &f)) {
		puts("wrong!");
		return 0;
	}
	if (f->v != 1) {
		puts("wrong!");
		return 0;
	}
	if (!iimap_remove(&h, 1)) {
		puts("wrong!");
		return 0;
	}
	test1(&h);
	iimap_free(&h);
	h = iimap_new();
	test2(&h);
	iimap_free(&h);
	h = iimap_new_with_cap(0);
	test3(&h);
	iimap_free(&h);
	return 0;
}
