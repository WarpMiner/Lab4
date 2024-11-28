#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>

using namespace std;

class WriterAndReader {
public:
    WriterAndReader(bool writerPriority) 
        : writerCount(0), readerCount(0), priority(writerPriority) {}

    void startWriting() {
        unique_lock<mutex> lock(mx);
        if (priority) {
            cv.wait(lock, [this]() { return writerCount == 0; });
        } else {
            cv.wait(lock, [this]() { return readerCount == 0 && writerCount == 0; });
        }
        writerCount++;
    }

    void stopWriting() {
        unique_lock<mutex> lock(mx);
        writerCount--;
        if (writerCount == 0) {
            cv.notify_all();
        }
    }

    void startReading() {
        unique_lock<mutex> lock(mx);
        if (!priority) {
            readerCount++;
        } else {
            cv.wait(lock, [this]() { return writerCount == 0; });
            readerCount++;
        }
    }

    void stopReading() {
        unique_lock<mutex> lock(mx);
        readerCount--;
        cv.notify_all();
    }

    void setPriority(bool newPriority) {
        unique_lock<mutex> lock(mx);
        priority = newPriority;
    }

private:
    mutex mx;
    condition_variable cv;
    int writerCount; // Количество активных писателей
    int readerCount; // Количество активных читателей
    bool priority;   // Приоритет (писатели - true, читатели - false)
};

void writer(WriterAndReader& rw, int id, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        rw.startWriting();
        cout << "Писатель " << id << " пишет" << endl;
        this_thread::sleep_for(chrono::milliseconds(1000)); // Симуляция записи
        rw.stopWriting();
    }
}

void reader(WriterAndReader& rw, int id, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        rw.startReading();
        cout << "Читатель " << id << " читает" << endl;
        this_thread::sleep_for(chrono::milliseconds(1000)); // Симуляция чтения
        rw.stopReading();
    }
}

int main() {
    WriterAndReader rw(true);
    const int iterations = 1;
    const int writerCount = 6;
    const int readerCount = 2;

    cout << endl << "Приоритет писателей: " << endl;
    vector<thread> writers(writerCount);
    vector<thread> readers(readerCount);

    for (int i = 0; i < writerCount; ++i) {
        writers[i] = thread(writer, ref(rw), i + 1, iterations);
    }
    for (int i = 0; i < readerCount; ++i) {
        readers[i] = thread(reader, ref(rw), i + 1, iterations);
    }

    for (auto& th : writers) {
        th.join();
    }
    for (auto& th : readers) {
        th.join();
    }

    rw.setPriority(false);

    cout << endl << "Приоритет читателей: " << endl;
    writers.clear();
    readers.clear();

    for (int i = 0; i < writerCount; ++i) {
        writers.emplace_back(writer, ref(rw), i + 1, iterations);
    }
    for (int i = 0; i < readerCount; ++i) {
        readers.emplace_back(reader, ref(rw), i + 1, iterations);
    }

    for (auto& th : writers) {
        th.join();
    }
    for (auto& th : readers) {
        th.join();
    }

    return 0;
}
