#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

struct StatResult {
    // đơn vị: microseconds
    long long insertTime;
    long long searchTime;
    long long deleteTime;
};

enum SlotState { EMPTY, OCCUPIED, DELETED };

template<typename K, typename V>
struct Entry {
    K key;
    V value;
    SlotState state;
    Entry() : state(EMPTY) {}
};

bool isPrime(int n);
int nextPrime(int n);
std::string doubleToStr(double x, int precision = 2);
void printSummaryTable(
    double lf1, double lf2,
    StatResult dht1, StatResult dht2,
    StatResult lpt1, StatResult lpt2,
    StatResult qpt1, StatResult qpt2
);
void printDetailHeader(void);

// Hàm kiểm tra số nguyên tố
bool isPrime(int n) {
    if (n < 2) {
        return false;
    }
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}
// Tìm số nguyên tố lớn hơn n
int nextPrime(int n) {
    int x = n + 1;
    while (!isPrime(x)) {
        ++x;
    }
    return x;
}
// Chuyển từ double sang string với precision tùy chỉnh
std::string doubleToStr(double x, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << x;
    return oss.str();
}

// ======= Double Hashing Table =======
template<typename K, typename V>
class DoubleHashTable {
    int TABLE_SIZE;
    int keysPresent;
    int PRIME;
    std::vector<Entry<K, V>> hashTable;
    std::vector<bool> isPrimeArr;
public:
    // Thống kê như cũ...
    int totalProbesInsert = 0, totalProbesSearch = 0, totalProbesDelete = 0;
    int totalCollision = 0, nInsert = 0, nSearch = 0, nDelete = 0;

    DoubleHashTable(int n) {
        TABLE_SIZE = n;
        isPrimeArr.assign(n, true);
        for (int i = 2; i * i < n; ++i)
            if (isPrimeArr[i])
                for (int j = i * i; j < n; j += i)
                    isPrimeArr[j] = false;
        PRIME = TABLE_SIZE - 1;
        while (PRIME > 1 && !isPrimeArr[PRIME]) PRIME--;
        keysPresent = 0;
        hashTable.assign(TABLE_SIZE, Entry<K, V>());
    }
    int hash1(const K& key) { return std::hash<K>{}(key) % TABLE_SIZE; }
    int hash2(const K& key) { return PRIME - (std::hash<K>{}(key) % PRIME); }
    bool isFull() { return keysPresent == TABLE_SIZE; }

    bool insert(const K& key, const V& value) {
        if (isFull()) return false;
        int probe = hash1(key), offset = hash2(key), probes = 1;
        if (hashTable[probe].state == OCCUPIED) totalCollision++;
        while (hashTable[probe].state == OCCUPIED && hashTable[probe].key != key) {
            probe = (probe + offset) % TABLE_SIZE;
            probes++;
        }
        if (hashTable[probe].state != OCCUPIED) {
            hashTable[probe].key = key;
            hashTable[probe].value = value;
            hashTable[probe].state = OCCUPIED;
            keysPresent++;
            totalProbesInsert += probes;
            nInsert++;
            return true;
        }
        else if (hashTable[probe].key == key) { // Update value
            hashTable[probe].value = value;
            return true;
        }
        return false;
    }

    bool search(const K& key, V& outValue) {
        int probe = hash1(key), offset = hash2(key), initialPos = probe, probes = 1;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) break;
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                outValue = hashTable[probe].value;
                totalProbesSearch += probes; nSearch++;
                return true;
            }
            if (probe == initialPos && !firstItr) break;
            probe = (probe + offset) % TABLE_SIZE;
            probes++; firstItr = false;
        }
        totalProbesSearch += probes; nSearch++;
        return false;
    }

    void erase(const K& key) {
        int probe = hash1(key), offset = hash2(key), initialPos = probe, probes = 1;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) { totalProbesDelete += probes; nDelete++; return; }
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                hashTable[probe].state = DELETED; keysPresent--;
                totalProbesDelete += probes; nDelete++; return;
            }
            if (probe == initialPos && !firstItr) { totalProbesDelete += probes; nDelete++; return; }
            probe = (probe + offset) % TABLE_SIZE;
            probes++; firstItr = false;
        }
    }
    void print() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << (hashTable[i] == -1 ? "Empty" : std::to_string(hashTable[i])) << ", ";
        }
        std::cout << "\n";
    }
};

// ======= Linear Probing Table =======
template<typename K, typename V>
class LinearProbingTable {
    int TABLE_SIZE;
    int keysPresent;
    std::vector<Entry<K, V>> hashTable;
public:
    // Các biến thống kê như cũ
    int totalProbesInsert = 0, totalProbesSearch = 0, totalProbesDelete = 0;
    int totalCollision = 0, nInsert = 0, nSearch = 0, nDelete = 0;

    LinearProbingTable(int n) : TABLE_SIZE(n), keysPresent(0), hashTable(n, Entry<K, V>()) {}

    int hash(const K& key) { return std::hash<K>{}(key) % TABLE_SIZE; }
    bool isFull() { return keysPresent == TABLE_SIZE; }

    bool insert(const K& key, const V& value) {
        if (isFull()) return false;
        int probe = hash(key), probes = 1;
        if (hashTable[probe].state == OCCUPIED) totalCollision++;
        while (hashTable[probe].state == OCCUPIED && hashTable[probe].key != key) {
            probe = (probe + 1) % TABLE_SIZE; probes++;
        }
        if (hashTable[probe].state != OCCUPIED) {
            hashTable[probe].key = key;
            hashTable[probe].value = value;
            hashTable[probe].state = OCCUPIED;
            keysPresent++;
            totalProbesInsert += probes;
            nInsert++;
            return true;
        }
        else if (hashTable[probe].key == key) { // Update value
            hashTable[probe].value = value;
            return true;
        }
        return false;
    }

    bool search(const K& key, V& outValue) {
        int probe = hash(key), initialPos = probe, probes = 1;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) break;
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                outValue = hashTable[probe].value;
                totalProbesSearch += probes; nSearch++;
                return true;
            }
            if (probe == initialPos && !firstItr) break;
            probe = (probe + 1) % TABLE_SIZE; probes++; firstItr = false;
        }
        totalProbesSearch += probes; nSearch++;
        return false;
    }

    void erase(const K& key) {
        int probe = hash(key), initialPos = probe, probes = 1;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) { totalProbesDelete += probes; nDelete++; return; }
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                hashTable[probe].state = DELETED; keysPresent--;
                totalProbesDelete += probes; nDelete++; return;
            }
            if (probe == initialPos && !firstItr) { totalProbesDelete += probes; nDelete++; return; }
            probe = (probe + 1) % TABLE_SIZE; probes++; firstItr = false;
        }
    }
    void print() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << (hashTable[i] == -1 ? "Empty" : std::to_string(hashTable[i])) << ", ";
        }
        std::cout << "\n";
    }
};


template<typename K, typename V>
class QuadraticProbingTable {
    int TABLE_SIZE;
    int keysPresent;
    std::vector<Entry<K, V>> hashTable;
public:
    int totalProbesInsert = 0, totalProbesSearch = 0, totalProbesDelete = 0;
    int totalCollision = 0, nInsert = 0, nSearch = 0, nDelete = 0;

    QuadraticProbingTable(int n) : TABLE_SIZE(n), keysPresent(0), hashTable(n, Entry<K, V>()) {}

    int hash(const K& key) { return std::hash<K>{}(key) % TABLE_SIZE; }
    bool isFull() { return keysPresent == TABLE_SIZE; }

    bool insert(const K& key, const V& value) {
        if (isFull()) return false;
        int probe = hash(key), probes = 1, i = 0;
        if (hashTable[probe].state == OCCUPIED) totalCollision++;
        while (hashTable[probe].state == OCCUPIED && hashTable[probe].key != key) {
            ++i;
            probe = (hash(key) + i * i) % TABLE_SIZE;
            probes++;
        }
        if (hashTable[probe].state != OCCUPIED) {
            hashTable[probe].key = key;
            hashTable[probe].value = value;
            hashTable[probe].state = OCCUPIED;
            keysPresent++;
            totalProbesInsert += probes;
            nInsert++;
            return true;
        }
        else if (hashTable[probe].key == key) {
            hashTable[probe].value = value;
            return true;
        }
        return false;
    }

    bool search(const K& key, V& outValue) {
        int probe = hash(key), initialPos = probe, probes = 1, i = 0;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) break;
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                outValue = hashTable[probe].value;
                totalProbesSearch += probes; nSearch++;
                return true;
            }
            ++i;
            probe = (hash(key) + i * i) % TABLE_SIZE;
            probes++; firstItr = false;
            if (probe == initialPos && !firstItr) break;
        }
        totalProbesSearch += probes; nSearch++;
        return false;
    }

    void erase(const K& key) {
        int probe = hash(key), initialPos = probe, probes = 1, i = 0;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) { totalProbesDelete += probes; nDelete++; return; }
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                hashTable[probe].state = DELETED; keysPresent--;
                totalProbesDelete += probes; nDelete++; return;
            }
            ++i;
            probe = (hash(key) + i * i) % TABLE_SIZE;
            probes++; firstItr = false;
            if (probe == initialPos && !firstItr) { totalProbesDelete += probes; nDelete++; return; }
        }
    }
    void print() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << (hashTable[i] == -1 ? "Empty" : std::to_string(hashTable[i])) << ", ";
        }
        std::cout << "\n";
    }
};

// Hàm tính thời gian thực hiện các thao tác trên bảng băm
template <typename Table>
StatResult testTable(Table& table, const std::vector<std::pair<int, int>>& keyvals, const std::vector<int>& search_indices, const std::vector<int>& delete_indices) {
    StatResult res;
    auto t1 = std::chrono::steady_clock::now();
    // insert
    for (auto& kv : keyvals) table.insert(kv.first, kv.second);
    auto t2 = std::chrono::steady_clock::now();
    // search
    int tmp;
    for (int idx : search_indices) {
        table.search(keyvals[idx].first, tmp); // cần 1 biến value out
    }
    auto t3 = std::chrono::steady_clock::now();
    // delete
    for (int idx : delete_indices) {
        table.erase(keyvals[idx].first);
    }
    auto t4 = std::chrono::steady_clock::now();
    res.insertTime = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    res.searchTime = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
    res.deleteTime = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
    return res;
}

// Hàm in bảng thống kê tổng hợp thời gian thực hiện các thao tác
void printSummaryTable(
    double lf1, double lf2,
    StatResult dht1, StatResult dht2,
    StatResult lpt1, StatResult lpt2,
    StatResult qpt1, StatResult qpt2
) {
    std::cout << "\n===== TABLE OF PERFORMANCE COMPARISON (us) =====\n";
    std::cout << std::setw(30) << std::left << " "
        << std::setw(20) << "Double Hashing"
        << std::setw(20) << "Linear Probing"
        << std::setw(20) << "Quadratic Probing" << '\n';

    std::cout << std::setw(30) << std::left << ("[Insert] LF1 (" + doubleToStr(lf1, 2) + "):")
        << std::setw(20) << dht1.insertTime
        << std::setw(20) << lpt1.insertTime
        << std::setw(20) << qpt1.insertTime << '\n';

    std::cout << std::setw(30) << std::left << ("[Insert] LF2 (" + doubleToStr(lf2, 2) + "):")
        << std::setw(20) << dht2.insertTime
        << std::setw(20) << lpt2.insertTime
        << std::setw(20) << qpt2.insertTime << '\n';

    std::cout << std::setw(30) << std::left << ("[Search] LF1 (" + doubleToStr(lf1, 2) + "):")
        << std::setw(20) << dht1.searchTime
        << std::setw(20) << lpt1.searchTime
        << std::setw(20) << qpt1.searchTime << '\n';

    std::cout << std::setw(30) << std::left << ("[Search] LF2 (" + doubleToStr(lf2, 2) + "):")
        << std::setw(20) << dht2.searchTime
        << std::setw(20) << lpt2.searchTime
        << std::setw(20) << qpt2.searchTime << '\n';

    std::cout << std::setw(30) << std::left << ("[Delete] LF1 (" + doubleToStr(lf1, 2) + "):")
        << std::setw(20) << dht1.deleteTime
        << std::setw(20) << lpt1.deleteTime
        << std::setw(20) << qpt1.deleteTime << '\n';

    std::cout << std::setw(30) << std::left << ("[Delete] LF2 (" + doubleToStr(lf2, 2) + "):")
        << std::setw(20) << dht2.deleteTime
        << std::setw(20) << lpt2.deleteTime
        << std::setw(20) << qpt2.deleteTime << '\n';
}
// Hàm in bảng thống kê chi tiết về số lần probe, số lần va chạm và tỷ lệ va chạm
template <typename Table>
void printDetailStats(const std::string& algoName, double lf, Table& table) {
    std::cout << std::left << std::setw(20) << algoName
        << std::setw(12) << std::fixed << std::setprecision(2) << lf
        << std::setw(10) << table.nInsert
        << std::setw(12) << std::fixed << std::setprecision(4) << (table.nInsert ? 1.0 * table.totalProbesInsert / table.nInsert : 0)
        << std::setw(10) << table.totalCollision
        << std::setw(12) << std::fixed << std::setprecision(4) << (table.nInsert ? 100.0 * table.totalCollision / table.nInsert : 0)
        << std::setw(10) << table.nSearch
        << std::setw(12) << std::fixed << std::setprecision(4) << (table.nSearch ? 1.0 * table.totalProbesSearch / table.nSearch : 0)
        << std::setw(10) << table.nDelete
        << std::setw(12) << std::fixed << std::setprecision(4) << (table.nDelete ? 1.0 * table.totalProbesDelete / table.nDelete : 0)
        << '\n';
}
// Hàm in tiêu đề cho bảng thống kê chi tiết
void printDetailHeader(void) {
    std::cout << std::left << std::setw(20) << "Algorithm"
        << std::setw(12) << "LoadFac"
        << std::setw(10) << "Insert"
        << std::setw(12) << "Probe/Ins"
        << std::setw(10) << "Coll"
        << std::setw(12) << "CollRate"
        << std::setw(10) << "Search"
        << std::setw(12) << "Probe/S"
        << std::setw(10) << "Delete"
        << std::setw(12) << "Probe/D"
        << '\n';
    std::cout << std::string(120, '-') << '\n';
}

int main(void) {
    system("color F0");
    int M;
    std::cout << "Enter the number of elements to test: ";
    std::cin >> M;

    // Nhập load factor thứ nhất (do người dùng nhập)
    double lf1;
    std::cout << "Enter the first load factor to compare (e.g., 0.7): ";
    std::cin >> lf1;

    // Load factor tối ưu (do chương trình chọn, ví dụ 0.5)
    double lf2 = 0.5; // toi uu cho linear
    std::cout << "Auto-selected optimal load factor: " << lf2 << '\n';

    // Tính số slot cho mỗi bảng băm
    int N1 = nextPrime(int(M / lf1));
    int N2 = nextPrime(int(M / lf2));

    std::cout << "TABLE_SIZE with load factor 1 (" << lf1 << "): " << N1 << '\n';
    std::cout << "TABLE_SIZE with load factor 2 (" << lf2 << "): " << N2 << '\n';

    // Đặt số lần search/delete
    int num_search, num_delete;
    do {
        std::cout << "Enter the number of searches (MAX " << M << "): ";
        std::cin >> num_search;
        if (num_search > M) {
            std::cout << "Too many searches! Limiting to " << M << ".\n";
            num_search = M;
        }
    } while (num_search < 0);

    do {
        std::cout << "Enter the number of deletes (MAX " << M << "): ";
        std::cin >> num_delete;
        if (num_delete > M) {
            std::cout << "Too many deletes! Limiting to " << M << ".\n";
            num_delete = M;
        }
    } while (num_delete < 0);

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist_key(1, std::max(N1, N2) * 10);    // key random
    std::uniform_int_distribution<int> dist_val(1, 1000000);                  // value random

    // Sinh M cặp key-value ngẫu nhiên không trùng key
    std::vector<std::pair<int, int>> keyvals;
    while ((int)keyvals.size() < M) {
        int key = dist_key(rng);
        int val = dist_val(rng);
        bool dup = false;
        for (auto& kv : keyvals)
            if (kv.first == key) { dup = true; break; }
        if (!dup) keyvals.emplace_back(key, val);
    }
    std::cout << "=> Successfully generated test values <=\n";

    // Sinh chỉ số ngẫu nhiên cho search/delete
    std::vector<int> search_indices, delete_indices, all_indices(M);
    for (int i = 0; i < M; ++i) all_indices[i] = i;
    std::shuffle(all_indices.begin(), all_indices.end(), rng);
    for (int i = 0; i < num_search; ++i) search_indices.push_back(all_indices[i]);
    std::shuffle(all_indices.begin(), all_indices.end(), rng);
    for (int i = 0; i < num_delete; ++i) delete_indices.push_back(all_indices[i]);

    // ---- CẤU HÌNH 1: Load factor do người dùng nhập ----
    DoubleHashTable<int, int> dht1(N1);
    LinearProbingTable<int, int> lpt1(N1);
    QuadraticProbingTable<int, int> qpt1(N1);

    // ---- CẤU HÌNH 2: Load factor tối ưu (ví dụ 0.5) ----
    DoubleHashTable<int, int> dht2(N2);
    LinearProbingTable<int, int> lpt2(N2);
    QuadraticProbingTable<int, int> qpt2(N2);

    // Test và lưu thời gian
    auto dht1_stat = testTable(dht1, keyvals, search_indices, delete_indices);
    auto lpt1_stat = testTable(lpt1, keyvals, search_indices, delete_indices);
    auto qpt1_stat = testTable(qpt1, keyvals, search_indices, delete_indices);

    auto dht2_stat = testTable(dht2, keyvals, search_indices, delete_indices);
    auto lpt2_stat = testTable(lpt2, keyvals, search_indices, delete_indices);
    auto qpt2_stat = testTable(qpt2, keyvals, search_indices, delete_indices);

    // In bảng thống kê thời gian tổng
    printSummaryTable(lf1, lf2, dht1_stat, dht2_stat, lpt1_stat, lpt2_stat, qpt1_stat, qpt2_stat);

    // In bảng thống kê chi tiết probe, collision, rate
    std::cout << "\n===== SUMMARY TABLE: PROBES, COLLISIONS, RATES =====\n";
    printDetailHeader();
    printDetailStats("DoubleHash-LF1", lf1, dht1);
    printDetailStats("LinearProb-LF1", lf1, lpt1);
    printDetailStats("QuadraticProb-LF1", lf1, qpt1);
    printDetailStats("DoubleHash-LF2", lf2, dht2);
    printDetailStats("LinearProb-LF2", lf2, lpt2);
    printDetailStats("QuadraticProb-LF2", lf2, qpt2);

    std::cout << "\n=== FINISHED ===\n";
    system("pause");
    return 0;
}
