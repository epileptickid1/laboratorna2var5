

#include <algorithm>
#include <execution>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <numeric>
#include <iomanip>

using namespace std;
using clk = chrono::steady_clock;


vector<int> make_data(size_t n) {
    mt19937 gen(42);
    uniform_int_distribution<int> dist(1, 1'000'000);
    vector<int> v(n);
    for (auto& x : v) x = dist(gen);
    return v;
}

template <typename F>
double measure(F func) {
    auto t1 = clk::now();
    func();
    auto t2 = clk::now();
    return chrono::duration<double>(t2 - t1).count();
}


int parallel_min(const vector<int>& v, int K) {
    size_t n = v.size();
    vector<int> local_min(K);
    vector<thread> threads;

    size_t block = n / K;

    for (int i = 0; i < K; ++i) {
        size_t begin = i * block;
        size_t end = (i == K - 1) ? n : begin + block;

        threads.emplace_back([&, i, begin, end] {
            local_min[i] = *min_element(v.begin() + begin, v.begin() + end);
            });
    }

    for (auto& t : threads) t.join();

    return *min_element(local_min.begin(), local_min.end());
}



int main() { //using MSVC
    cout << fixed << setprecision(6);

    vector<size_t> sizes = { 100'000, 1'000'000, 10'000'000 };
    unsigned hw = thread::hardware_concurrency();

    cout << "CPU threads: " << hw << "\n\n";

    for (size_t n : sizes) {
        cout << "=============================\n";
        cout << "Data size: " << n << "\n";

        auto data = make_data(n);

        
        double t_no_policy = measure([&] {
            volatile int m = *min_element(data.begin(), data.end());
            });

        cout << "min_element (no policy): " << t_no_policy << " s\n";

        
        double t_seq = measure([&] {
            volatile int m =
                *min_element(execution::seq, data.begin(), data.end());
            });

        double t_par = measure([&] {
            volatile int m =
                *min_element(execution::par, data.begin(), data.end());
            });

        double t_par_unseq = measure([&] {
            volatile int m =
                *min_element(execution::par_unseq, data.begin(), data.end());
            });

        cout << "seq        : " << t_seq << " s\n";
        cout << "par        : " << t_par << " s\n";
        cout << "par_unseq  : " << t_par_unseq << " s\n";

        
        cout << "\nCustom parallel algorithm:\n";
        cout << "K\tTime (s)\n";

        double best_time = 1e9;
        int best_k = 1;

        for (int K = 1; K <= (int)hw * 2; ++K) {
            double t = measure([&] {
                volatile int m = parallel_min(data, K);
                });

            cout << K << "\t" << t << "\n";

            if (t < best_time) {
                best_time = t;
                best_k = K;
            }
        }

        cout << "Best K = " << best_k
            << " (CPU threads = " << hw << ")\n";
        cout << "=============================\n\n";
    }

    return 0;
}
