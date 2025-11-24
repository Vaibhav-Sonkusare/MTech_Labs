// Hierarchical / Multi-Resolution Compound Bloom Filter (HR-CBF) added
//
// - Keeps your original BloomFilter and CompoundBloomFilter implementations.
// - Adds HR_CBF: hierarchy of BloomFilter levels with automatic geometric sizing
//   and per-level k tuning (k ≈ (m_i / n) * ln(2)).
// - Integrates HR_CBF into the experiment runner for comparison.
//
// Compile: g++ -O2 -std=c++17 this_file.cpp -o hr_cbf_demo

#include <bits/stdc++.h>
using namespace std;
using u64 = unsigned long long;
using u32 = unsigned int;

// splitmix64 for mixing hashes (deterministic)
static inline u64 splitmix64(u64 x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// double hashing: produce k indices from two base hashes
// We'll use std::hash<T> to get a base h1 and then splitmix to get h2-like.
// For variety per-filter we also include a per-filter salt.
struct Hasher {
    u64 salt;
    Hasher(u64 s = 0) : salt(s) {}
    // take arbitrary data (string) -> return pair of 64-bit hashes (h1, h2)
    pair<u64,u64> base_hashes(const string &data) const {
        // h1 from std::hash combined with salt
        u64 h1 = std::hash<string>{}(data) ^ salt;
        // h2 from splitmix64 of h1 + salt
        u64 h2 = splitmix64(h1 + 0x9e3779b97f4a7c15ULL + salt);
        // ensure non-zero h2
        if (h2 == 0) h2 = 0x9e3779b97f4a7c15ULL;
        return {h1, h2};
    }
    // generate i-th hash in [0, m-1]
    u64 hash_i(const string &data, size_t i, u64 m) const {
        auto [h1, h2] = base_hashes(data);
        u64 combined = h1 + i * h2;
        // reduce mod m safely
        return combined % m;
    }
};

// simple bitset stored in vector<uint8_t> (1 byte per bit is simple but we pack into bytes)
class BitArray {
    vector<uint8_t> bytes;
    size_t bits;
public:
    BitArray(size_t bits_ = 0) { reset(bits_); }
    void reset(size_t bits_) {
        bits = bits_;
        bytes.assign((bits + 7) / 8, 0);
    }
    inline void set(size_t idx) {
        bytes[idx >> 3] |= (1u << (idx & 7));
    }
    inline bool get(size_t idx) const {
        return (bytes[idx >> 3] >> (idx & 7)) & 1u;
    }
    size_t size_bits() const { return bits; }
    size_t size_bytes() const { return bytes.size(); }
};

// Classic Bloom Filter
class BloomFilter {
    size_t m;           // number of bits
    size_t k;           // number of hash functions
    BitArray bits;
    Hasher hasher;
public:
    // m_bits: number of bits; k_hashes: number of hash functions; salt: per-filter salt
    BloomFilter(size_t m_bits = 1024, size_t k_hashes = 3, u64 salt = 0)
        : m(m_bits), k(k_hashes), bits(m_bits), hasher(salt) {}

    void insert(const string &item) {
        auto &h = hasher;
        for (size_t i = 0; i < k; ++i) {
            u64 idx = h.hash_i(item, i, m);
            bits.set(idx);
        }
    }
    bool contains(const string &item) const {
        auto &h = hasher;
        for (size_t i = 0; i < k; ++i) {
            u64 idx = h.hash_i(item, i, m);
            if (!bits.get(idx)) return false;
        }
        return true;
    }
    size_t bit_size() const { return m; }
    size_t hash_count() const { return k; }

    // Diagnostics: approximate single-level false-positive probability given n inserted items.
    // f = (1 - exp(-k*n/m))^k
    double approx_fpr(size_t n) const {
        if (m == 0 || n == 0) return 0.0;
        double lam = (double)k * (double)n / (double)m;
        double p0 = exp(-lam); // prob a bit is zero
        double fi = pow(1.0 - p0, (double)k);
        return fi;
    }
};

// Compound Bloom Filter: holds multiple BloomFilter instances.
// Insert: insert into every filter. Query: return true only if all filters return true.
class CompoundBloomFilter {
    vector<BloomFilter> filters;
public:
    // Create 'count' filters each with m_bits_per and k_hashes
    CompoundBloomFilter(size_t count, size_t m_bits_per, size_t k_hashes) {
        // create independent salts for each filter
        std::mt19937_64 rng(123456ULL);
        for (size_t i = 0; i < count; ++i) {
            u64 salt = rng();
            filters.emplace_back(m_bits_per, k_hashes, salt);
        }
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
        size_t s = 0;
        for (auto &f : filters) s += f.bit_size();
        return s;
    }

    size_t filter_count() const { return filters.size(); }
    size_t hash_count_each() const { return filters.empty() ? 0 : filters[0].hash_count(); }
};

// -----------------------------
// HR_CBF: Hierarchical / Multi-Resolution CBF
// -----------------------------
class HR_CBF {
    // levels[0] = smallest/coarsest, levels[L-1] = largest/finest
    vector<BloomFilter> levels;
    vector<size_t> m_bits;
    vector<size_t> k_hashes;
    size_t L;

public:
    // Construct HR_CBF by specifying total bits budget, number of levels L,
    // expected number of inserts 'expected_n', and the geometric ratio alpha (>1).
    // If alpha <= 1.0, fallback to equal sizing.
    HR_CBF(size_t total_bits, size_t levels_count, size_t expected_n, double alpha = 2.0) {
        if (levels_count == 0) levels_count = 1;
        L = levels_count;
        m_bits.assign(L, 0);
        k_hashes.assign(L, 1);

        // Compute geometric sizes m_i = m0 * alpha^i so that sum m_i = total_bits
        if (alpha <= 1.0) {
            // equal sizes
            size_t base = total_bits / L;
            for (size_t i = 0; i < L; ++i) m_bits[i] = base;
            // distribute remainder
            size_t rem = total_bits - base * L;
            for (size_t i = 0; i < rem; ++i) ++m_bits[i % L];
        } else {
            // m0 * (alpha^L - 1) / (alpha - 1) = total_bits => m0 = total_bits*(alpha-1)/(alpha^L -1)
            double alpha_pow_L = pow(alpha, (double)L);
            double m0_d = (double)total_bits * (alpha - 1.0) / (alpha_pow_L - 1.0);
            // round and ensure at least 8 bits per level
            size_t sum_assigned = 0;
            for (size_t i = 0; i < L; ++i) {
                double mi = m0_d * pow(alpha, (double)i);
                size_t mi_sz = (size_t)max(16.0, floor(mi + 0.5)); // min 16 bits to avoid tiny filters
                m_bits[i] = mi_sz;
                sum_assigned += mi_sz;
            }
            // adjust rounding differences
            if (sum_assigned != total_bits) {
                if (sum_assigned < total_bits) {
                    size_t rem = total_bits - sum_assigned;
                    // add to largest levels first
                    for (size_t i = L; rem > 0 && i-- > 0; ) {
                        size_t add = min(rem, (size_t)1);
                        m_bits[i] += add; rem -= add;
                    }
                } else {
                    size_t over = sum_assigned - total_bits;
                    for (size_t i = L; over > 0 && i-- > 0; ) {
                        size_t sub = min(over, m_bits[i] > 16 ? (size_t)1 : (size_t)0);
                        m_bits[i] -= sub; over -= sub;
                    }
                }
            }
        }

        // compute per-level k using k ≈ (m_i / n) * ln(2)
        const double ln2 = log(2.0);
        for (size_t i = 0; i < L; ++i) {
            if (expected_n == 0) {
                k_hashes[i] = 3; // default
            } else {
                double k_opt = ((double)m_bits[i] / (double)expected_n) * ln2;
                size_t k_round = (size_t)max(1.0, floor(k_opt + 0.5));
                k_hashes[i] = k_round;
            }
        }

        // build BloomFilter levels with independent salts
        std::mt19937_64 rng(123456ULL);
        levels.reserve(L);
        for (size_t i = 0; i < L; ++i) {
            u64 salt = rng();
            levels.emplace_back(m_bits[i], k_hashes[i], salt);
        }
    }

    // Alternative constructor that accepts explicit m_bits and k_hashes arrays
    HR_CBF(const vector<size_t>& m_bits_in, const vector<size_t>& k_hashes_in) {
        L = m_bits_in.size();
        m_bits = m_bits_in;
        k_hashes = k_hashes_in;
        std::mt19937_64 rng(123456ULL);
        for (size_t i = 0; i < L; ++i) {
            u64 salt = rng();
            levels.emplace_back(m_bits[i], k_hashes[i], salt);
        }
    }

    void insert(const string &item) {
        // Insert into all levels (write-all)
        for (size_t i = 0; i < L; ++i) levels[i].insert(item);
    }

    bool contains(const string &item) const {
        // Cascaded early-stop: check from coarse (0) -> fine (L-1)
        for (size_t i = 0; i < L; ++i) {
            if (!levels[i].contains(item)) return false;
        }
        return true;
    }

    size_t total_bits() const {
        size_t s = 0;
        for (auto b : m_bits) s += b;
        return s;
    }

    size_t level_count() const { return L; }

    // Approximate hierarchical FPR using independence assumption:
    // FPR ≈ product_i f_i where f_i is approx single-level FPR
    double approx_fpr(size_t n_inserted) const {
        double prod = 1.0;
        for (size_t i = 0; i < L; ++i) {
            double fi = levels[i].approx_fpr(n_inserted);
            prod *= fi;
        }
        return prod;
    }

    // For diagnostics: return m_bits and k_hashes
    vector<size_t> get_m_bits() const { return m_bits; }
    vector<size_t> get_k_hashes() const { return k_hashes; }
};

// Utility: generate random strings (for testing)
string random_string(size_t len, mt19937_64 &rng) {
    static const char alphabet[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    uniform_int_distribution<int> dist(0, (int)sizeof(alphabet)-2);
    string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) s.push_back(alphabet[dist(rng)]);
    return s;
}

// Experiment: compare FPR of a single BloomFilter vs a CompoundBloomFilter of 2 filters.
// We keep total memory equal. Also compare HR_CBF variant.
void run_experiment() {
    cout << "Running experiment comparing single BF vs 2-filter CBF vs HR_CBF\n\n";

    const size_t total_bits = 1 << 20; // 1M bits (~128 KB) — adjustable
    const size_t k = 7;                // number of hash functions per (simple) filter
    const size_t n_insert = 200000;    // number of elements to insert
    const size_t n_test = 200000;      // number of negative tests (to estimate FPR)

    // Prepare RNG and data
    mt19937_64 rng(123456ULL);
    vector<string> inserted;
    inserted.reserve(n_insert);
    for (size_t i = 0; i < n_insert; ++i) inserted.push_back(random_string(16, rng));

    // Single filter: all bits used in one filter
    BloomFilter single(total_bits, k, 0xaaaaffff1234ULL);
    for (auto &s : inserted) single.insert(s);

    // Compound: 2 filters each half the bits (equal total bits)
    size_t filters_count = 2;
    size_t per_filter_bits = total_bits / filters_count;
    CompoundBloomFilter cbf(filters_count, per_filter_bits, k);
    for (auto &s : inserted) cbf.insert(s);

    // Compound: 2 filters each of 'total_bits' bits (equal 2 * total bits)
    size_t filters_count2 = 2;
    size_t per_filter_bits2 = total_bits;
    CompoundBloomFilter cbf2(filters_count2, per_filter_bits2, k);
    for (auto &s : inserted) cbf2.insert(s);

    // HR_CBF: hierarchical with L levels, auto-tuned using expected_n = n_insert
    size_t L = 3;
    double alpha = 2.0; // geometric factor
    HR_CBF hr_cbf(total_bits, L, n_insert, alpha);
    for (auto &s : inserted) hr_cbf.insert(s);

    // Print configuration summary for HR_CBF
    cout << "HR_CBF configuration:\n";
    auto m_bits = hr_cbf.get_m_bits();
    auto k_hashes = hr_cbf.get_k_hashes();
    for (size_t i = 0; i < m_bits.size(); ++i) {
        cout << " level " << i << ": m = " << m_bits[i]
             << " bits, k = " << k_hashes[i] << "\n";
    }
    cout << " approx hierarchical FPR (analytic) = "
         << (100.0 * hr_cbf.approx_fpr(n_insert)) << " %\n\n";

    // Test false positive rate using random strings that are not in inserted set
    size_t false_pos_single = 0;
    size_t false_pos_cbf = 0;
    size_t false_pos_cbf2 = 0;
    size_t false_pos_hr = 0;

    // To reduce chance of accidental collision with inserted set, generate strings until not in inserted
    unordered_set<string> inserted_set(inserted.begin(), inserted.end());

    for (size_t i = 0; i < n_test; ++i) {
        string q;
        do {
            q = random_string(16, rng);
        } while (inserted_set.count(q));
        if (single.contains(q)) false_pos_single++;
        if (cbf.contains(q)) false_pos_cbf++;
        if (cbf2.contains(q)) false_pos_cbf2++;
        if (hr_cbf.contains(q)) false_pos_hr++;
    }

    double fpr_single = 100.0 * false_pos_single / double(n_test);
    double fpr_cbf = 100.0 * false_pos_cbf / double(n_test);
    double fpr_cbf2 = 100.0 * false_pos_cbf2 / double(n_test);
    double fpr_hr = 100.0 * false_pos_hr / double(n_test);

    cout << "Parameters:\n";
    cout << "  total bits = " << total_bits << "\n";
    cout << "  simple filter k = " << k << "\n";
    cout << "  inserted (n) = " << n_insert << "\n";
    cout << "  negative tests = " << n_test << "\n\n";

    cout << "Results (measured):\n";
    cout << "  Single BF false positives: " << false_pos_single << " / " << n_test
         << "  => FPR = " << fpr_single << " %\n";
    cout << "  2-filter CBF false positives (same total bits): " << false_pos_cbf << " / " << n_test
         << "  => FPR = " << fpr_cbf << " %\n";
    cout << "  2-filter CBF2 false positives (each filter = total_bits): " << false_pos_cbf2 << " / " << n_test
         << "  => FPR = " << fpr_cbf2 << " %\n";
    cout << "  HR_CBF false positives: " << false_pos_hr << " / " << n_test
         << "  => FPR = " << fpr_hr << " %\n\n";

    cout << "Memory used (approx):\n";
    cout << "  Single BF bits: " << single.bit_size() << " bits\n";
    cout << "  CBF total bits: " << cbf.total_bits() << " bits\n";
    cout << "  CBF2 total bits: " << cbf2.total_bits() << " bits\n";
    cout << "  HR_CBF total bits: " << hr_cbf.total_bits() << " bits\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "Compound Bloom Filter (CBF) + HR_CBF demo\n";
    cout << "----------------------------------------\n\n";

    run_experiment();

    cout << "\nDone.\n";
    return 0;
}
