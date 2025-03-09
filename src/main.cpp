#include <iostream>
#include <fstream>

#include "file_operations.h"
#include "menu.h"

using namespace std;

int main() {
    ofstream indexStream("../index.dat", ios::in | ios::out | ios::binary);
    ofstream mainStream("../main.dat", ios::in | ios::out | ios::binary);
    ofstream overflowStream("../overflow.dat", ios::in | ios::out | ios::binary);

    if (!indexStream) {
        ofstream fileStream("../index.dat", ios::out | ios::binary);
    }
    if (!mainStream) {
        ofstream fileStream("../main.dat", ios::out | ios::binary);
    }
    if (!overflowStream) {
        ofstream fileStream("../overflow.dat", ios::out | ios::binary);
    }

    initIndex();
    initMain();

    indexStream.close();
    mainStream.close();
    overflowStream.close();

    runProgram();

    remove("../index.dat");
    remove("../main.dat");
    remove("../overflow.dat");

    return 0;
}
