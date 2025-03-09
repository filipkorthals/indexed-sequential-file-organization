#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#define PAGE_SIZE 4
#define ALPHA 0.5
#define REORG_COND 0.25
#define AUTO_REORG true

#include <vector>
#include "record.h"

using namespace std;

void initIndex();
void initMain();
template <typename T> vector<T>* getPage(fstream* fileStream, int filePosition, int* accessCounter);
Record* readRecord();
int insertRecord(int* mainRecords, int* overflowRecords, Record* record, bool isUpdate = false);
void insertRecordOption(int* mainRecords, int* overflowRecords);
void addPage(fstream* mainStream, int* mainFilePosition, int* accessCounter, Record* record);
void addIndexPage(fstream* indexStream, int* indexFilePosition, int* accessCounter, int key, int pageNumber);
void displayRecord();
void displayFile();
void findPage(fstream* indexStream, int* indexFilePosition, int* accessCounter, int key, int* selectedPage);
int reorganize(int* mainRecords, int* overflowRecords);
void reorganizeOption(int* mainRecords, int* overflowRecords);
int deleteRecord(int* mainRecords, int* overflowRecords, int key, bool isUpdate = false);
void deleteRecordOption(int* mainRecords, int* overflowRecords);
void updateRecord(int* mainRecords, int* overflowRecords);
void readFromFile(int* mainRecords, int* overflowRecords);


#endif //FILE_OPERATIONS_H
