#include <iostream>
#include <random>
#include <chrono>
#include "PatienceSort.h"

using namespace std;



int main() {

    const int count = 10000000;
    const int max_value = 100000;
    const float randomness = 0.1f;          // Use 10% random values

    const int rounds = 10;                  // builds the average from 10 cycles
    vector<int> ps, ref;
    vector<float> ref_results, ps_results;
    float ref_result = 0;
    float ps_result = 0;
    int num_randoms;

    std::random_device dev;
    std::mt19937 mt(dev());
    std::uniform_int_distribution<int> dist_value(0, max_value);
    std::uniform_int_distribution<int> dist_rand(0, count);


    num_randoms = count * randomness;
    for(size_t i = 0; i < count; i++) {
        ps.push_back(i);
    }
    for(size_t i = 0; i < num_randoms; i++) {
        ps[dist_rand(mt)] = dist_value(mt);
    }
    ref = ps;

    cout << "Sorting " << count << " integers with a size of " << sizeof(int) << endl;

    for(int i = 0; i < rounds; i++) {
        vector<int> values_ps, values_ref;
        values_ps = ps;
        values_ref = ref;

        auto t0 = std::chrono::high_resolution_clock::now();
        sort(values_ref.begin(), values_ref.end());
        auto t1 = std::chrono::high_resolution_clock::now();
        PatienceSortFunc(values_ps.begin(), values_ps.end());
        auto t2 = std::chrono::high_resolution_clock::now();


        ref_results.push_back(chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        ps_results.push_back(chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());

    }

    for_each(ref_results.begin(), ref_results.end(), [&](float f){ref_result += f;});
    for_each(ps_results.begin(), ps_results.end(), [&](float f){ps_result += f;});
    ref_result /= ref_results.size();
    ps_result /= ps_results.size();


    cout << "std::sort:\t" << ref_result << " ms" << endl;
    cout << "Patience Sort:\t" << ps_result << " ms" << endl;


    return 0;
}
