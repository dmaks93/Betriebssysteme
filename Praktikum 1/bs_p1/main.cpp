#include <iostream>
#include <sys/resource.h>
#include <fstream>
#include <cctype>

using std::cout;
using std::cin;
using std::endl;
using std::ifstream;
using std::string;

static int counter {0}; // funcRec

const int arraySize {1410065000}; // funcMem
const int funcMemOutputFrequency {1000};
const int numberOfCalls {104600}; // funcRec
const int funcRecOutputFrequency {1000};

void storedMemoryInfo () {
    ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        string line;
        string charValue;
        char c;
        int realValue {};
        std::getline(meminfo, line);
        for (int i = 0; i < line.size(); i++) {
            c = line.at(i);
            if (isdigit(c))
                charValue = charValue + c;
        }
        realValue = stoi(charValue);
        realValue = realValue / 1024.0;
        cout << "RAM at disposal: " << realValue << " MB" << endl;
        meminfo.close();
    }
}

void timeUsageProvider () {
    struct rusage timeUsage;
    getrusage (RUSAGE_SELF, &timeUsage);
    cout << "User CPU Time: " << timeUsage.ru_utime.tv_sec << "." << timeUsage.ru_utime.tv_usec << " seconds" << endl;
    cout << "System CPU Time: " << timeUsage.ru_stime.tv_sec << "." << timeUsage.ru_stime.tv_usec << " seconds" << endl;
}

void ramUsageProvider () {
    struct rusage memUsage;
    getrusage(RUSAGE_SELF, &memUsage);
    cout << "RAM usage: " << memUsage.ru_maxrss / 1024.0 << " MB" << endl;
}

void availableStackProvider () {
    struct rlimit memLimit;
    getrlimit(RLIMIT_STACK, &memLimit);
    cout << "STACK at disposal (getrlimit): " << memLimit.rlim_cur / (1024.0 * 1024.0) << " MB" << endl;
    storedMemoryInfo();
}

void funcMem (int size, int freq) {
    int* arr = new int [size];
    for (int i = 0; i < size; i++) {
        arr [i] = i;
        if (((i + 1) % freq) == 0) {
            cout << i+1 << ":" << endl;
            timeUsageProvider();
            ramUsageProvider();
        }
    }
    delete[] arr;
    ramUsageProvider();
    availableStackProvider();
}

void funcRec (int recursions, char* stackStartPointer, int freq) {
    counter++;
    if (counter == recursions) {
        char stackEnd;
        char* stackEndPointer {&stackEnd};
        double estimatedStack = stackStartPointer - stackEndPointer;
        timeUsageProvider();
        availableStackProvider();
        cout << "Estimated stack usage: " << estimatedStack / (1024.0 * 1024.0) << " MB" << endl;
    }
    if (counter < recursions) {
        if ((counter % freq) == 0) {
            char stackCurrent;
            char* stackCurrentPointer {&stackCurrent};
            double estimatedStack = stackStartPointer - stackCurrentPointer;
            timeUsageProvider();
            cout << "Estimated current stack usage: " << estimatedStack / (1024.0 * 1024.0) << " MB" << endl;
        }
        funcRec(recursions, stackStartPointer, freq);
    }
}

int main() {

    int choice {};

    while (true) {
        cout << "1 --> funcMem" << endl;
        cout << "2 --> funcRec" << endl;
        cout << "3 --> exit" << endl;
        cout << "Your choice is: ";
        cin >> choice;
        if (choice == 1)
            funcMem (arraySize, funcMemOutputFrequency);
        if (choice == 2) {
            char stackStart {};
            funcRec(numberOfCalls, &stackStart, funcRecOutputFrequency);
        }
        if (choice == 3) {
            cout << "Bye" << endl;
            break;
        }
    }

    return 0;
}
