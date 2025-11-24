/*****************************************************************************************
 * Advanced Data Structures Project
 * Hierarchical / Multi-Resolution Compound Bloom Filter (HR-CBF)
 *
 * Features:
 *  - Classic BloomFilter implementation (bit array + k hash functions)
 *  - Compound Bloom Filter (multiple filters, AND semantics)
 *  - HR-CBF (Hierarchical Bloom Filter)
 *        → geometric bit allocation per level
 *        → optimal per-level k ≈ (m_i / n) * ln(2)
 *        → cascaded early-stop lookup
 *
 * This file integrates all three implementations and runs
 * a controlled experiment comparing:
 *      1. Single Bloom Filter
 *      2. 2-Filter Compound Bloom Filter (same memory)
 *      3. 2-Filter Compound Bloom Filter (double memory)
 *      4. HR-CBF (same total memory)
 *
 * Compilation:
 *      g++ -O2 -std=c++17 hr_cbf.cpp -o hr_cbf
 *****************************************************************************************/

#include <bits/stdc++.h>
using namespace std;

using u64 = unsigned long long;
using u32 = unsigned int;

/*-----------------------------------------------------------
 * Hash Utilities (SplitMix64)
 *-----------------------------------------------------------*/
static inline u64 splitmix64(u64 x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

/*-----------------------------------------------------------
 * Hasher: Double Hashing with Independent Salt
 *-----------------------------------------------------------*/
struct Hasher {
    u64 salt;

    explicit Hasher(u64 s = 0) : salt(s) {}

    pair<u64, u64> base_hashes(const string &data) const {
        u64 h1 = std::hash<string>{}(data) ^ salt;
        u64 h2 = splitmix64(h1 + salt + 0x9e3779b97f4a7c15ULL);
        if (h2 == 0) h2 = 0x9e3779b97f4a7c15ULL;   // avoid zero multiplier
        return {h1, h2};
    }

    inline u64 hash_i(const string &data, size_t i, u64 m) const {
        auto [h1, h2] = base_hashes(data);
        return (h1 + i * h2) % m;
    }
};

/*-----------------------------------------------------------
 * BitArray: Compact Bit Storage
 *-----------------------------------------------------------*/
class BitArray {
private:
    vector<uint8_t> bytes;
    size_t bit_count;

public:
    explicit BitArray(size_t bits = 0) { reset(bits); }

    void reset(size_t bits) {
        bit_count = bits;
        bytes.assign((bits + 7) / 8, 0);
    }

    inline void set(size_t idx) {
        bytes[idx >> 3] |= (1u << (idx & 7));
    }

    inline bool get(size_t idx) const {
        return (bytes[idx >> 3] >> (idx & 7)) & 1u;
    }

    size_t size_bits() const { return bit_count; }
};

/*-----------------------------------------------------------
 * BloomFilter: Classic Implementation
 *-----------------------------------------------------------*/
class BloomFilter {
private:
    size_t m;                // number of bits
    size_t k;                // number of hash functions
    BitArray bits;
    Hasher hasher;

public:
    BloomFilter(size_t m_bits = 1024, size_t k_hashes = 3, u64 salt = 0)
        : m(m_bits), k(k_hashes), bits(m_bits), hasher(salt) {}

    void insert(const string &item) {
        for (size_t i = 0; i < k; ++i)
            bits.set(hasher.hash_i(item, i, m));
    }

    bool contains(const string &item) const {
        for (size_t i = 0; i < k; ++i)
            if (!bits.get(hasher.hash_i(item, i, m)))
                return false;
        return true;
    }

    size_t bit_size() const { return m; }

    // Approximate false-positive rate: f = (1 - exp(-kn/m))^k
    double approx_fpr(size_t n) const {
        if (m == 0 || n == 0) return 0.0;
        double lam = (double)k * n / m;
        double p0 = exp(-lam);
        return pow(1.0 - p0, (double)k);
    }
};

/*-----------------------------------------------------------
 * Compound Bloom Filter (CBF)
 *-----------------------------------------------------------*/
class CompoundBloomFilter {
private:
    vector<BloomFilter> filters;

public:
    CompoundBloomFilter(size_t count, size_t bits_per_filter, size_t k_hashes) {
        std::mt19937_64 rng(999999ULL); // deterministic salt generation
        filters.reserve(count);
        for (size_t i = 0; i < count; ++i)
            filters.emplace_back(bits_per_filter, k_hashes, rng());
    }

    void insert(const string &item) {
        for (auto &f : filters) f.insert(item);
    }

    bool contains(const string &item) const {
        for (const auto &f : filters)
            if (!f.contains(item)) return false;
        return true;
    }

    size_t total_bits() const {
        size_t sum = 0;
        for (const auto &f : filters)
            sum += f.bit_size();
        return sum;
    }
};

/*-----------------------------------------------------------
 * HR-CBF: Hierarchical Multi-Level Bloom Filter
 *-----------------------------------------------------------*/
class HR_CBF {
private:
    vector<BloomFilter> levels;
    vector<size_t> m_bits;
    vector<size_t> k_hashes;
    size_t L;

public:
    HR_CBF(size_t total_bits, size_t level_count, size_t expected_n, double alpha = 2.0)
        : L(level_count)
    {
        if (L == 0) L = 1;

        m_bits.assign(L, 0);
        k_hashes.assign(L, 1);

        /*----- Geometric Bit Allocation: m_i = m0 * alpha^i -----*/
        if (alpha <= 1.0) {
            size_t base = total_bits / L;
            for (size_t i = 0; i < L; ++i) m_bits[i] = base;

            size_t rem = total_bits - base * L;
            for (size_t i = 0; i < rem; ++i)
                m_bits[i % L]++;
        }
        else {
            double apL = pow(alpha, (double)L);
            double m0 = (double)total_bits * (alpha - 1.0) / (apL - 1.0);

            size_t assigned = 0;
            for (size_t i = 0; i < L; ++i) {
                size_t mi = (size_t)floor(m0 * pow(alpha, i) + 0.5);
                mi = max(mi, (size_t)16);  // safety: minimal bit size
                m_bits[i] = mi;
                assigned += mi;
            }

            // Adjust rounding errors
            if (assigned < total_bits) {
                size_t rem = total_bits - assigned;
                for (size_t i = L; rem > 0 && i-- > 0;)
                    m_bits[i]++, rem--;
            }
        }

        /*----- Optimal Per-Level k -----*/
        const double ln2 = log(2.0);
        for (size_t i = 0; i < L; ++i) {
            if (expected_n == 0) k_hashes[i] = 3;
            else {
                double k_est = ((double)m_bits[i] / expected_n) * ln2;
                k_hashes[i] = max((size_t)1, (size_t)floor(k_est + 0.5));
            }
        }

        /*----- Construct Levels -----*/
        std::mt19937_64 rng(1234567ULL);
        for (size_t i = 0; i < L; ++i)
            levels.emplace_back(m_bits[i], k_hashes[i], rng());
    }

    void insert(const string &item) {
        for (auto &lvl : levels) lvl.insert(item);
    }

    bool contains(const string &item) const {
        for (const auto &lvl : levels)
            if (!lvl.contains(item))
                return false;
        return true;
    }

    size_t total_bits() const {
        return accumulate(m_bits.begin(), m_bits.end(), (size_t)0);
    }

    double approx_fpr(size_t n) const {
        double prod = 1.0;
        for (const auto &lvl : levels)
            prod *= lvl.approx_fpr(n);
        return prod;
    }

    const vector<size_t>& get_m_bits() const { return m_bits; }
    const vector<size_t>& get_k_hashes() const { return k_hashes; }
};

/*-----------------------------------------------------------
 * Utility: Random String Generator
 *-----------------------------------------------------------*/
string random_string(size_t len, mt19937_64 &rng) {
    static const char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    uniform_int_distribution<int> dist(0, sizeof(alphabet) - 2);

    string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i)
        s.push_back(alphabet[dist(rng)]);
    return s;
}

/*-----------------------------------------------------------
 * Experiment Runner
 *-----------------------------------------------------------*/
void run_experiment() {
    cout << "=== Bloom Filter Experiments ===\n\n";

    const size_t total_bits = 1 << 20;   // 1M bits
    const size_t k_simple = 7;
    const size_t n_insert = 200000;
    const size_t n_test = 200000;

    mt19937_64 rng(2025ULL);

    /*----- Generate Data -----*/
    vector<string> inserted(n_insert);
    for (auto &s : inserted) s = random_string(16, rng);

    unordered_set<string> inserted_set(inserted.begin(), inserted.end());

    /*----- Single BF -----*/
    BloomFilter bf_single(total_bits, k_simple, 111111ULL);
    for (auto &s : inserted) bf_single.insert(s);

    /*----- 2-Filter CBF (equal total bits) -----*/
    CompoundBloomFilter cbf_same(2, total_bits / 2, k_simple);
    for (auto &s : inserted) cbf_same.insert(s);

    /*----- 2-Filter CBF2 (double memory) -----*/
    CompoundBloomFilter cbf_double(2, total_bits, k_simple);
    for (auto &s : inserted) cbf_double.insert(s);

    /*----- HR-CBF -----*/
    HR_CBF hr(total_bits, 3, n_insert, 2.0);
    for (auto &s : inserted) hr.insert(s);

    /*----- Evaluate -----*/
    size_t fp_single = 0, fp_cbf = 0, fp_cbf2 = 0, fp_hr = 0;

    for (size_t i = 0; i < n_test; ++i) {
        string q;
        do q = random_string(16, rng);
        while (inserted_set.count(q));

        fp_single += bf_single.contains(q);
        fp_cbf   += cbf_same.contains(q);
        fp_cbf2  += cbf_double.contains(q);
        fp_hr    += hr.contains(q);
    }

    cout << fixed << setprecision(4);

    cout << "\n--- HR-CBF Configuration ---\n";
    auto m_vec = hr.get_m_bits();
    auto k_vec = hr.get_k_hashes();
    for (size_t i = 0; i < m_vec.size(); ++i)
        cout << " Level " << i << ": m=" << m_vec[i]
             << ", k=" << k_vec[i] << "\n";
    cout << " Approx HR-CBF FPR (analytic): "
         << hr.approx_fpr(n_insert) * 100 << "%\n\n";

    cout << "--- Results (Measured FPR) ---\n";
    cout << " Single BF     : " << (fp_single * 100.0 / n_test) << "%\n";
    cout << " CBF (same)    : " << (fp_cbf * 100.0 / n_test)    << "%\n";
    cout << " CBF (double)  : " << (fp_cbf2 * 100.0 / n_test)   << "%\n";
    cout << " HR-CBF        : " << (fp_hr * 100.0 / n_test)     << "%\n\n";

    cout << "--- Memory Usage ---\n";
    cout << " Single BF : " << bf_single.bit_size()     << " bits\n";
    cout << " CBF       : " << cbf_same.total_bits()    << " bits\n";
    cout << " CBF2      : " << cbf_double.total_bits()  << " bits\n";
    cout << " HR-CBF    : " << hr.total_bits()          << " bits\n";
}

/*-----------------------------------------------------------*/
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    run_experiment();
    return 0;
}
