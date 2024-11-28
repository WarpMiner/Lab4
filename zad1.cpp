#include <iostream>
#include <thread>
#include <mutex>
#include <barrier>
#include <chrono>
#include <random>
#include <semaphore>
#include <condition_variable>
#include <atomic>

using namespace std;

const int thread_count = 5; // Кол-во потоков
const int char_count = 1; // Кол-во символов, которое генерируется потоком

barrier my_barrier(thread_count); // Определяем барьер для синхронизации потоков
mutex slim_mtx; // Мьютекс для обеспечения взаимного исключения
counting_semaphore<1> sem(1); // Семофор с единичным счетчиком

class StopWatch { // Класс для измерения времени выполнения
    chrono::time_point<chrono::steady_clock> start_time; 
public:
    void start() {
        start_time = chrono::steady_clock::now(); // Запускаем таймер
    }

    double elapsed() {
        auto end_time = chrono::steady_clock::now(); // Получаем текущее время
        chrono::duration<double> duration = end_time - start_time; // Вычисляем продолжительность
        return duration.count(); // Возвращаем продолжительность в секундах
    }
};

class SemaphoreSlim {
    atomic<int> count; // Подсчет доступных ресурсов
    mutex mtx; // Мьютекс для защиты данных
    condition_variable cv; // Условная переменная для сигнализации
public:
    
    SemaphoreSlim(int initial) : count(initial) {}
    
    void acquireSlim() { // Функция для захвата ресурса
        unique_lock<mutex> lock(slim_mtx);
        while (count <= 0) {
            cv.wait(lock); // Ожидаем освобождения ресурса
        }
        --count; // Уменьшаем счетчик
    }

    void releaseSlim() {  // Функция для освобождения ресурса
        unique_lock<mutex> lock(mtx);
        ++count; // Увеличиваем счетчик
        cv.notify_one(); // Уведомляем один поток
    }
};

class Monitor {
private:
    mutex m;
    condition_variable cv;

public:
    
    void enter() {
        m.lock();
    }

    void exit() {
        m.unlock();
    }

    void wait() {
        unique_lock<mutex> lock(m);
        cv.wait(lock); // Ожидание сигнала
    }

    void signal() {
        unique_lock<mutex> lock(m);
        cv.notify_one(); // Разбудить один поток
    }
};
SemaphoreSlim sem_slim(1);
Monitor monitor;

class RandomCharGenerator {
    mutex mtx;
    condition_variable slim_cv;
    atomic_flag spinlock = ATOMIC_FLAG_INIT; // Spinlock для реализации взаимного исключения
    
public:
    void generateRandomCharsMutex(int thread_id) { // Генерация случайных символов с использованием мьютекса
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126);

        for (int i = 0; i < char_count; ++i) {
            char random_char = dis(gen);
            mtx.lock(); // Блокировка мьютекса
            cout << "Mutex thread " << thread_id << ": " << random_char << endl;
            mtx.unlock();
        }
    }

    void generateRandomCharsSemaphore(int thread_id) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126);

        for (int i = 0; i < char_count; ++i) {
            sem.acquire(); // Захват семафора
            char random_char = dis(gen);
            cout << "Semaphore thread " << thread_id << ": " << random_char << endl;
            sem.release(); // Освобождение семафора
        }
    }

    void generateRandomCharsSemaphoreSlim(int thread_id) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126);

        for (int i = 0; i < char_count; ++i) {
            sem_slim.acquireSlim();
            char random_char = dis(gen);
            cout << "SemaphoreSlim thread " << thread_id << ": " << random_char << endl;
            sem_slim.releaseSlim(); // Исправлено на правильное имя метода
        }
    }

    void generateRandomCharsBarrier(int thread_id) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126);

        for (int i = 0; i < char_count; ++i) {
            char random_char = dis(gen);
            {
                lock_guard<mutex> lock(mtx);
                cout << "Barrier thread " << thread_id << ": " << random_char << endl;
            }
            my_barrier.arrive_and_wait(); // Ожидание других потоков
        }
    }

    void generateRandomCharsSpinLock(int thread_id) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126);

        for (int i = 0; i < char_count; ++i) {
            while (spinlock.test_and_set(memory_order_acquire));
            char random_char = dis(gen);
            cout << "SpinLock thread " << thread_id << ": " << random_char << endl;
            spinlock.clear(memory_order_release);
        }
    }
    
    void generateRandomCharsSpinWait(int thread_id) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126);

        for (int i = 0; i < char_count; ++i) {
            while (spinlock.test_and_set(memory_order_acquire)) {
                this_thread::yield();
            }
            char random_char = dis(gen);
            cout << "SpinWait thread " << thread_id << ": " << random_char << endl;
            spinlock.clear(memory_order_release);
        }
    }

    void generateRandomCharsMonitor(int thread_id) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(32, 126); // Генерация случайных символов из ASII таблицы

        for (int i = 0; i < char_count; ++i) {
            monitor.enter();
            char random_char = dis(gen);
            cout << "Monitor thread " << thread_id << ": " << random_char << endl;
        }
        spinlock.clear(memory_order_acquire);
        monitor.exit();
    }
};

int main() {
    thread threads[thread_count];
    StopWatch stopwatch;
    double time_taken;
    RandomCharGenerator generator;

    // Потоки с использованием mutex
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsMutex, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by Mutex threads: " << time_taken << " seconds" << endl;

    // Потоки с использованием semaphore
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsSemaphore, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by Semaphore threads: " << time_taken << " seconds" << endl;

    // Потоки с использованием semaphore slim
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsSemaphoreSlim, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by SemaphoreSlim threads: " << time_taken << " seconds" << endl;

    // Потоки с использованием barrier
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsBarrier, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by Barrier threads: " << time_taken << " seconds" << endl;

    // Потоки с использованием spinlock
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsSpinLock, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by SpinLock threads: " << time_taken << " seconds" << endl;

    // Потоки с использованием spinwait
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsSpinWait, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by SpinWait threads: " << time_taken << " seconds" << endl;

    // Потоки с использованием monitor
    stopwatch.start();
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(&RandomCharGenerator::generateRandomCharsMonitor, &generator, i);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    time_taken = stopwatch.elapsed();
    cout << "Time taken by Monitor threads: " << time_taken << " seconds" << endl;

    return 0;
}
