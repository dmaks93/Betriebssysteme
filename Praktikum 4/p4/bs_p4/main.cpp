#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <semaphore.h>
#include <queue>
#include <mutex>
#include <random>

using namespace std;

int numberOfWorkers {0}; // m
static int ingredientContainerCapacity {0}; // n
int frequency {0}; // r
static int numberOfCustomers {0}; // k
std::chrono::microseconds preparationTime {0}; // t

static int amountOfPatties {0};
static int piecesOfSalad {0};
static int slicesOfCucumber {0};
static int amountOfBuns {0};

static int numberOfServedCustomers {0};
static int amountOfBurgersMade {0};
double burgerTimeRatio {0.0};

queue <std::pair<std::pair<int,sem_t*>,int>> orders; // customer id, semaphore, order quantity

sem_t orderCapacity;
sem_t orderPlaced;

sem_t pattiesAvailiability;
sem_t saladAvailiability;
sem_t cucumberAvailiability;
sem_t bunsAvailiability;

std::mutex pattiesAccess;
std::mutex saladAccess;
std::mutex cucumberAccess;
std::mutex bunsAccess;

std::mutex ordersAccess;

void fillIngredients() {
    std::chrono::microseconds repetition = std::chrono::microseconds(1000000 / frequency);

    while (numberOfServedCustomers != numberOfCustomers) {

        pattiesAccess.lock();
        if (amountOfPatties < ingredientContainerCapacity) {
            amountOfPatties++;
            sem_post(&pattiesAvailiability);
        }
        pattiesAccess.unlock();

        saladAccess.lock();
        if (piecesOfSalad < ingredientContainerCapacity) {
            piecesOfSalad++;
            sem_post(&saladAvailiability);
        }
        saladAccess.unlock();

        cucumberAccess.lock();
        if (slicesOfCucumber < ingredientContainerCapacity) {
            slicesOfCucumber++;
            sem_post(&cucumberAvailiability);
        }
        cucumberAccess.unlock();

        bunsAccess.lock();
        if (amountOfBuns < ingredientContainerCapacity) {
            amountOfBuns++;
            sem_post(&bunsAvailiability);
        }
        bunsAccess.unlock();

        std::this_thread::sleep_for(repetition);
    }
}

void placeOrder(int customerId) {
    sem_t* customerSemaphore = new sem_t;
    sem_init(customerSemaphore, 0, 0);

    sem_wait(&orderCapacity);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 10);

    int orderQuantity = dist(gen);

    ordersAccess.lock();
    orders.push(std::make_pair(std::make_pair(customerId, customerSemaphore),orderQuantity));
    ordersAccess.unlock();

    sem_post(&orderPlaced);

    sem_wait(customerSemaphore);
}

void processOrder(int workerId) {
    while (true) {
        sem_wait(&orderPlaced);
        ordersAccess.lock();

        if (orders.empty()) {
            ordersAccess.unlock();
            sem_post(&orderPlaced);
            break;
        }

        int custId = orders.front().first.first;
        int ordQuantity = orders.front().second;
        sem_t* custSemaphore = orders.front().first.second;

        for (int k = 0; k < ordQuantity; k++) {

            pattiesAccess.lock();
            if (amountOfPatties == 0) {
                pattiesAccess.unlock();
                sem_wait(&pattiesAvailiability);
                pattiesAccess.lock();
                amountOfPatties--;
                pattiesAccess.unlock();
            } else {
                amountOfPatties--;
                sem_wait(&pattiesAvailiability);
                pattiesAccess.unlock();
            }

            saladAccess.lock();
            if (piecesOfSalad == 0) {
                saladAccess.unlock();
                sem_wait(&saladAvailiability);
                saladAccess.lock();
                piecesOfSalad--;
                saladAccess.unlock();
            } else {
                piecesOfSalad--;
                sem_wait(&saladAvailiability);
                saladAccess.unlock();
            }

            cucumberAccess.lock();
            if (slicesOfCucumber == 0) {
                cucumberAccess.unlock();
                sem_wait(&cucumberAvailiability);
                cucumberAccess.lock();
                slicesOfCucumber--;
                cucumberAccess.unlock();
            } else {
                slicesOfCucumber--;
                sem_wait(&cucumberAvailiability);
                cucumberAccess.unlock();
            }

            bunsAccess.lock();
            if (amountOfBuns == 0) {
                bunsAccess.unlock();
                sem_wait(&bunsAvailiability);
                bunsAccess.lock();
                amountOfBuns--;
                bunsAccess.unlock();
            } else {
                amountOfBuns--;
                sem_wait(&bunsAvailiability);
                bunsAccess.unlock();
            }

            std::this_thread::sleep_for(preparationTime);
        }
        sem_post(custSemaphore);
        numberOfServedCustomers++;
        orders.pop();
        amountOfBurgersMade = amountOfBurgersMade + ordQuantity;
        ordersAccess.unlock();
        sem_post(&orderCapacity);
        if (numberOfServedCustomers == numberOfCustomers) {
            sem_post(&orderPlaced);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Not all arguments provided, check your input! \n";
        return 1;
    }

    numberOfWorkers = std::stoi(argv[1]);
    ingredientContainerCapacity = std::stoi(argv[2]);
    frequency = std::stoi(argv[3]);
    numberOfCustomers = std::stoi(argv[4]);
    preparationTime = std::chrono::microseconds(std::stoll(argv[5]));

    amountOfPatties = ingredientContainerCapacity;
    piecesOfSalad = ingredientContainerCapacity;
    slicesOfCucumber = ingredientContainerCapacity;
    amountOfBuns = ingredientContainerCapacity;

    auto startTime = std::chrono::high_resolution_clock::now();

    sem_init(&orderCapacity, 0, numberOfWorkers);
    sem_init(&orderPlaced, 0, 0);

    sem_init (&pattiesAvailiability, 0, ingredientContainerCapacity);
    sem_init (&saladAvailiability, 0, ingredientContainerCapacity);
    sem_init (&cucumberAvailiability, 0, ingredientContainerCapacity);
    sem_init (&bunsAvailiability, 0, ingredientContainerCapacity);

    vector <std::thread> workers;
    for(int i = 1; i <= numberOfWorkers; i++)
        workers.push_back(std::thread (processOrder, i));

    vector <std::thread> customers;
    for (int j = 1; j <= numberOfCustomers; j++)
        customers.push_back(std::thread (placeOrder, j));

    std::thread fillingMachine (fillIngredients);

    for (int k = 0; k < workers.size(); k++)
        workers.at(k).join();

    for (int t = 0; t < customers.size(); t++)
        customers.at(t).join();

    fillingMachine.join();

    auto endTime = std::chrono::high_resolution_clock::now();

    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    double burgerTimeRatio = static_cast<double>(amountOfBurgersMade) / elapsedTime.count();

    cout << "Number of workers: " << numberOfWorkers << "   || Preparation time: " << preparationTime.count() << "ms \n";
    cout << amountOfBurgersMade << " Burger" << " / " << elapsedTime.count() << " Seconds || " << "Ratio of: " << burgerTimeRatio << "\n\n";

    sem_destroy(&orderCapacity);
    sem_destroy(&orderPlaced);
    sem_destroy(&pattiesAvailiability);
    sem_destroy(&saladAvailiability);
    sem_destroy(&cucumberAvailiability);
    sem_destroy(&bunsAvailiability);

    return 0;
}
