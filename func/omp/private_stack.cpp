#include <omp.h>
#include <stdio.h>
#include <faasm/faasm.h>

bool fail = false;
bool accessed = false; /* racy: should be atomic bool */

int random_stuff(int x, int y, int *z, int *l, int *m, int n) {
    if (x == *z) {
        n += *m + *l;
        if (n >= 0) {
            if (y > 0 && *z == omp_get_thread_num() && x == omp_get_thread_num()) {
                *z = 100;
                *l = 101 * x;
                *m = 102;
                return 200;
            }
            else {
                return -2;
            }
        }
        else {
            return -3;
        }
    }
    else {
        return -1;
    }
}

FAASM_MAIN_FUNC() {
    #pragma omp parallel default(none) shared(fail) num_threads(4)
    {
        int num = omp_get_thread_num();
        int y = 3 * num + 1;
        int l = 2 * num + 3;
        int m = y / (l + 1);
        int n = m * m + 6;
        if (random_stuff(num, y, &num, &l, &m, n) != 200) {
            fail = true;
        }
        if (num != 100 || l != 101 * omp_get_thread_num() || m != 102) {
            fail = true;
        }
    }
    return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
