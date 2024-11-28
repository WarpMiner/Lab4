#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "timer/timer.h"

using namespace std;

class Network {
public:
    void shopAdd() {
        int n;
        cout << "Введите кол-во магазинов: ";
        cin >> n;

        for (int i = 0; i < n; i++) {
            Shop shop;

            // Населённый пункт
            char loc = 65 + rand() % 26;
            shop.locality = loc;

            // Улица
            char streets = 97 + rand() % 26;
            shop.street = streets;

            // Номер дома
            shop.house_num = 1 + rand() % 200;

            // ID магазина
            int id = 1 + rand() % 15;
            shop.id = "M" + to_string(id);

            shops.push_back(shop);
        }
    }

    void findStreet(int start, int end, unordered_map<string, unordered_set<string>>& localResult) {
        // Хранит улицы и соответствующие населенные пункты
        unordered_map<string, unordered_set<string>> streetMap;

        // Заполнение карты
        for (int i = start; i < end; ++i) {
            const auto& shop = shops[i];
            streetMap[shop.street].insert(shop.locality);
        }

        // Сохранение результата в локальный контейнер
        for (const auto& entry : streetMap) {
            if (entry.second.size() > 1) {
                localResult[entry.first] = entry.second; // Сохраняем результат
            }
        }
    }

    void printResults(const unordered_map<string, unordered_set<string>>& result) {
        cout << "Магазины на одинаковых улицах в разных населенных пунктах:" << endl;
        for (const auto& entry : result) {
            const string& street = entry.first;
            const unordered_set<string>& localities = entry.second;

            cout << "Улица: " << street << " - Населенные пункты: ";
            for (const auto& locality : localities) {
                cout << locality << ", ";
            }
            cout << endl;
        }
    }

    void print() {
        for (int i = 0; i < shops.size(); i++) {
            cout << i + 1 << ". г. " << shops[i].locality << ", ул. " << shops[i].street << ", дом " << shops[i].house_num << ", ID: " << shops[i].id << endl;
        }
    }

    struct Shop {
        string locality;
        string street;
        int house_num;
        string id;
    };

    vector<Shop> shops;
};

int main() {
    srand(time(0));

    Network network;
    cout << "Выберите действие:\n1. Добавить магазины\n2. Найти магазины\n3. Вывести магазины\n4. Выход\n---------------------" << endl;
    while (true) {
        string action;
        cout << ">> ";
        cin >> action;

        if (action == "1") {
            network.shopAdd();
        } else if (action == "2") {
            {
                Timer t;
                cout << "Однопоточная обработка: " << endl;
                unordered_map<string, unordered_set<string>> result;
                network.findStreet(0, network.shops.size(), result);
                network.printResults(result);
            }

            int countThread;
            cout << "Введите кол-во потоков: ";
            cin >> countThread;
            int size = network.shops.size();
            int chunkSize = size / countThread;
            vector<unordered_map<string, unordered_set<string>>> results(countThread); // Вектор для хранения результатов от каждого потока

            {
                Timer t;
                cout << endl << "Многопоточная обработка: " << endl;
                vector<thread> threads(countThread);
                for (int i = 0; i < countThread; ++i) {
                    int start = i * chunkSize;
                    int end = (i == countThread - 1) ? size : start + chunkSize; // Обработка последнего потока
                    threads[i] = thread([&network, start, end, &results, i]() { network.findStreet(start, end, results[i]); });
                }
                for (int i = 0; i < countThread; ++i) {
                    threads[i].join();
                }

                // Объединение результатов
                unordered_map<string, unordered_set<string>> finalResult;
                for (const auto& localResult : results) {
                    for (const auto& entry : localResult) {
                        finalResult[entry.first].insert(entry.second.begin(), entry.second.end());
                    }
                }
                network.printResults(finalResult);
            }

        } else if (action == "3") {
            network.print();
        } else if (action == "4") {
            cout << "Выход..." << endl;
            break;
        } else {
            cerr << "Такого действия нет -_-" << endl;
        }
    }

    return 0;
}
