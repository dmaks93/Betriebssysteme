#include <iostream>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>

using namespace::std;

static bool processInBackground {};
vector<pid_t> backgroundProcesses;
pid_t currentForegroundProcess {0};
pid_t currentParentPID {0};

void stopForegroundProcess(int signum) {
    if (currentForegroundProcess != 0) {
        cout << "Stopping foreground process: " << currentForegroundProcess << "\n";
        kill(currentForegroundProcess, signum);
    }
}

void stopBackgroundProcess (pid_t bckProcessToStop) {
    for (pid_t bckProcess : backgroundProcesses) {
        if (bckProcess == bckProcessToStop) {
            cout << "Stopping background process: " << bckProcess << "\n";
            kill(bckProcess, SIGTSTP);
            break;
        }
    }
}

void resumeStoppedProcess (pid_t processToResume) {
    bool resumeInBackground = false;
    for (pid_t pid : backgroundProcesses) {
        if (pid == processToResume) {
            resumeInBackground = true;
            break;
        }
    }
    if (resumeInBackground)
        kill(processToResume, SIGCONT);

    if (!resumeInBackground) {
        kill(processToResume, SIGCONT);
        waitpid(processToResume, NULL, WUNTRACED);
    }
}

void handleBackgroundProcesses(int signum) {
    int status;
    pid_t backgroundProcess;

    while ((backgroundProcess = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            cout << "Background process " << backgroundProcess << " terminated with exit status: " << WEXITSTATUS(status) << "\n";

        } else if (WIFSIGNALED(status)) {
            cout << "Background process " << backgroundProcess << " terminated by signal: " << WTERMSIG(status) << "\n";
        }
        for (int i = 0; i < backgroundProcesses.size(); i++) {
            if (backgroundProcesses.at(i) == backgroundProcess) {
                backgroundProcesses.erase(backgroundProcesses.begin() + i);
                break;
            }
        }
    }
}

void handleExitInterrupt(int signum) {

    for (pid_t pid : backgroundProcesses) {
        cout << "Stopping background process: " << pid << "\n";
        kill(pid, SIGTERM);
    }
    if (!backgroundProcesses.empty()) {
        for (pid_t pid : backgroundProcesses) {
            kill(pid, SIGKILL);
        }
    }

    if (currentForegroundProcess != 0)
        kill(currentForegroundProcess, SIGTERM);
    if (currentForegroundProcess != 0)
        kill(currentForegroundProcess, SIGKILL);

    exit(EXIT_SUCCESS);
}

void printBackgroundProcesses() {
    if (backgroundProcesses.size() != 0)
        cout << "Background processes: ";
    for (pid_t pid : backgroundProcesses) {
        cout << pid << ", ";
    }
    if (backgroundProcesses.size() == 0)
        cout << "No background processes \n";
}

void inputHandler(string& input) {
    processInBackground = {false};
    int spacePosition {};
    const char* path {};
    string instruction {};
    vector <string> instructions {};

    if (input.back() == '&') {
        processInBackground = {true};
        input.pop_back();
    }
    if (input.find(' ') != -1) {
        spacePosition = input.find(' ');
        path = (input.substr(0, spacePosition)).c_str();
    }
    else {
        path = input.c_str();
    }
    istringstream separator(input);

    while (separator >> instruction) {
        instructions.push_back(instruction);
    }
    char* arguments[instructions.size() + 1];

    for (int i = 0; i < instructions.size(); i++) {
        arguments[i] = const_cast<char*>(instructions[i].c_str());
    }
    arguments[instructions.size()] = nullptr;

    cout << "Parent process ID: " << getpid() << "\n";
    currentParentPID = getppid();
    pid_t pid = fork();

    if (pid == -1)
        cerr << "Error while forkin process \n";

    if (pid == 0) {
        setpgid(getpid(), getpid());
        execvp(path, arguments);
        cerr << "Error while executing child process \n";
        exit(EXIT_FAILURE);
    }
    else {
        if (!processInBackground) {
            currentForegroundProcess = pid;
            waitpid(pid, NULL, WUNTRACED);
            cout << "Foreground process ID: " << pid << "\n";
        }
        else {
            cout << "Background process ID: " << pid << "\n";
            backgroundProcesses.push_back(pid);
        }

        path = nullptr;
        for (int i = 0; i < instructions.size(); i++) {
            arguments[i] = nullptr;
        }

        printBackgroundProcesses();
        cout << "\n";
    }
}

int main() {
    signal(SIGTSTP, stopForegroundProcess);
    signal(SIGCHLD, handleBackgroundProcesses);
    signal(SIGINT, handleExitInterrupt);

    string input {};

    while (true) {
        cout << "Shell> ";
        std::getline(cin,input);

        if (input == "exit") {
            char choice {};
            cout << "Terminate program and all processes? (y/n): \n";
            cin >> choice;
            if (choice == 'y') {
                if(backgroundProcesses.size() != 0) {
                    cout << "There are background processes \n";
                    for (int i = 0; i < backgroundProcesses.size(); i++) {
                        cout << "Process " << backgroundProcesses.at(i) << "\n";
                    }
                    cout << "Either wait for them to finish or terminate program using ctrl + c \n";
                }
                while (backgroundProcesses.size() != 0) {
                    if (backgroundProcesses.size() == 0) {
                        cout << "No more fun for you \n";
                        return 0;
                    }
                }
            }
            if (choice == 'n') {
                cout << "Have fun \n";
                input.clear();
            }
        }
        if (input.substr(0, 4) == "stop") {
            pid_t pidToStop = stoi(input.substr(4));
            stopBackgroundProcess(pidToStop);
            continue;
        }
        if (input.substr(0, 4) == "cont") {
            pid_t pidToResume = stoi(input.substr(4));
            resumeStoppedProcess(pidToResume);
            continue;
        }
        else if (input.size() != 0) {
            inputHandler(input);
        }
    }
    return 0;
}
