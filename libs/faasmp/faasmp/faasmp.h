#ifndef _FAASMP_H
#define _FAASMP_H

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
#include <random>


#ifdef __cplusplus
extern "C" {
#endif

void __faasmp_debug_copy(int *a, int *b);

struct AlignedElem {
    int32_t i = 0;

    AlignedElem() = default;

    AlignedElem(int32_t i) : i(i) {};

    friend inline bool operator==(const AlignedElem &lhs, const AlignedElem &rhs) {
        return lhs.i == rhs.i;
    };
} __attribute__ ((aligned(128)));

#ifdef __wasm__

std::string randomString(int len) {
    char result[len];

    static const char alphanum[] =
            "123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    static std::random_device rd;
    static std::mt19937 rng(rd());

    // Range cannot include last element of alphanum array as this is a null terminator
    std::uniform_int_distribution<int> uni(0, sizeof(alphanum) - 2);

    for (int i = 0; i < len; ++i) {
        int r = uni(rng);
        result[i] = alphanum[r];
    }

    return std::string(result, result + len);
}

class i64 {

private:
    int64_t x = 0;
    // Depending on the number of threads and the reduction, a copy constructor with the initial reductor
    // is created on some paths (i.e. for certain threads). A better implementation would deal with references
    // only so the copy constructor is as cheap as possible.
    // We would also like to support coercion from all arithmetic operators by having a logical default constructor
    // which we need to have no overhead compared to a raw arithmetic type
    // For now we keep this shorter than small string optimisations, could do cache line optimisation too.
    std::string reductionName;

    explicit i64() = default;

public:

    // Should be called on reduction init only and not in user code. This would be enforced by compiler.
    // For now we make the single argument constructor private.
    static i64 threadNew() {
        return i64();
    }

    explicit i64(int64_t x) : x(x), reductionName = randomString(12) {
    }

    void redisSum(i64 &out) {
        printf("+= %ld\n", x);
        printf("Brrr incrb or lock pull add push unlock += at key %s\n", out.reductionName.c_str());
    }

    /*
     * Overloadings: ideally we could want to take any type of input that coerces to int64_t via the
     * one argument constructor. However at the moment we use the one argument constructor to create
     * a random string (on initialisation of the first reductor) which allows us to make this type
     * completely transparent in the native implementation (i64 = int64_t). I think concepts to specialise
     * those operators can help.
     */

    i64 &operator+=(const int64_t other) {
        x += other;
        return *this;
    }

    i64 &operator-=(const int64_t other) {
        x -= other;
        return *this;
    }

    i64 &operator++() {
        ++x;
        return *this;
    }

    i64 &operator--() {
        --x;
        return *this;
    }

    operator double() const {
        return (double) x;
    }

    operator int64_t() const {
        return x;
    }
};


#endif

#ifdef __cplusplus
}
#endif

#ifdef __wasm__
#pragma omp declare reduction \
(+: i64: omp_in.redisSum(omp_out)) \
initializer(omp_priv=i64::threadNew())

#else // i.e not __wasm__

using i64 = int64_t;

#endif // __wasm__


#endif // _FAASMP_H
