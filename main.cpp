#include <iostream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cassert>
#include <chrono>

#include <cilk/cilk.h>

std::ptrdiff_t constexpr BLOCK = 32;
int const RUNS = 5;

std::mt19937 rng = std::mt19937(std::random_device()());

template <typename T>
void assert_sorted(T* first, T* last) {
    while (first + 1 < last) {
        assert(*first <= *(first + 1));
        ++first;
    }
}

template<class T>
T* partition(T* first, T* last, T piv) {
    --last;
    while (first <= last) {
        while (*first < piv) {
            ++first;
        }
        while (*last > piv) {
            --last;
        }
        if (first >= last) {
            break;
        }
        if (first != last) {
            std::iter_swap(first, last);
            ++first;
            --last;
        }
    }
    return last + 1;
}

template <typename T>
void seq_sort(T* first, T* last) {
    if (last - first < 2) {
        return;
    }
    T pivot = *(first + (rng() % (last - first)));

    // std::cout << "pivot " << pivot << std::endl;
    // std::cout << "before part [";
    // for (T* it = first; it != last; ++it) {
    //     std::cout << ' ' << *it;
    // }
    // std::cout << std::endl;

    T* middle = partition(first, last, pivot);
    seq_sort(first, middle);
    seq_sort(middle, last);
}

template <typename T>
void par_sort(T* first, T* last) {
    if (last - first < BLOCK) {
        std::sort(first, last);
        return;
    }
    T pivot = *(first + (rng() % (last - first)));
    T* middle = partition(first, last, pivot);
    cilk_scope {
        cilk_spawn par_sort(first, middle);
        par_sort(middle, last);
    }
}

int main(int argc, char* argv[]) {
    long n = 100 * 1000 * 1000;
    if (argc == 2) {
        n = std::atoi(argv[1]);
    }

    std::uniform_int_distribution<long> dist(0,n);
    std::vector<long> a(n);
    std::vector<long> b(n);

    double ssum = 0.0;

    for (int i = 0; i < RUNS; ++i) {
        std::generate(a.begin(), a.end(), [&]() { return dist(rng); });
        std::copy(a.begin(), a.end(), b.begin());
        auto t1 = std::chrono::high_resolution_clock::now();
        seq_sort(a.data(), a.data() + a.size());
        // std::cout << "seq sorted" << std::endl;
        auto t2 = std::chrono::high_resolution_clock::now();
        par_sort(b.data(), b.data() + b.size());
        // std::cout << "seq sorted" << std::endl;
        auto t3 = std::chrono::high_resolution_clock::now();

        assert_sorted(a.data(), a.data() + a.size());
        assert_sorted(b.data(), b.data() + b.size());

        auto seq_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
        auto par_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2);
        double speedup = (double) seq_duration.count() / (double) par_duration.count();
        std::cout << "sequential: " << seq_duration.count() << " ms" << std::endl;
        std::cout << "parallel  : " << par_duration.count() << " ms" << std::endl;
        std::cout << "speedup: " << speedup << std::endl;
        ssum += speedup;
    }

    std::cout << std::setprecision(10) << "average speedup " << ssum / RUNS << std::endl;

    return 0;
}
