//
// Created by Ahmedou Ectewechny on 23.11.23.
//
#include <iostream>
#include <sys/types.h>
#include <unistd.h>     // Für execvp
#include <sys/wait.h>   // Für waitpid
#include <vector>
#include <string>
using namespace std;


vector<string> eingabeToken(string& eingabe){
    string teil="";
    vector<string> token;
    for(unsigned int i=0; i<eingabe.size(); i++)
    {
        if(eingabe[i] != ' '){
            teil += eingabe[i];
        }
        else {
            token.push_back(teil);
            teil="";
        }
    }
    token.push_back(teil);
    return token;
}

int execute(string& eingabe){
    vector<string>input = eingabeToken(eingabe);
    vector<char*> arguments;
    arguments.reserve(input.size()+1);
    for(const string& i: input){
        arguments.push_back((const_cast<char*>(i.c_str())));
    }
    arguments.push_back(nullptr);
    return execvp(arguments[0], arguments.data());
}


void backGroundpidAusgabe(vector<int>& pids){
    for(auto pid:pids){
        cout<<"pid : "<<pid<<endl;
    }
}

void eingabeAusführen(string& eingabe) {


    char lastchar;
    bool background = false;
    pid_t pid;
    vector<int> backgroundPids;
    int runexe;
    lastchar = eingabe.back();
    if (lastchar == '&') {
        background = true;
        eingabe.pop_back();
    }

    pid = fork();
    cout <<"pid : "<<pid<<endl;
    if (pid < 0) {
        cerr << "Fehler beim Forken des Prozesses." << endl;
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Kindprozess
        runexe = execute(eingabe);
        if (runexe == -1) {
            cout << "Fehler beim Ausführen des Befehls." << endl;
            exit(-1);
        }
    } else{
        if(!background){
            waitpid(pid , nullptr ,0);
        }else {
            // Im Hintergrund weitermachen und die PID speichern
            cout << "Prozess im Hintergrund gestartet: " << pid << endl;
            backgroundPids.push_back(pid);
            background = false;
        }

    }

    backGroundpidAusgabe(backgroundPids);
}




int main() {

    string eingabe ;

    while (true) {
        cout << "$";
        getline(cin, eingabe);
        // Überprüfen, ob der Benutzer "exit" eingegeben hat
        if (eingabe == "exit") {
            cout << "Moechten Sie die Shell wirklich beenden? [Y/N]: ";
            char response;
            cin >> response;
            if (response == 'Y' || response == 'y') {
                return 0;
            } else {
                cin.ignore(); // leert den Eingabepuffer, um unerwünschte Zeichen zu vermeiden
                continue;
            }

        }    else if(eingabe.size() == 0){
            continue;
        }
        else {
            eingabeAusführen(eingabe);
        }
    }
    return 0;
}
