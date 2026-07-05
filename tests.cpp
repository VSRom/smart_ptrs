#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
// Предполагаем, что ваш код лежит в файле "my_smart_pointers.hpp"
#include "my_smart_pointers.hpp" 

void test_unique_ptr() {
    std::cout << "Running myUnique_ptr tests..." << std::endl;
    
    // Базовое создание и удаление
    {
        myUnique_ptr<int> u_ptr(new int(42));
        assert(u_ptr.get() != nullptr);
        assert(*u_ptr == 42);
    } // Тут память должна очиститься

    // Перемещение
    myUnique_ptr<int> u1(new int(100));
    myUnique_ptr<int> u2 = std::move(u1);
    assert(u1.get() == nullptr);
    assert(*u2 == 100);

    // Release и Reset
    int* raw = u2.release();
    assert(u2.get() == nullptr);
    assert(*raw == 100);
    
    u2.reset(raw);
    assert(*u2 == 100);
    
    u2.reset();
    assert(u2.get() == nullptr);
    std::cout << "myUnique_ptr tests PASSED!" << std::endl;
}

void test_shared_and_weak_ptr() {
    std::cout << "Running myShared_ptr and myWeak_ptr tests..." << std::endl;

    myWeak_ptr<int> w_outer;
    {
        myShared_ptr<int> s1(new int(500));
        assert(s1.use_count() == 1);
        assert(s1.unique() == true);

        myShared_ptr<int> s2 = s1;
        assert(s1.use_count() == 2);
        assert(s2.use_count() == 2);

        myWeak_ptr<int> w1(s1);
        w_outer = w1;
        assert(w1.expired() == false);
        assert(w1.use_count() == 2);

        // Проверяем работу lock()
        myShared_ptr<int> s3 = w1.lock();
        assert(s3.get() != nullptr);
        assert(*s3 == 500);
        assert(s1.use_count() == 3);
    } // s1, s2, s3 выходят из области видимости

    // Проверяем, что weak_ptr распознал удаление объекта
    assert(w_outer.expired() == true);
    myShared_ptr<int> s_empty = w_outer.lock();
    assert(s_empty.get() == nullptr);

    std::cout << "myShared_ptr and myWeak_ptr tests PASSED!" << std::endl;
}

void test_concurrent_lock() {
    std::cout << "Running multi-threaded concurrent lock() tests..." << std::endl;
    
    myShared_ptr<int> sp(new int(999));
    myWeak_ptr<int> wp(sp);

    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_locks{0};

    // Запускаем потоки, которые одновременно пытаются сделать lock()
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([wp, &success_locks]() {
            for (int j = 0; j < 1000; ++j) {
                if (auto sp_locked = wp.lock()) {
                    if (*sp_locked == 999) {
                        success_locks.fetch_add(1);
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(success_locks.load() == num_threads * 1000);
    std::cout << "Concurrent lock() tests PASSED!" << std::endl;
}

int main() {
    test_unique_ptr();
    std::cout << "---------------------------------------" << std::endl;
    test_shared_and_weak_ptr();
    std::cout << "---------------------------------------" << std::endl;
    test_concurrent_lock();
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
