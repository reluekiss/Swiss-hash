#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "uthash.h" // Download from https://github.com/troydhanson/uthash

#define NOPS 100000

// With uthash, the structure must contain the key and the hash handle.
typedef struct {
	char *key; // The key (must be a member of the struct)
	int num;
	char *string;
	UT_hash_handle hh; // This makes the structure hashable
} my_type_t;

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

// The benchmark macro, adapted to use uthash's HASH_COUNT.
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
					   HASH_COUNT(users), op - (batch_size - 1), op,           \
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

	// The hash table is just a pointer to the head of the list of structs.
	// It MUST be initialized to NULL.
	my_type_t *users = NULL;

	// This array will store pointers to the actual structs we've inserted.
	my_type_t *inserted_elements[NOPS];
	size_t inserted_count = 0;
	int batch_size = NOPS / 10;

	BENCHMARK("Insert", insert, {
		// With uthash, we must allocate memory for the whole struct.
		my_type_t *s = malloc(sizeof(my_type_t));
		s->key = random_string(10);
		s->num = rand();
		s->string = random_string(15);

		// Add the new struct to the hash table.
		// The 2nd arg is the name of the key field in the struct.
		HASH_ADD_STR(users, key, s);

		// Store a pointer to the struct for later lookup/delete tests.
		inserted_elements[inserted_count++] = s;
	});

	BENCHMARK("Lookup", lookup, {
		// Get a random element that we know was inserted.
		my_type_t *element_to_find = inserted_elements[rand() % inserted_count];
		char *key_to_find = element_to_find->key;

		my_type_t *found_user;
		// Find the element by its key string.
		HASH_FIND_STR(users, key_to_find, found_user);

		// Use the result to prevent the lookup from being optimized away.
		(void)found_user;
	});

	BENCHMARK("Iterate", iterate, {
		size_t checksum = 0;
		my_type_t *current_user;
        my_type_t *tmp;

		// HASH_ITER is the safe iteration macro.
		HASH_ITER(hh, users, current_user, tmp) {
			checksum += current_user->num;
		}

		// Prevent the loop from being optimized away.
		size_t sink = checksum;
		(void)sink;
	});

	BENCHMARK("Delete", delete, {
		size_t idx = rand() % inserted_count;
		my_type_t *element_to_del = inserted_elements[idx];

		// Delete the element from the hash table.
		HASH_DEL(users, element_to_del);

		// Now, we are responsible for freeing the memory.
		free(element_to_del->key);
		free(element_to_del->string);
		free(element_to_del); // Free the struct itself.

		// Replace the now-deleted element in our tracking array
		// with the one from the end to keep the array dense.
		inserted_elements[idx] = inserted_elements[--inserted_count];
	});

	// --- Memory Cleanup ---
	// Free any remaining elements in the hash table.
	printf("Cleaning up remaining %u elements...\n", HASH_COUNT(users));
	my_type_t *current_user, *tmp;
	HASH_ITER(hh, users, current_user, tmp) {
		HASH_DEL(users, current_user); // remove from hash
		free(current_user->key);
		free(current_user->string);
		free(current_user); // free the struct
	}
	printf("Cleanup complete.\n\n");

	// Note: Memory usage analysis for uthash is not straightforward as it
	// does not expose internal capacity/bucket metrics. This section is
	// omitted as it cannot be implemented in the same way as the reference.

	return 0;
}
