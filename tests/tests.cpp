#include <cassert>
#include <iostream>
#include "main.cpp"  

void testDoubleHashTable() {
    DoubleHashTable<int, int> table(11);
    assert(table.insert(5, 100));
    assert(table.insert(16, 200)); 

    int val;
    assert(table.search(5, val));
    assert(val == 100);

    assert(table.search(16, val));
    assert(val == 200);

    table.erase(5);
    assert(!table.search(5, val));
    assert(table.search(16, val));
}

void testLinearHashTable() {
    LinearHashTable<int, int> table(7);
    assert(table.insert(1, 10));
    assert(table.insert(8, 20)); // collide với 1

    int val;
    assert(table.search(1, val));
    assert(val == 10);

    assert(table.search(8, val));
    assert(val == 20);

    table.erase(1);
    assert(!table.search(1, val));
    assert(table.search(8, val));
}

void testQuadraticHashTable() {
    QuadraticHashTable<int, int> table(13);
    assert(table.insert(3, 300));
    assert(table.insert(16, 1600)); // collide với 3

    int val;
    assert(table.search(3, val));
    assert(val == 300);

    assert(table.search(16, val));
    assert(val == 1600);

    table.erase(3);
    assert(!table.search(3, val));
    assert(table.search(16, val));
}

int main() {
    std::cout << "Running unit tests...\n";
    testDoubleHashTable();
    testLinearHashTable();
    testQuadraticHashTable();
    std::cout << "All tests passed!\n";
    return 0;
}
