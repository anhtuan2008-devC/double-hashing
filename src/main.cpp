#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_set>

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
    double avgProbeSearchHit;
    double avgProbeSearchMiss;
    double avgProbeInsertAfterDelete;
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

struct ClusterUtils {
    template<typename EntryType>
    static int maxClusterLength(const std::vector<EntryType>& table) {
        int maxLen = 0, curLen = 0;
        for (const auto& entry : table) {
            if (entry.state == OCCUPIED) {
                ++curLen;
                maxLen = std::max(maxLen, curLen);
            } else {
                curLen = 0;
            }
        }
        return maxLen;
    }

    template<typename EntryType>
    static double avgClusterLength(const std::vector<EntryType>& table) {
        int totalClusters = 0, totalLen = 0, curLen = 0;
        for (const auto& entry : table) {
            if (entry.state == OCCUPIED) {
                ++curLen;
            } else {
                if (curLen > 0) {
                    ++totalClusters;
                    totalLen += curLen;
                    curLen = 0;
                }
            }
        }
        // Nếu bảng kết thúc bằng một cluster
        if (curLen > 0) {
            ++totalClusters;
            totalLen += curLen;
        }
        return totalClusters ? 1.0 * totalLen / totalClusters : 0;
    }
};

bool isPrime(int n);
int nextPrime(int n);
std::string doubleToStr(double x, int precision = 2);
void printSummaryTable(double lf1, double lf2, const StatResult& dht1, const StatResult& dht2, const StatResult& lpt1, const StatResult& lpt2, const StatResult& qpt1, const StatResult& qpt2);
void printDetailHeader(void);
int getTestSize();
double getUserLoadFactor();
void printTableSizes(double lf1, double lf2, int N1, int N2);
std::pair<std::vector<int>, std::vector<int>> generateIndices(int M, int num_search, int num_delete);

template<typename T>
void shuffle(std::vector<T>& vec, std::mt19937& rng) {
    for (int i = vec.size() - 1; i > 0; --i) {
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(rng);
        std::swap(vec[i], vec[j]);
    }
}

template<typename Iterator, typename T>
void iota(Iterator begin, Iterator end, T value) {
    while (begin != end) {
        *begin++ = value++;
    }
}

// Hàm kiểm tra số nguyên tố
bool isPrime(int n) {
    if (n < 2)
        return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) 
            return false;
    }
    return true;
}
// Tìm số nguyên tố lớn hơn n
int nextPrime(int n) {
    int x = n + 1;
    while (!isPrime(x)) 
        ++x;
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
        keysPresent = 0;
        hashTable.assign(TABLE_SIZE, Entry<K, V>());

        // Khởi tạo mảng kiểm tra số nguyên tố
        isPrimeArr = std::vector<bool>(TABLE_SIZE, true);
        if (TABLE_SIZE >= 2) isPrimeArr[0] = isPrimeArr[1] = false;
        for (int i = 2; i * i < TABLE_SIZE; ++i) {
            if (isPrimeArr[i]) {
                for (int j = i * i; j < TABLE_SIZE; j += i) {
                    isPrimeArr[j] = false;
                }
            }
        }

        // Tìm số nguyên tố lớn nhất < TABLE_SIZE
        PRIME = TABLE_SIZE - 1;
        while (PRIME > 1 && !isPrimeArr[PRIME])
            PRIME--;
    }
    
    int hash1(const K& key) { 
        return std::hash<K>{}(key) % TABLE_SIZE; 
    }

    int hash2(const K& key) { 
        return PRIME - (std::hash<K>{}(key) % PRIME); 
    }

    bool isFull() {
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

    int maxClusterLength() const {
        return ClusterUtils::maxClusterLength(hashTable);
    }

    double avgClusterLength() const {
        return ClusterUtils::avgClusterLength(hashTable);
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

    int maxClusterLength() const {
        return ClusterUtils::maxClusterLength(hashTable);
    }

    double avgClusterLength() const {
        return ClusterUtils::avgClusterLength(hashTable);
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

    int maxClusterLength() const {
        return ClusterUtils::maxClusterLength(hashTable);
    }

    double avgClusterLength() const {
        return ClusterUtils::avgClusterLength(hashTable);
    }
};

template<typename DHTable, typename LTable, typename QTable>
void insertAndPrintClusterStats(const DHTable& dht, const LTable& lpt, const QTable& qpt, const std::vector<std::pair<int, int>>& keyvals, const std::string& label = "") {
    DHTable dht_copy = dht;
    LTable lpt_copy = lpt;
    QTable qpt_copy = qpt;
    for (auto& kv : keyvals) {
        dht_copy.insert(kv.first, kv.second);
        lpt_copy.insert(kv.first, kv.second);
        qpt_copy.insert(kv.first, kv.second);
    }
    std::cout << "\n===== CLUSTER LENGTH STATISTICS" << (label.empty() ? "" : (" - " + label)) << " =====\n";
    std::cout << std::setw(32) << std::left << " "
        << std::setw(20) << "Double Hashing"
        << std::setw(20) << "Linear Probing"
        << std::setw(20) << "Quadratic Probing" << '\n';
    std::cout << std::setw(32) << std::left << "[Max cluster length]:"
        << std::setw(20) << dht_copy.maxClusterLength()
        << std::setw(20) << lpt_copy.maxClusterLength()
        << std::setw(20) << qpt_copy.maxClusterLength() << '\n';
    std::cout << std::setw(32) << std::left << "[Avg cluster length]:"
        << std::setw(20) << dht_copy.avgClusterLength()
        << std::setw(20) << lpt_copy.avgClusterLength()
        << std::setw(20) << qpt_copy.avgClusterLength() << '\n';
}

// Hàm tính thời gian thực hiện các thao tác trên bảng băm
template <typename Table>
StatResult testTable(Table& table, const std::vector<std::pair<int, int>>& keyvals, const std::vector<int>& search_hit_indices, const std::vector<int>& search_miss_keys, const std::vector<int>& delete_indices) {
    constexpr int NUM_RUNS = 3;
    StatResult res;
    long long totalInsertTime = 0, totalSearchHitTime = 0, totalSearchMissTime = 0, totalDeleteTime = 0;

    // Thống kê probe cho từng loại search
    long long totalProbeSearchHit = 0, totalProbeSearchMiss = 0;
    long long totalProbeInsertAfterDelete = 0;
    int nInsertAfterDelete = 0;

    for (int run = 0; run < NUM_RUNS; ++run) {
        Table tempTable(table); // clone

        // Insert all keyvals
        auto t1 = std::chrono::high_resolution_clock::now();
        for (auto& kv : keyvals)
            tempTable.insert(kv.first, kv.second);
        auto t2 = std::chrono::high_resolution_clock::now();

        // Search HIT (tồn tại)
        int tmp;
        auto t3 = std::chrono::high_resolution_clock::now();
        for (int idx : search_hit_indices) {
            int probes_before = tempTable.stats.totalProbesSearch;
            tempTable.search(keyvals[idx].first, tmp);
            totalProbeSearchHit += tempTable.stats.totalProbesSearch - probes_before;
        }
        auto t4 = std::chrono::high_resolution_clock::now();

        // Search MISS (không tồn tại)
        auto t5 = std::chrono::high_resolution_clock::now();
        for (int key : search_miss_keys) {
            int probes_before = tempTable.stats.totalProbesSearch;
            tempTable.search(key, tmp);
            totalProbeSearchMiss += tempTable.stats.totalProbesSearch - probes_before;
        }
        auto t6 = std::chrono::high_resolution_clock::now();

        // Delete các key
        auto t7 = std::chrono::high_resolution_clock::now();
        for (int idx : delete_indices)
            tempTable.erase(keyvals[idx].first);
        auto t8 = std::chrono::high_resolution_clock::now();

        // Insert lại các key vừa xóa (giá trị mới random)
        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist_val(1, 1000000);
        for (int idx : delete_indices) {
            int probes_before = tempTable.stats.totalProbesInsert;
            tempTable.insert(keyvals[idx].first, dist_val(rng));
            totalProbeInsertAfterDelete += tempTable.stats.totalProbesInsert - probes_before;
            nInsertAfterDelete++;
        }

        // Tính thời gian từng phần (chia nhỏ rõ ràng)
        totalInsertTime      += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        totalSearchHitTime   += std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
        totalSearchMissTime  += std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count();
        totalDeleteTime      += std::chrono::duration_cast<std::chrono::microseconds>(t8 - t7).count();

        // Ghi lại stats (1 lần duy nhất)
        if (run == 0)
            table.stats = tempTable.stats;
    }

    int nHit = search_hit_indices.size() * NUM_RUNS;
    int nMiss = search_miss_keys.size() * NUM_RUNS;

    res.insertTime       = totalInsertTime      / NUM_RUNS;
    res.searchTime       = (totalSearchHitTime + totalSearchMissTime) / NUM_RUNS;
    res.deleteTime       = totalDeleteTime      / NUM_RUNS;
    res.avgProbeSearchHit  = (nHit   ? 1.0 * totalProbeSearchHit / nHit : 0);
    res.avgProbeSearchMiss = (nMiss  ? 1.0 * totalProbeSearchMiss / nMiss : 0);
    res.avgProbeInsertAfterDelete = (nInsertAfterDelete ? 1.0 * totalProbeInsertAfterDelete / nInsertAfterDelete : 0);

    return res;
}

// Hàm in bảng thống kê tổng hợp thời gian thực hiện các thao tác
void printSummaryTable(double lf1, double lf2, const StatResult& dht1, const StatResult& dht2, const StatResult& lpt1, const StatResult& lpt2, const StatResult& qpt1, const StatResult& qpt2) {
    std::cout << "\n===== TABLE OF PERFORMANCE COMPARISON (us) =====\n";
    std::cout << std::setw(50) << std::left << " "
        << std::setw(20) << "Double Hashing"
        << std::setw(20) << "Linear Probing"
        << std::setw(20) << "Quadratic Probing" << '\n';

    // Thời gian thực hiện các thao tác
    std::cout << std::setw(50) << std::left << ("[Insert Time] LF1 (" + doubleToStr(lf1, 2) + "):")
        << std::setw(20) << dht1.insertTime
        << std::setw(20) << lpt1.insertTime
        << std::setw(20) << qpt1.insertTime << '\n';

    std::cout << std::setw(50) << std::left << ("[Insert Time] LF2 (" + doubleToStr(lf2, 2) + "):")
        << std::setw(20) << dht2.insertTime
        << std::setw(20) << lpt2.insertTime
        << std::setw(20) << qpt2.insertTime << '\n';

    std::cout << std::setw(50) << std::left << ("[Search Time] LF1 (" + doubleToStr(lf1, 2) + "):")
        << std::setw(20) << dht1.searchTime
        << std::setw(20) << lpt1.searchTime
        << std::setw(20) << qpt1.searchTime << '\n';

    std::cout << std::setw(50) << std::left << ("[Search Time] LF2 (" + doubleToStr(lf2, 2) + "):")
        << std::setw(20) << dht2.searchTime
        << std::setw(20) << lpt2.searchTime
        << std::setw(20) << qpt2.searchTime << '\n';

    std::cout << std::setw(50) << std::left << ("[Delete Time] LF1 (" + doubleToStr(lf1, 2) + "):")
        << std::setw(20) << dht1.deleteTime
        << std::setw(20) << lpt1.deleteTime
        << std::setw(20) << qpt1.deleteTime << '\n';

    std::cout << std::setw(50) << std::left << ("[Delete Time] LF2 (" + doubleToStr(lf2, 2) + "):")
        << std::setw(20) << dht2.deleteTime
        << std::setw(20) << lpt2.deleteTime
        << std::setw(20) << qpt2.deleteTime << '\n';

    // In probe search hit/miss/insert after delete 
    std::cout << "\n----- PROBE STATISTICS (Average probes per operation) -----\n";
    std::cout << std::setw(50) << std::left << "[Avg probe/search HIT] LF1:"
        << std::setw(20) << dht1.avgProbeSearchHit
        << std::setw(20) << lpt1.avgProbeSearchHit
        << std::setw(20) << qpt1.avgProbeSearchHit << '\n';

    std::cout << std::setw(50) << std::left << "[Avg probe/search MISS] LF1:"
        << std::setw(20) << dht1.avgProbeSearchMiss
        << std::setw(20) << lpt1.avgProbeSearchMiss
        << std::setw(20) << qpt1.avgProbeSearchMiss << '\n';

    std::cout << std::setw(50) << std::left << "[Avg probe/insert-after-delete] LF1:"
        << std::setw(20) << dht1.avgProbeInsertAfterDelete
        << std::setw(20) << lpt1.avgProbeInsertAfterDelete
        << std::setw(20) << qpt1.avgProbeInsertAfterDelete << '\n';

    std::cout << std::setw(50) << std::left << "[Avg probe/search HIT] LF2:"
        << std::setw(20) << dht2.avgProbeSearchHit
        << std::setw(20) << lpt2.avgProbeSearchHit
        << std::setw(20) << qpt2.avgProbeSearchHit << '\n';

    std::cout << std::setw(50) << std::left << "[Avg probe/search MISS] LF2:"
        << std::setw(20) << dht2.avgProbeSearchMiss
        << std::setw(20) << lpt2.avgProbeSearchMiss
        << std::setw(20) << qpt2.avgProbeSearchMiss << '\n';

    std::cout << std::setw(50) << std::left << "[Avg probe/insert-after-delete] LF2:"
        << std::setw(20) << dht2.avgProbeInsertAfterDelete
        << std::setw(20) << lpt2.avgProbeInsertAfterDelete
        << std::setw(20) << qpt2.avgProbeInsertAfterDelete << '\n';
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

// Hàm sinh search hit/miss indices với num_search mặc định = M, miss_rate nhập từ user
std::pair<std::vector<int>, std::vector<int>> generateSearchHitMissIndices(const int& M, const std::vector<int>& all_indices, const std::vector<std::pair<int, int>>& keyvals,const int& key_upper_bound ) {
    int num_search = M;
    double miss_rate = -1;
    while (miss_rate < 0 || miss_rate > 1) {
        std::cout << "Enter the miss rate (0-1) for search operations: ";
        std::cin >> miss_rate;
        if (miss_rate < 0 || miss_rate > 1) {
            std::cout << "Invalid rate! Please enter a value between 0 and 1.\n";
        }
    }

    int num_miss = int(num_search * miss_rate + 0.5);
    int num_hit = num_search - num_miss;

    std::vector<int> hit_indices, miss_keys;

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist_key(1, key_upper_bound);

    // Chọn ngẫu nhiên num_hit chỉ số trong all_indices (truy cập key hợp lệ)
    std::vector<int> indices = all_indices;
    shuffle(indices.begin(), indices.end(), rng);
    hit_indices.assign(indices.begin(), indices.begin() + num_hit);

    // Lấy tập key đã tồn tại để sinh miss key không trùng
    std::unordered_set<int> exist_keys;
    for (const auto& kv : keyvals) exist_keys.insert(kv.first);

    while ((int)miss_keys.size() < num_miss) {
        int key = dist_key(rng);
        if (exist_keys.count(key)) continue; // Đảm bảo miss
        miss_keys.push_back(key);
        exist_keys.insert(key); // Tránh trùng lặp miss
    }

    return {hit_indices, miss_keys};
}

std::vector<std::pair<int, int>> generateRandomKeyVals(int M, int key_upper, int val_upper = 1000000) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist_key(1, key_upper);
    std::uniform_int_distribution<int> dist_val(1, val_upper);

    std::unordered_set<int> used;
    std::vector<std::pair<int, int>> keyvals;
    while ((int)keyvals.size() < M) {
        int key = dist_key(rng);
        if (used.count(key)) continue;
        used.insert(key);
        keyvals.emplace_back(key, dist_val(rng));
    }
    return keyvals;
}

std::vector<std::pair<int, int>> generateSequentialKeyVals(int M, int val_upper = 1000000) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist_val(1, val_upper);
    std::vector<std::pair<int, int>> keyvals;
    for (int i = 1; i <= M; ++i)
        keyvals.emplace_back(i, dist_val(rng));
    return keyvals;
}

std::vector<std::pair<int, int>> generateClusteredKeyVals(int M, int key_upper, int val_upper = 1000000) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist_val(1, val_upper);

    int num_clusters = 5;
    int per_cluster = M / num_clusters;
    std::vector<std::pair<int, int>> keyvals;
    int base = 1;
    for (int c = 0; c < num_clusters; ++c) {
        for (int i = 0; i < per_cluster && (int)keyvals.size() < M; ++i) {
            keyvals.emplace_back(base + i, dist_val(rng));
        }
        base += key_upper / num_clusters; // Nhảy cụm
    }
    // Nếu chưa đủ, sinh thêm key lẻ cuối cùng
    while ((int)keyvals.size() < M) {
        keyvals.emplace_back(base++, dist_val(rng));
    }
    return keyvals;
}

std::pair<std::vector<int>, std::vector<int>> generateIndices(int M, int num_search, int num_delete) {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::vector<int> all_indices(M), search_indices, delete_indices;
    iota(all_indices.begin(), all_indices.end(), 0);

    shuffle(all_indices.begin(), all_indices.end(), rng);
    search_indices.assign(all_indices.begin(), all_indices.begin() + num_search);

    shuffle(all_indices.begin(), all_indices.end(), rng);
    delete_indices.assign(all_indices.begin(), all_indices.begin() + num_delete);

    return {search_indices, delete_indices};
}

int main(void) {
    // Nhập đầu vào chung
    int M = getTestSize();
    double lf1 = getUserLoadFactor();
    double lf2 = 0.5; // tự động chọn

    int N1 = nextPrime(int(M / lf1));
    int N2 = nextPrime(int(M / lf2));
    printTableSizes(lf1, lf2, N1, N2);

    // Nhập miss rate và số delete chỉ 1 lần
    double miss_rate = -1;
    while (miss_rate < 0 || miss_rate > 1) {
        std::cout << "Enter the miss rate (0-1) for search operations: ";
        std::cin >> miss_rate;
        if (miss_rate < 0 || miss_rate > 1) {
            std::cout << "Invalid rate! Please enter a value between 0 and 1.\n";
        }
    }

    int num_delete = M;

    // Lặp qua cả 3 kiểu dữ liệu
    for (int datatype = 1; datatype <= 3; ++datatype) {
        std::string patternName;
        if (datatype == 1) patternName = "RANDOM";
        else if (datatype == 2) patternName = "SEQUENTIAL";
        else patternName = "CLUSTERED";

        std::cout << "\n===============================\n";
        std::cout << ">>> DATA PATTERN: " << patternName << "\n";

        // Tạo key-value tương ứng
        std::vector<std::pair<int, int>> keyvals;
        if (datatype == 1)
            keyvals = generateRandomKeyVals(M, std::max(N1, N2) * 10);
        else if (datatype == 2)
            keyvals = generateSequentialKeyVals(M);
        else
            keyvals = generateClusteredKeyVals(M, std::max(N1, N2) * 10);

        // Sinh chỉ số search hit/miss
        std::vector<int> all_indices(M);
        iota(all_indices.begin(), all_indices.end(), 0);

        int num_search = M;
        int num_miss = int(num_search * miss_rate + 0.5);
        int num_hit = num_search - num_miss;

        // Hit indices
        std::vector<int> indices = all_indices;
        shuffle(indices.begin(), indices.end(), std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::vector<int> search_hit_indices(indices.begin(), indices.begin() + num_hit);

        // Miss keys
        std::unordered_set<int> exist_keys;
        for (const auto& kv : keyvals) exist_keys.insert(kv.first);
        std::vector<int> search_miss_keys;
        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist_key(1, std::max(N1, N2) * 10);
        while ((int)search_miss_keys.size() < num_miss) {
            int key = dist_key(rng);
            if (exist_keys.count(key)) continue;
            search_miss_keys.push_back(key);
            exist_keys.insert(key);
        }

        // Delete indices
        std::vector<int> delete_indices = all_indices;
        shuffle(delete_indices.begin(), delete_indices.end(), rng);
        delete_indices.resize(num_delete);

        // Tạo bảng băm cho 2 cấu hình LF1 và LF2
        DoubleHashTable<int, int> dht1(N1), dht2(N2);
        LinearHashTable<int, int> lpt1(N1), lpt2(N2);
        QuadraticHashTable<int, int> qpt1(N1), qpt2(N2);

        // Thống kê cluster
        insertAndPrintClusterStats(dht1, lpt1, qpt1, keyvals, "After Insert with LF1");
        insertAndPrintClusterStats(dht2, lpt2, qpt2, keyvals, "After Insert with LF2");

        // Đo hiệu năng
        auto dht1_stat = testTable(dht1, keyvals, search_hit_indices, search_miss_keys, delete_indices);
        auto dht2_stat = testTable(dht2, keyvals, search_hit_indices, search_miss_keys, delete_indices);
        auto lpt1_stat = testTable(lpt1, keyvals, search_hit_indices, search_miss_keys, delete_indices);
        auto lpt2_stat = testTable(lpt2, keyvals, search_hit_indices, search_miss_keys, delete_indices);
        auto qpt1_stat = testTable(qpt1, keyvals, search_hit_indices, search_miss_keys, delete_indices);
        auto qpt2_stat = testTable(qpt2, keyvals, search_hit_indices, search_miss_keys, delete_indices);

        // In bảng thống kê hiệu năng
        printSummaryTable(lf1, lf2, dht1_stat, dht2_stat, lpt1_stat, lpt2_stat, qpt1_stat, qpt2_stat);

        std::cout << "\n===== SUMMARY TABLE: PROBES, COLLISIONS, RATES =====\n";
        printDetailHeader();
        printDetailStats("DoubleHash-LF1", lf1, dht1);
        printDetailStats("LinearProb-LF1", lf1, lpt1);
        printDetailStats("QuadraticProb-LF1", lf1, qpt1);
        printDetailStats("DoubleHash-LF2", lf2, dht2);
        printDetailStats("LinearProb-LF2", lf2, lpt2);
        printDetailStats("QuadraticProb-LF2", lf2, qpt2);
        std::cout << "\n";
    }

    std::cout << "\n=== FINISHED ALL DATA PATTERNS ===\n";
    return 0;
}
