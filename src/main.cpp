#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <numeric>
#include <algorithm>

enum SlotState { 
    EMPTY, 
    OCCUPIED, 
    DELETED 
};

template<typename K, typename V>
struct Entry {
    K key;
    V value;
    SlotState state;
    Entry() : state(EMPTY) {}
    Entry(const K& k, const V& v, SlotState s) : key(k), value(v), state(s) {}
};

struct StatResult {
    // đơn vị: microseconds
    long long insertTime;
    long long searchTime;
    long long deleteTime;
};

struct HashStats {
    int totalProbesInsert = 0;
    int totalProbesSearch = 0;
    int totalProbesDelete = 0;
    int totalCollision = 0;
    int nInsert = 0;
    int nSearch = 0;
    int nDelete = 0;
};

bool isPrime(int n);
int nextPrime(int n);
std::string doubleToStr(double x, int precision = 2);
void printSummaryTable(double lf1, double lf2, StatResult dht1, StatResult dht2, StatResult lpt1, StatResult lpt2, StatResult qpt1, StatResult qpt2);
void printDetailHeader(void);
int getTestSize();
double getUserLoadFactor();
void printTableSizes(double lf1, double lf2, int N1, int N2);
int getNumOps(const std::string& opName, int maxVal);
std::vector<std::pair<int, int>> generateUniqueKeyVals(int M, int N1, int N2);
std::pair<std::vector<int>, std::vector<int>> generateIndices(int M, int num_search, int num_delete);

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
    HashStats stats;

    DoubleHashTable(int n) {
        TABLE_SIZE = n;
        isPrimeArr.assign(n, true);
        for (int i = 2; i * i < n; ++i)
            if (isPrimeArr[i])
                for (int j = i * i; j < n; j += i)
                    isPrimeArr[j] = false;
        PRIME = TABLE_SIZE - 1;
        while (PRIME > 1 && !isPrimeArr[PRIME]) 
            PRIME--;
        keysPresent = 0;
        hashTable.assign(TABLE_SIZE, Entry<K, V>());
    }
    
    int hash1(const K& key) { 
        return std::hash<K>{}(key) % TABLE_SIZE; 
    }

    int hash2(const K& key) { 
        return PRIME - (std::hash<K>{}(key) % PRIME); 
    }

    bool isFull() 
    { 
        return keysPresent == TABLE_SIZE; 
    }

    bool insert(const K& key, const V& value) {
        if (isFull()) return false;
        int probe = hash1(key);
        int offset = hash2(key);
        int probes = 1;
        if (hashTable[probe].state == OCCUPIED) 
            stats.totalCollision++;
        while (hashTable[probe].state == OCCUPIED && hashTable[probe].key != key) {
            probe = (probe + offset) % TABLE_SIZE;
            probes++;
        }
        if (hashTable[probe].state != OCCUPIED) {
            hashTable[probe].key = key;
            hashTable[probe].value = value;
            hashTable[probe].state = OCCUPIED;
            keysPresent++;
            stats.totalProbesInsert += probes;
            stats.nInsert++;
            return true;
        }
        else if (hashTable[probe].key == key) { 
            hashTable[probe].value = value;
            return true;
        }
        return false;
    }

    bool search(const K& key, V& outValue) {
        int probe = hash1(key);
        int offset = hash2(key);
        int initialPos = probe;
        int probes = 1;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) 
                break;
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                outValue = hashTable[probe].value;
                stats.totalProbesSearch += probes; 
                stats.nSearch++;
                return true;
            }
            if (probe == initialPos && !firstItr) 
                break;
            probe = (probe + offset) % TABLE_SIZE;
            probes++; 
            firstItr = false;
        }
        stats.totalProbesSearch += probes; 
        stats.nSearch++;
        return false;
    }

    void erase(const K& key) {
        int probe = hash1(key);
        int offset = hash2(key);
        int initialPos = probe;
        int probes = 1;
        bool firstItr = true;
        while (true) {
            if (hashTable[probe].state == EMPTY) { 
                stats.totalProbesDelete += probes; 
                stats.nDelete++; 
                return; 
            }
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                hashTable[probe].state = DELETED; 
                keysPresent--;
                stats.totalProbesDelete += probes; 
                stats.nDelete++; 
                return;
            }
            if (probe == initialPos && !firstItr) { 
                stats.totalProbesDelete += probes; 
                stats.nDelete++; 
                return; 
            }
            probe = (probe + offset) % TABLE_SIZE;
            probes++; 
            firstItr = false;
        }
    }
};

// ======= Linear Probing Table =======
template<typename K, typename V>
class LinearHashTable {
    int TABLE_SIZE;
    int keysPresent;
    std::vector<Entry<K, V>> hashTable;
public:
    HashStats stats;

    LinearHashTable(int n) {
        TABLE_SIZE = n;
        keysPresent = 0;
        hashTable.assign(TABLE_SIZE, Entry<K, V>());
    }

    int hash(const K& key) {
        return std::hash<K>{}(key) % TABLE_SIZE;
    }

    bool isFull() {
        return keysPresent == TABLE_SIZE;
    }

    bool insert(const K& key, const V& value) {
        if (isFull()) return false;
        int probe = hash(key);
        int probes = 1;
        if (hashTable[probe].state == OCCUPIED)
            stats.totalCollision++;
        while (hashTable[probe].state == OCCUPIED && hashTable[probe].key != key) {
            probe = (probe + 1) % TABLE_SIZE;
            probes++;
        }
        if (hashTable[probe].state != OCCUPIED) {
            hashTable[probe] = Entry<K, V>(key, value, OCCUPIED);
            keysPresent++;
            stats.totalProbesInsert += probes;
            stats.nInsert++;
            return true;
        } else if (hashTable[probe].key == key) {
            hashTable[probe].value = value;
            return true;
        }
        return false;
    }

    bool search(const K& key, V& outValue) {
        int probe = hash(key);
        int probes = 1;
        while (hashTable[probe].state != EMPTY) {
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                outValue = hashTable[probe].value;
                stats.totalProbesSearch += probes;
                stats.nSearch++;
                return true;
            }
            probe = (probe + 1) % TABLE_SIZE;
            probes++;
        }
        stats.totalProbesSearch += probes;
        stats.nSearch++;
        return false;
    }

    void erase(const K& key) {
        int probe = hash(key);
        int probes = 1;
        while (hashTable[probe].state != EMPTY) {
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                hashTable[probe].state = DELETED;
                keysPresent--;
                stats.totalProbesDelete += probes;
                stats.nDelete++;
                return;
            }
            probe = (probe + 1) % TABLE_SIZE;
            probes++;
        }
        stats.totalProbesDelete += probes;
        stats.nDelete++;
    }
};

// ======= Quadratic Probing Table =======
template<typename K, typename V>
class QuadraticHashTable {
    int TABLE_SIZE;
    int keysPresent;
    std::vector<Entry<K, V>> hashTable;
public:
    HashStats stats;

    QuadraticHashTable(int n) {
        TABLE_SIZE = n;
        keysPresent = 0;
        hashTable.assign(TABLE_SIZE, Entry<K, V>());
    }

    int hash(const K& key) {
        return std::hash<K>{}(key) % TABLE_SIZE;
    }

    bool isFull() {
        return keysPresent == TABLE_SIZE;
    }

    bool insert(const K& key, const V& value) {
        if (isFull()) return false;
        int base = hash(key);
        int i = 0;
        int probes = 0;
        while (i < TABLE_SIZE) {
            int probe = (base + i * i) % TABLE_SIZE;
            probes++;
            if (i == 0 && hashTable[probe].state == OCCUPIED)
                stats.totalCollision++;
            if (hashTable[probe].state == EMPTY || hashTable[probe].state == DELETED) {
                hashTable[probe] = Entry<K, V>(key, value, OCCUPIED);
                keysPresent++;
                stats.totalProbesInsert += probes;
                stats.nInsert++;
                return true;
            } else if (hashTable[probe].key == key) {
                hashTable[probe].value = value;
                return true;
            }
            i++;
        }
        return false;
    }

    bool search(const K& key, V& outValue) {
        int base = hash(key);
        int i = 0;
        int probes = 0;
        while (i < TABLE_SIZE) {
            int probe = (base + i * i) % TABLE_SIZE;
            probes++;
            if (hashTable[probe].state == EMPTY)
                break;
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                outValue = hashTable[probe].value;
                stats.totalProbesSearch += probes;
                stats.nSearch++;
                return true;
            }
            i++;
        }
        stats.totalProbesSearch += probes;
        stats.nSearch++;
        return false;
    }

    void erase(const K& key) {
        int base = hash(key);
        int i = 0;
        int probes = 0;
        while (i < TABLE_SIZE) {
            int probe = (base + i * i) % TABLE_SIZE;
            probes++;
            if (hashTable[probe].state == EMPTY)
                break;
            if (hashTable[probe].state == OCCUPIED && hashTable[probe].key == key) {
                hashTable[probe].state = DELETED;
                keysPresent--;
                stats.totalProbesDelete += probes;
                stats.nDelete++;
                return;
            }
            i++;
        }
        stats.totalProbesDelete += probes;
        stats.nDelete++;
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
void printSummaryTable(double lf1, double lf2, StatResult dht1, StatResult dht2, StatResult lpt1, StatResult lpt2, StatResult qpt1, StatResult qpt2) {
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
    const auto& stats = table.stats;

    double avgInsertProbes = (stats.nInsert > 0) ? 1.0 * stats.totalProbesInsert / stats.nInsert : 0;
    double collisionRate   = (stats.nInsert > 0) ? 100.0 * stats.totalCollision / stats.nInsert : 0;
    double avgSearchProbes = (stats.nSearch > 0) ? 1.0 * stats.totalProbesSearch / stats.nSearch : 0;
    double avgDeleteProbes = (stats.nDelete > 0) ? 1.0 * stats.totalProbesDelete / stats.nDelete : 0;

    std::cout << std::left
        << std::setw(20) << algoName
        << std::setw(12) << std::fixed << std::setprecision(2) << lf
        << std::setw(10) << stats.nInsert
        << std::setw(12) << std::fixed << std::setprecision(4) << avgInsertProbes
        << std::setw(10) << stats.totalCollision
        << std::setw(12) << std::fixed << std::setprecision(4) << collisionRate
        << std::setw(10) << stats.nSearch
        << std::setw(12) << std::fixed << std::setprecision(4) << avgSearchProbes
        << std::setw(10) << stats.nDelete
        << std::setw(12) << std::fixed << std::setprecision(4) << avgDeleteProbes
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

int getTestSize() {
    int M;
    std::cout << "Enter the number of elements to test: ";
    std::cin >> M;
    return M;
}

double getUserLoadFactor() {
    double lf;
    std::cout << "Enter the first load factor to compare (e.g., 0.7): ";
    std::cin >> lf;
    std::cout << "Auto-selected optimal load factor: 0.5\n";
    return lf;
}

void printTableSizes(double lf1, double lf2, int N1, int N2) {
    std::cout << "TABLE_SIZE with load factor 1 (" << lf1 << "): " << N1 << '\n';
    std::cout << "TABLE_SIZE with load factor 2 (" << lf2 << "): " << N2 << '\n';
}

int getNumOps(const std::string& opName, int maxVal) {
    int num;
    do {
        std::cout << "Enter the number of " << opName << " (MAX " << maxVal << "): ";
        std::cin >> num;
        if (num > maxVal) {
            std::cout << "Too many! Limiting to " << maxVal << ".\n";
            num = maxVal;
        }
    } while (num < 0);
    return num;
}

std::vector<std::pair<int, int>> generateUniqueKeyVals(int M, int N1, int N2) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist_key(1, std::max(N1, N2) * 10);
    std::uniform_int_distribution<int> dist_val(1, 1000000);

    std::unordered_set<int> used;
    std::vector<std::pair<int, int>> keyvals;

    while ((int)keyvals.size() < M) {
        int key = dist_key(rng);
        if (used.count(key)) continue;
        used.insert(key);
        keyvals.emplace_back(key, dist_val(rng));
    }
    std::cout << "=> Successfully generated test values <=\n";
    return keyvals;
}

std::pair<std::vector<int>, std::vector<int>> generateIndices(int M, int num_search, int num_delete) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::vector<int> all_indices(M), search_indices, delete_indices;
    std::iota(all_indices.begin(), all_indices.end(), 0);

    std::shuffle(all_indices.begin(), all_indices.end(), rng);
    search_indices.assign(all_indices.begin(), all_indices.begin() + num_search);

    std::shuffle(all_indices.begin(), all_indices.end(), rng);
    delete_indices.assign(all_indices.begin(), all_indices.begin() + num_delete);

    return {search_indices, delete_indices};
}

int main(void) {
    system("color F0");

    int M = getTestSize();
    double lf1 = getUserLoadFactor();
    double lf2 = 0.5; // Tối ưu mặc định

    int N1 = nextPrime(int(M / lf1));
    int N2 = nextPrime(int(M / lf2));

    printTableSizes(lf1, lf2, N1, N2);

    int num_search = getNumOps("searches", M);
    int num_delete = getNumOps("deletes", M);

    auto keyvals = generateUniqueKeyVals(M, N1, N2);
    auto [search_indices, delete_indices] = generateIndices(M, num_search, num_delete);

    // Tạo bảng băm
    DoubleHashTable<int, int> dht1(N1), dht2(N2);
    LinearHashTable<int, int> lpt1(N1), lpt2(N2);
    QuadraticHashTable<int, int> qpt1(N1), qpt2(N2);

    // Test và đo thời gian
    auto dht1_stat = testTable(dht1, keyvals, search_indices, delete_indices);
    auto dht2_stat = testTable(dht2, keyvals, search_indices, delete_indices);
    auto lpt1_stat = testTable(lpt1, keyvals, search_indices, delete_indices);
    auto lpt2_stat = testTable(lpt2, keyvals, search_indices, delete_indices);
    auto qpt1_stat = testTable(qpt1, keyvals, search_indices, delete_indices);
    auto qpt2_stat = testTable(qpt2, keyvals, search_indices, delete_indices);

    // In kết quả
    printSummaryTable(lf1, lf2, dht1_stat, dht2_stat, lpt1_stat, lpt2_stat, qpt1_stat, qpt2_stat);

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
