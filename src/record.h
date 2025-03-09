#ifndef RECORD_H
#define RECORD_H

#include <iostream>

struct Record {
    int key;
    double mass;
    double specificHeat;
    double temperatureChange;
    int overflowPointer;
};

struct IndexRecord {
    int key;
    int pageNumber;
};

#endif //RECORD_H
