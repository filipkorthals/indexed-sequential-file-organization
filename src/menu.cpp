#include "menu.h"
#include <iostream>
#include <string>

#include "file_operations.h"

using namespace std;

void display() {
    cout << "\nWybierz opcje:" << endl;
    cout << "1. Wstaw rekord" << endl;
    cout << "2. Odczytaj rekord" << endl;
    cout << "3. Wyswietl plik" << endl;
    cout << "4. Reorganizuj plik" << endl;
    cout << "5. Usun rekord" << endl;
    cout << "6. Aktualizuj rekord" << endl;
    cout << "7. Wprowadz z pliku" << endl;
    cout << "8. Wyjscie" << endl;
}

void runProgram() {
    int mainRecords = 1;
    int overflowRecords = 0;

    string input;
    int selection = 0;

    while (selection != 8) {
        display();
        cin >> input;

        selection = atoi(input.c_str());

        switch (selection) {
            case 1:
                insertRecordOption(&mainRecords, &overflowRecords);
            break;
            case 2:
                displayRecord();
            break;
            case 3:
                displayFile();
            break;
            case 4:
                reorganizeOption(&mainRecords, &overflowRecords);
            break;
            case 5:
                deleteRecordOption(&mainRecords, &overflowRecords);
            break;
            case 6:
                updateRecord(&mainRecords, &overflowRecords);
            break;
            case 7:
                readFromFile(&mainRecords, &overflowRecords);
            break;
            case 8:
                cout << "Wyjscie" << endl;
            break;
            default:
                cout << "Niepoprawna opcja" << endl;
            break;
        }
    }
}

