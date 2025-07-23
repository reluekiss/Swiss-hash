#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "khash.h" // Download from https://github.com/attractivechaos/klib

#define NOPS 100000

// Define the value type stored in the hash map
typedef struct {
	int num;
	char *string;
} my_type_t;

// Define the hash map type from string keys to my_type_t values
// This macro generates:
// - kh_map_t_t: the hash map type
// - kh_init_map_t, kh_destroy_map_t, etc.: functions for this map type
KHASH_MAP_INIT_STR(map_t, my_type_t)

// Generates a random string of a given length.
// The caller is responsible for freeing the returned memory.
char *random_string(size_t length) {
	static const char charset[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char *str = malloc(length + 1);
	if (!str) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i < length; i++) {
		str[i] = charset[rand() % (sizeof charset - 1)];
	}
	str[length] = '\0';
	return str;
}

// The benchmark macro, adapted to use khash's size function.
#define BENCHMARK(LABEL, VAR, CODE)                                            \
	do {                                                                       \
		printf("Starting " LABEL "s\n");                                       \
		clock_t diff = 0, batch_diff = 0;                                      \
		for (int op = 0; op < NOPS; op++) {                                    \
			clock_t start = clock();                                           \
			CODE;                                                              \
			clock_t end = clock();                                             \
			diff += end - start;                                               \
			batch_diff += end - start;                                         \
			if ((op + 1) % batch_size == 0) {                                  \
				printf(LABEL " avg @ size %u (ops %4d–%4d): %.3f ns\n",         \
					   kh_size(h), op - (batch_size - 1), op,                   \
					   ((double)batch_diff / CLOCKS_PER_SEC / batch_size) *     \
						   1e9);                                               \
				batch_diff = 0;                                                \
			}                                                                  \
		}                                                                      \
		double avg_##VAR = ((double)diff / CLOCKS_PER_SEC / NOPS) * 1e9;        \
		printf("Average " LABEL " time per op: %.6f ns\n\n", avg_##VAR);        \
	} while (0)

int main(void) {
	srand((unsigned)time(NULL));

	// Array to store pointers to the original keys for lookup/delete tests
	char *inserted_keys[NOPS];
	size_t inserted_count = 0;
	int batch_size = NOPS / 10;

	// Initialize the hash map
	kh_map_t_t *h = kh_init(map_t);

	BENCHMARK("Insert", insert, {
		char *key = random_string(10);
		my_type_t value = {0};
		value.num = rand();
		value.string = random_string(15);

		int ret;
		khint_t k = kh_put_map_t(h, key, &ret);
		// khash with INIT_STR strdup's the key, so we can free our copy
		free(key);

		// Set the value for the key
		kh_val(h, k) = value;

		// Store the key *from the hash table* for later use.
		// This is the pointer to the internally strdup'd key.
		inserted_keys[inserted_count++] = (char *)kh_key(h, k);
	});

	BENCHMARK("Lookup", lookup, {
		// Select a random key that we know was inserted
		char *key = inserted_keys[rand() % inserted_count];
		khint_t k = kh_get_map_t(h, key);
		// Access the value to make the lookup meaningful
		if (k != kh_end(h)) {
			(void)kh_val(h, k);
		}
	});

	BENCHMARK("Iterate", iterate, {
		size_t checksum = 0;
		khint_t k;
		// Standard iteration pattern for khash
		for (k = kh_begin(h); k != kh_end(h); ++k) {
			// Must check if the bucket exists
			if (kh_exist(h, k)) {
				checksum += kh_val(h, k).num;
			}
		}
		// Prevent the loop from being optimized away
		size_t sink = checksum;
		(void)sink;
	});

	BENCHMARK("Delete", delete, {
		size_t idx = rand() % inserted_count;
		char *key_to_del = inserted_keys[idx];

		khint_t k = kh_get_map_t(h, key_to_del);
		if (k != kh_end(h)) {
			// In a real application, you MUST free memory you own.
			// 1. Free the key which was strdup'd by khash.
			free((char *)kh_key(h, k));
			// 2. Free the string inside our value struct.
			free(kh_val(h, k).string);

			kh_del_map_t(h, k);

			// Replace the now-deleted key in our tracking array
			// with the one from the end to keep the array dense.
			inserted_keys[idx] = inserted_keys[--inserted_count];
		}
	});

	// --- Memory Cleanup ---
	// Free any remaining elements in the hash table
	printf("Cleaning up remaining %u elements...\n", kh_size(h));
	khint_t k;
	for (k = kh_begin(h); k != kh_end(h); ++k) {
		if (kh_exist(h, k)) {
			free((char *)kh_key(h, k));
			free(kh_val(h, k).string);
		}
	}
	kh_destroy_map_t(h);
	printf("Cleanup complete.\n\n");

	// Note: Memory analysis for khash is less direct than for the swissmap
	// implementation, as it doesn't expose all capacity metrics in the
	// public struct. This section is omitted as it cannot be implemented
	// in the same way as the reference.

	return 0;
}
