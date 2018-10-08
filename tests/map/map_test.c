#include <stdio.h>
#include <stdlib.h> // rand
#include <limits.h> // INT_MAX
#include <assert.h> // assert
#include <ctype.h> // isspace
#include <string.h> // strtok
#include <time.h> // clock

#include "builtin.h"

static void slugify(char *s) {
    int i = 0;
    int j = 0;
    while (s[i]) {
        s[i] = s[j];
        if (isalpha(s[i]) || isdigit(s[i]) || isspace(s[i])) {
            if ('A' <= s[i] && s[i] <= 'Z')
                s[i] = s[i] - 'A' + 'a';
            i++;
            j++;
        } else {
            j++;
        }
    }
}

static void test_shakespeare(void) {
    map_t m;
    map_init(&m, &desc_str, &desc_int);
    FILE *fp = fopen("t8.shakespeare.txt", "r");
    char *line = NULL;
    size_t line_len = 0;
    const char *sep = " \n";
    while (getline(&line, &line_len, fp) >= 0) {
        slugify(line);
        char *key = strtok(line, sep);
        if (!key)
            continue;
        do {
            if (key && *key && !isspace(*key)) {
                int val = 0;
                map_get(&m, &key, &val);
                ++val;
                map_set(&m, &key, &val);
            }
        } while ((key = strtok(NULL, sep)));
    }
    fclose(fp);
    printf("most common words in Shakespeare:\n");
    slice_t keys = map_keys(&m);
    for (int i = 0; i < len(&keys); ++i) {
        char *key;
        int val;
        slice_get(&keys, i, &key);
        map_get(&m, &key, &val);
        if (val > 4000) {
            printf("\t%s (%d uses)\n", key, val);
        }
    }
    map_deinit(&m);
}

static void test_ints(void) {
    const int num_iters = 1024;
    static int elts[num_iters];
    map_t m;
    map_init(&m, &desc_int, &desc_int);
    srand(0);
    for (int i = 0; i < num_iters; i++) {
        int key = rand() % num_iters;
        ++elts[key];
        map_set(&m, &key, &elts[key]);
    }
    int total = 0;
    slice_t keys = map_keys(&m);
    for (int i = 0; i < slice_len(&keys); i++) {
        int key, val;
        slice_get(&keys, i, &key);
        assert(!map_get(&m, &key, &val));
        assert(val == elts[key]);
        total += val;
    }
    slice_deinit(&keys);
    map_deinit(&m);
    assert(total == num_iters);
}

int main() {
    clock_t start = clock();
    /* test_ints(); */
    test_shakespeare();
    printf("%f secs\n", (float)(clock() - start) / CLOCKS_PER_SEC);
    printf("map_lookups: %d\n", map_lookups);
    printf("map_hits: %d\n", map_hits);
    printf("map_misses: %d\n", map_misses);
    printf("map_iters: %d\n", map_iters);
    printf("hit%%: %f\n", (float)map_hits / map_lookups);
    printf("iters/lookup: %f\n", (float)map_iters / map_lookups);
    printf("done\n");
    return 0;
}
