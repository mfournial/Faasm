#include <omp.h>
#include <cstdio>

float dotprod(const float B[], const float C[], const int N);

int main() {
    const int n = 100;
    float b[n];
    float c[n];
    float expected = 0;
    for (int i = 0; i < n; i++) {
        b[i] = 2 * i;
        c[i] = 3 * i % 10;
        expected += b[i] * c[i];
    }
    float sum = dotprod(b, c, n);
    if (sum != expected) {
        printf("Bad dot product accross devices and teams. Expected %f but got %f\n", expected, sum);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//#pragma omp declare target
//extern void get_offload_dev();
//#pragma omp end declare target

float dotprod(const float B[], const float C[], const int N) {
    float sum0 = 0.0;
    float sum1 = 0.0;
    #pragma omp target map(to: B[:N], C[:N])
    #pragma omp teams num_teams(2)
    {
        int i;
        if (omp_get_num_teams() != 2)
            abort();

//        get_offload_dev();

        if (omp_get_team_num() == 0)
        {
            #pragma omp parallel for reduction(+:sum0)
            for (i=0; i<N/2; i++)
                sum0 += B[i] * C[i];
        }
        else if (omp_get_team_num() == 1)
        {
            #pragma omp parallel for reduction(+:sum1)
            for (i=N/2; i<N; i++)
                sum1 += B[i] * C[i];
        }
    }
    return sum0 + sum1;
}
