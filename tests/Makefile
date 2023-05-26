
deps := iimap.c test_iimap.c ../hash_map.c ../hash_map.h

test_iimap: $(deps)
	gcc -march=native -Og -ggdb -fsanitize=address -I.. test_iimap.c --coverage iimap.c -o test_iimap

clean:
	rm *.gcno *.gcda *.gcov