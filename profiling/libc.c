#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <time.h>

#define NOPS 10000

typedef struct {
  int   num;
  char *string;
} my_type_t;

char* random_string(size_t length) {
  static const char charset[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";
  char *str = malloc(length + 1);
  if (!str) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < length; i++) {
    str[i] = charset[rand() % (sizeof charset - 1)];
  }
  str[length] = '\0';
  return str;
}

#define BENCHMARK(LABEL, VAR, CODE)                                         \
  do {                                                                      \
    printf("Starting " LABEL "s\n");                                        \
    clock_t diff = 0, batch_diff = 0;                                       \
    for (int op = 0; op < NOPS; op++) {                                     \
      clock_t start = clock();                                              \
      CODE;                                                                 \
      clock_t end = clock();                                                \
      diff += end - start;                                                  \
      batch_diff += end - start;                                            \
      if ((op + 1) % batch_size == 0) {                                     \
        printf(                                                             \
            LABEL " avg @ size %zu (ops %4d–%4d): %.3f ns\n",               \
            inserted_count,                                                 \
            op - (batch_size - 1),                                          \
            op,                                                             \
            ((double)batch_diff / CLOCKS_PER_SEC / batch_size) * 1e9);      \
        batch_diff = 0;                                                     \
      }                                                                     \
    }                                                                       \
    double avg_##VAR = ((double)diff / CLOCKS_PER_SEC / NOPS) * 1e9;        \
    printf("Average " LABEL " time per op: %.6f ns\n\n", avg_##VAR);        \
  } while (0)

int main(void) {
  srand((unsigned)time(NULL));

  if (!hcreate(2 * NOPS)) {
    perror("hcreate");
    return EXIT_FAILURE;
  }

  char **inserted_keys =
      malloc(NOPS * sizeof(*inserted_keys));
  if (!inserted_keys) {
    perror("malloc");
    hdestroy();
    return EXIT_FAILURE;
  }
  size_t inserted_count = 0;
  int batch_size = NOPS / 10;

  BENCHMARK("Insert", insert, {
    char *key = random_string(10);
    my_type_t *value = malloc(sizeof(*value));
    if (!value) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    value->num = rand();
    value->string = random_string(15);
    ENTRY e = {0};
    e.key = key;
    e.data = (void*)value;
    ENTRY *r = hsearch(e, ENTER);
    if (!r) {
      fprintf(stderr, "hsearch ENTER failed\n");
      exit(EXIT_FAILURE);
    }
    inserted_keys[inserted_count++] = key;
  });

  BENCHMARK("Lookup", lookup, {
    char *key = inserted_keys[rand() % inserted_count];
    ENTRY e = { .key = key };
    ENTRY *r = hsearch(e, FIND);
    (void)r;  /* discard the result */
  });

  BENCHMARK("Iterate", iterate, {
    size_t checksum = 0;
    for (size_t i = 0; i < inserted_count; i++) {
      ENTRY e = { .key = inserted_keys[i] };
      ENTRY *r = hsearch(e, FIND);
      my_type_t *v = (my_type_t *)r->data;
      checksum += v->num;
    }
    (void)checksum;
  });

  BENCHMARK("Delete", delete, {
    size_t idx = rand() % inserted_count;
    char *key = inserted_keys[idx];
    ENTRY e = { .key = key };
    ENTRY *r = hsearch(e, FIND);
    if (r && r->data) {
      my_type_t *v = (my_type_t *)r->data;
      free(v->string);
      free(v);
      r->data = NULL;              /* mark as deleted */
      free(key);                   /* free the key string */
      inserted_keys[idx] =
          inserted_keys[--inserted_count];
    }
  });

  hdestroy();
  free(inserted_keys);
  return 0;
}
