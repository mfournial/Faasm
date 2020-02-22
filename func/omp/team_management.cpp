#include<cstdio>
#include <cstdlib>
#include <omp.h>
//#include <faasm/faasm.h>

int foo() {
    return 4;
}

int main() {
    int res = 0, n = 0;
    #pragma omp target teams num_teams(foo()) default(none) shared(n) map(res, n) reduction(+:res)
    {
        res = omp_get_team_num();
        if (omp_get_team_num() == 0)
            n = omp_get_num_teams();
    }

    const int expected = (n * (n - 1)) / 2;    // Sum of first n-1 natural numbers
    printf("Expected %d but got %d\n", expected, res);
    if (res != expected) {
        printf("Map team distribute error. Expected %d but got %d\n", expected, res);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
