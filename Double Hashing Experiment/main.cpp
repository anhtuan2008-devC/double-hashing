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
class DoubleHashTable {
    int TABLE_SIZE;
    int keysPresent;
    int PRIME;
    std::vector<int> hashTable;
    std::vector<bool> isPrimeArr;
public:
    int totalProbesInsert = 0;
    int totalProbesSearch = 0;
    int totalProbesDelete = 0;
    int totalCollision = 0;
    int nInsert = 0;
    int nSearch = 0;
    int nDelete = 0;

    DoubleHashTable(int n) {
        TABLE_SIZE = n;
        isPrimeArr.assign(n, true);
        for (int i = 2; i * i < n; ++i) {
            if (isPrimeArr[i]) {
                for (int j = i * i; j < n; j += i) {
                    isPrimeArr[j] = false;
                }
            }
        }
        PRIME = TABLE_SIZE - 1;
        while (PRIME > 1 && !isPrimeArr[PRIME]) {
            PRIME--;
        }
        keysPresent = 0;
        hashTable.assign(TABLE_SIZE, -1);
    }
	// Hàm băm đầu tiên (hash1) 
    int hash1(int value) { 
        return value % TABLE_SIZE; 
    }
	// Hàm băm thứ hai (hash2) sử dụng số nguyên tố
    int hash2(int value) {
        return PRIME - (value % PRIME); 
    }
	// Kiểm tra xem bảng băm đã đầy chưa
    bool isFull() { 
        return keysPresent == TABLE_SIZE; 
    }
	// Hàm chèn giá trị vào bảng băm
    bool insert(int value) {
        if (isFull()) {
            return false;
        }
        int probe = hash1(value);
        int offset = hash2(value);
        int probes = 1;

        if (hashTable[probe] != -1 && hashTable[probe] != -2) {
            totalCollision++;
        }
        while (hashTable[probe] != -1 && hashTable[probe] != -2) {
            probe = (probe + offset) % TABLE_SIZE;
            probes++;
        }
        hashTable[probe] = value;
        keysPresent++;
        totalProbesInsert += probes;
        nInsert++;
        return true;
    }
	// Hàm tìm kiếm giá trị trong bảng băm
    bool search(int value) {
        int probe = hash1(value);
        int offset = hash2(value);
        int initialPos = probe;
        int probes = 1;
        bool firstItr = true;

        while (true) {
            if (hashTable[probe] == -1) {
                break;
            }
            if (hashTable[probe] == value) {
                totalProbesSearch += probes; 
                nSearch++;
                return true;
            }
            if (probe == initialPos && !firstItr) {
                break;
            }
            probe = (probe + offset) % TABLE_SIZE;
            probes++; 
            firstItr = false;
        }
        totalProbesSearch += probes; 
        nSearch++;
        return false;
    }
	// Hàm xóa giá trị khỏi bảng băm
    void erase(int value) {
        int probe = hash1(value);
        int offset = hash2(value);
        int initialPos = probe;
        int probes = 1;
        bool firstItr = true;

        while (true) {
            if (hashTable[probe] == -1) { 
                totalProbesDelete += probes; 
                nDelete++; 
                return; 
            }
            if (hashTable[probe] == value) {
                hashTable[probe] = -2;
                keysPresent--;
                totalProbesDelete += probes; 
                nDelete++;
                return;
            }
            if (probe == initialPos && !firstItr) { 
                totalProbesDelete += probes; 
                nDelete++; 
                return; 
            }
            probe = (probe + offset) % TABLE_SIZE;
            probes++; 
            firstItr = false;
        }
    }
	// Hàm in bảng băm
    void print() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << (hashTable[i] == -1 ? "Empty" : std::to_string(hashTable[i])) << ", ";
        }
        std::cout << "\n";
    }
};

// ======= Linear Probing Table =======
class LinearProbingTable {
    int TABLE_SIZE;
    int keysPresent;
    std::vector<int> hashTable;
public:
    int totalProbesInsert = 0;
    int totalProbesSearch = 0;
    int totalProbesDelete = 0;
    int totalCollision = 0;
    int nInsert = 0;
    int nSearch = 0;
    int nDelete = 0;

    LinearProbingTable(int n) : TABLE_SIZE(n), keysPresent(0), hashTable(n, -1) {}
	// Hàm băm đơn giản
    int hash(int value) { 
        return value % TABLE_SIZE; 
    }
	// Kiểm tra xem bảng băm đã đầy chưa
    bool isFull() { 
        return keysPresent == TABLE_SIZE; 
    }
	// Hàm chèn giá trị vào bảng băm
    bool insert(int value) {
        if (isFull()) {
            return false;
        }
        int probe = hash(value);
        int probes = 1;
        if (hashTable[probe] != -1 && hashTable[probe] != -2) {
            totalCollision++;
        }
        while (hashTable[probe] != -1 && hashTable[probe] != -2) {
            probe = (probe + 1) % TABLE_SIZE; 
            probes++;
        }
        hashTable[probe] = value; 
        keysPresent++;
        totalProbesInsert += probes; 
        nInsert++;
        return true;
    }
	// Hàm tìm kiếm giá trị trong bảng băm
    bool search(int value) {
        int probe = hash(value);
        int initialPos = probe;
        int probes = 1;
        bool firstItr = true;

        while (true) {
            if (hashTable[probe] == -1) {
                break;
            }
            if (hashTable[probe] == value) { 
                totalProbesSearch += probes; 
                nSearch++; 
                return true; 
            }
            if (probe == initialPos && !firstItr) {
                break;
            }
            probe = (probe + 1) % TABLE_SIZE; 
            probes++; 
            firstItr = false;
        }
        totalProbesSearch += probes; 
        nSearch++;
        return false;
    }
	// Hàm xóa giá trị khỏi bảng băm
    void erase(int value) {
        int probe = hash(value);
        int initialPos = probe;
        int probes = 1;
        bool firstItr = true;

        while (true) {
            if (hashTable[probe] == -1) { 
                totalProbesDelete += probes; 
                nDelete++; 
                return; 
            }
            if (hashTable[probe] == value) {
                hashTable[probe] = -2; 
                keysPresent--;
                totalProbesDelete += probes; 
                nDelete++;
                return;
            }
            if (probe == initialPos && !firstItr) { 
                totalProbesDelete += probes; 
                nDelete++; 
                return; 
            }
            probe = (probe + 1) % TABLE_SIZE; 
            probes++; 
            firstItr = false;
        }
    }
	// Hàm in bảng băm
    void print() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << (hashTable[i] == -1 ? "Empty" : std::to_string(hashTable[i])) << ", ";
        }
        std::cout << "\n";
    }
};

class QuadraticProbingTable {
    int TABLE_SIZE;
    int keysPresent;
    std::vector<int> hashTable;
public:
    int totalProbesInsert = 0;
    int totalProbesSearch = 0;
    int totalProbesDelete = 0;
    int totalCollision = 0;
    int nInsert = 0;
    int nSearch = 0;
    int nDelete = 0;

    QuadraticProbingTable(int n) : TABLE_SIZE(n), keysPresent(0), hashTable(n, -1) {}
	// Hàm băm đơn giản
    int hash(int value) { 
        return value % TABLE_SIZE; 
    }
	// Kiểm tra xem bảng băm đã đầy chưa
    bool isFull() { 
        return keysPresent == TABLE_SIZE; 
    }
	// Hàm chèn giá trị vào bảng băm
    bool insert(int value) {
        if (isFull()) {
            return false;
        }
        int probe = hash(value), probes = 1, i = 0;
        if (hashTable[probe] != -1 && hashTable[probe] != -2) {
            totalCollision++;
        }
        while (hashTable[probe] != -1 && hashTable[probe] != -2) {
            ++i;
            probe = (hash(value) + i * i) % TABLE_SIZE;
            probes++;
        }
        hashTable[probe] = value; 
        keysPresent++;
        totalProbesInsert += probes; 
        nInsert++;
        return true;
    }
	// Hàm tìm kiếm giá trị trong bảng băm
    bool search(int value) {
        int probe = hash(value);
        int initialPos = probe;
        int probes = 1;
        int i = 0;
        bool firstItr = true;

        while (true) {
            if (hashTable[probe] == -1) {
                break;
            }
            if (hashTable[probe] == value) { 
                totalProbesSearch += probes; 
                nSearch++; 
                return true; 
            }
            ++i;
            probe = (hash(value) + i * i) % TABLE_SIZE;
            probes++; 
            firstItr = false;
            if (probe == initialPos && !firstItr) {
                break;
            }
        }
        totalProbesSearch += probes; 
        nSearch++;
        return false;
    }
	// Hàm xóa giá trị khỏi bảng băm
    void erase(int value) {
        int probe = hash(value);
        int initialPos = probe;
        int probes = 1;
        int i = 0;
        bool firstItr = true;

        while (true) {
            if (hashTable[probe] == -1) { 
                totalProbesDelete += probes;
                nDelete++;
                return;
            }
            if (hashTable[probe] == value) {
                hashTable[probe] = -2; 
                keysPresent--;
                totalProbesDelete += probes; 
                nDelete++;
                return;
            }
            ++i;
            probe = (hash(value) + i * i) % TABLE_SIZE;
            probes++; 
            firstItr = false;
            if (probe == initialPos && !firstItr) { 
                totalProbesDelete += probes; 
                nDelete++; 
                return; 
            }
        }
    }
};
// Hàm tính thời gian thực hiện các thao tác trên bảng băm
template <typename Table>
StatResult testTable(Table& table, const std::vector<int>& vals, const std::vector<int>& search_indices, const std::vector<int>& delete_indices) {
    StatResult res;
    auto t1 = std::chrono::steady_clock::now();
    for (auto x : vals) table.insert(x);
    auto t2 = std::chrono::steady_clock::now();
    for (int idx : search_indices) table.search(vals[idx]);
    auto t3 = std::chrono::steady_clock::now();
    for (int idx : delete_indices) table.erase(vals[idx]);
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
    std::cout << "Enter the number of searches (MAX " << M << "): "; 
    std::cin >> num_search;
    num_search = std::min(num_search, M);

    std::cout << "Enter the number of deletes (MAX " << M << "): "; 
    std::cin >> num_delete;
    num_delete = std::min(num_delete, M);

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(1, std::max(N1, N2) * 10);

    // Sinh M số ngẫu nhiên không trùng lặp
    std::vector<int> vals;
    while ((int)vals.size() < M) {
        int x = dist(rng);
        bool dup = false;
        for (int v : vals) 
            if (v == x) { 
                dup = true; 
                break; 
            }
        if (!dup) vals.push_back(x);
    }
    std::cout << "=> Successfully generated test values <=\n";

    // Chỉ số random cho search/delete
    std::vector<int> search_indices, delete_indices, all_indices(M);
    for (int i = 0; i < M; ++i) all_indices[i] = i;
    shuffle(all_indices.begin(), all_indices.end(), rng);
    for (int i = 0; i < num_search; ++i) search_indices.push_back(all_indices[i]);
    shuffle(all_indices.begin(), all_indices.end(), rng);
    for (int i = 0; i < num_delete; ++i) delete_indices.push_back(all_indices[i]);

    // ---- CẤU HÌNH 1: Load factor do người dùng nhập ----
    DoubleHashTable dht1(N1);
    LinearProbingTable lpt1(N1);
    QuadraticProbingTable qpt1(N1);

    // ---- CẤU HÌNH 2: Load factor tối ưu (ví dụ 0.5) ----
    DoubleHashTable dht2(N2);
    LinearProbingTable lpt2(N2);
    QuadraticProbingTable qpt2(N2);

    // Test và lưu thời gian
    auto dht1_stat = testTable(dht1, vals, search_indices, delete_indices);
    auto lpt1_stat = testTable(lpt1, vals, search_indices, delete_indices);
    auto qpt1_stat = testTable(qpt1, vals, search_indices, delete_indices);

    auto dht2_stat = testTable(dht2, vals, search_indices, delete_indices);
    auto lpt2_stat = testTable(lpt2, vals, search_indices, delete_indices);
    auto qpt2_stat = testTable(qpt2, vals, search_indices, delete_indices);

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