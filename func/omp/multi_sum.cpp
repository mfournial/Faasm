#include <omp.h>
#include <cstdio>
#include <random>

#define num_devices num_threads

// Small
constexpr long ITERATIONS = 20000000;
// Big
//constexpr long ITERATIONS = 200000000;

long random_thread_int() {
    int threadNum = omp_get_thread_num();
    return threadNum * threadNum * 77 - 23 * threadNum + 1927;
}

int main() {
//    omp_set_default_device(-2);
    long result = 0;
    #pragma omp parallel num_devices(10) default(none) reduction(+:result)
    {
        std::uniform_real_distribution<double> unif(0, 1);
        std::mt19937 generator(random_thread_int());
        double x, y;
        #pragma omp for
        for (long i = 0; i < ITERATIONS; i++) {
            x = unif(generator);
            y = unif(generator);
            if (x * x + y * y <= 1.0) {
                result++;
            }
        }
    }

    double pi = (4.0 * result) / ITERATIONS;

    if (pi - 3.1415 > 0.0001) {
        printf("Custom reduction failed. Expected pi got %f\n", pi);
//        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
