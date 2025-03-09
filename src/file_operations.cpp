#include "file_operations.h"
#include <fstream>
#include <iostream>

#include "menu.h"

using namespace std;

void initIndex() {
    fstream indexStream("../index.dat", ios::in | ios::out | ios::binary);

    if (!indexStream) {
        cout << "Wystapil blad" << endl;
        return;
    }

    int tmp = 0;

    addIndexPage(&indexStream, &tmp, &tmp, 0, 1);

    indexStream.close();
}

void initMain() {
    fstream mainStream("../main.dat", ios::in | ios::out | ios::binary);

    if (!mainStream) {
        cout << "Wystapil blad" << endl;
        return;
    }

    Record record = {0, 0, 0, 0, -1};
    int tmp = 0;

    addPage(&mainStream, &tmp, &tmp, &record);

    mainStream.close();
}

template <typename T>
vector<T>* getPage(fstream* fileStream, int filePosition, int* accessCounter) {
    T tmpPage[PAGE_SIZE];

    fileStream->seekg(filePosition * PAGE_SIZE * sizeof(T), ios::beg);
    fileStream->read((char*)tmpPage, sizeof(T)*PAGE_SIZE);
    (*accessCounter)++;

    int recordsNumber = fileStream->gcount() / sizeof(T);

    auto* page = new vector<T>;
    for(int i = 0; i < recordsNumber; i++) {
        page->push_back(tmpPage[i]);
    }

    fileStream->clear();

    return page;
}

Record* readRecord() {
    auto* record = new Record;
    cout << "Podaj klucz: " << endl;
    cin >> record->key;
    cout << "Podaj wartosc masy: ";
    cin >> record->mass;
    cout << "Podaj wartosc ciepla wlasciwego: ";
    cin >> record->specificHeat;
    cout << "Podaj wartosc roznicy temperatur: ";
    cin >> record->temperatureChange;

    record->overflowPointer = -1;

    return record;
}

int insertRecord(int* mainRecords, int* overflowRecords, Record* record, bool isUpdate) {
    fstream indexStream("../index.dat", ios::in | ios::out | ios::binary);
    fstream mainStream("../main.dat", ios::in | ios::out | ios::binary);
    fstream overflowStream("../overflow.dat", ios::in | ios::out | ios::binary);

    if (!indexStream || !mainStream || !overflowStream) {
        cout << "Wystapil blad" << endl;
        return -1;
    }

    if (record->key == 1600) {
        int x;
    }

    int indexFilePosition = 0;
    int mainFilePosition = 0;
    int overflowFilePosition = 0;

    int beg, end;

    int accessCounter = 0;

    int selectedPage = 0;
    bool isOverflow = false;
    int overflowFrom = -1;

    vector<IndexRecord>* indexPage = nullptr;
    vector<Record>* mainPage = nullptr;
    vector<Record>* overflowPage = nullptr;

    findPage(&indexStream, &indexFilePosition, &accessCounter, record->key, &selectedPage);

    mainFilePosition = selectedPage - 1;
    mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);

    for (int i = 0; i < mainPage->size(); i++) {
        if (mainPage->at(i).key == record->key) {
            if (!isUpdate)
                cout << "Rekord o podanym kluczu juz istnieje." << endl;

            delete mainPage;
            indexStream.close();
            mainStream.close();
            overflowStream.close();
            return -1;
        }
    }

    int smallerIndex = 0;
    int largerIndex = mainPage->size() - 1;

    for (int i = 0; i < mainPage->size(); i++) {
        // ostatni mniejszy klucz, który nie jest pustym rekordem
        if (mainPage->at(i).key != -1 && mainPage->at(i).key < record->key) {
            smallerIndex = i;
        }
        // pierwszy większy klucz
        if (mainPage->at(i).key > record->key) {
            largerIndex = i;
            break;
        }
    }

    if (smallerIndex != mainPage->size() - 1) {     // ostatni mniejszy klucz nie jest ostatnim kluczem na stronie
        for (int i = smallerIndex; i <= largerIndex; i++) {
            if (mainPage->at(i).key == -1) {    // pierwszy pusty rekord
                if (mainPage->at(i).overflowPointer == -1) {    // jeśli nie jest usunięty rekord ze wskaźnikiem
                    mainPage->at(i) = (*record);

                    if (i == 0) {   // aktualizacja indeksu w przypadku pierwszego rekordu na stronie
                        indexFilePosition = (selectedPage - 1) / PAGE_SIZE;
                        indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);
                        int index = -1;
                        for (int j = 0; j < indexPage->size(); j++) {
                            if (indexPage->at(j).pageNumber == selectedPage) {
                                index = j;
                                break;
                            }
                        }
                        indexPage->at(index).key = record->key;
                        IndexRecord tempRecord[PAGE_SIZE];
                        for (int i = 0; i < PAGE_SIZE; i++)
                            tempRecord[i] = indexPage->at(i);
                        indexStream.seekg(indexFilePosition*sizeof(IndexRecord)*PAGE_SIZE, ios::beg);
                        indexStream.write((char*)tempRecord, sizeof(IndexRecord)*PAGE_SIZE);
                    }

                    delete record;
                    record = nullptr;

                    Record tmpPage[PAGE_SIZE];
                    for (int j = 0; j < PAGE_SIZE; j++)
                        tmpPage[j] = mainPage->at(j);

                    mainStream.seekg(mainFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                    mainStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                    accessCounter++;
                    (*mainRecords)++;
                    break;
                }

                // rekord posiada wskaźnik na przepełnienie
                overflowFilePosition = mainPage->at(i).overflowPointer / PAGE_SIZE;
                overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

                if (overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key > record->key) {
                    record->overflowPointer = mainPage->at(i).overflowPointer;
                    mainPage->at(i) = (*record);

                    if (i == 0) {   // aktualizacja indeksu w przypadku pierwszego rekordu na stronie
                        indexFilePosition = (selectedPage - 1) / PAGE_SIZE;
                        indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);
                        int index = -1;
                        for (int j = 0; j < indexPage->size(); j++) {
                            if (indexPage->at(j).pageNumber == selectedPage) {
                                index = j;
                                break;
                            }
                        }
                        indexPage->at(index).key = record->key;
                        IndexRecord tempRecord[PAGE_SIZE];
                        for (int i = 0; i < PAGE_SIZE; i++)
                            tempRecord[i] = indexPage->at(i);
                        indexStream.seekg(indexFilePosition*sizeof(IndexRecord)*PAGE_SIZE, ios::beg);
                        indexStream.write((char*)tempRecord, sizeof(IndexRecord)*PAGE_SIZE);
                    }

                    delete record;
                    record = nullptr;

                    Record tmpPage[PAGE_SIZE];
                    for (int j = 0; j < PAGE_SIZE; j++)
                        tmpPage[j] = mainPage->at(j);

                    mainStream.seekg(mainFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                    mainStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                    accessCounter++;
                    (*mainRecords)++;
                    break;
                }

                // wskaźnik w rekordzie wskazuje na mniejszy klucz
                isOverflow = true;
                overflowFrom = i;
            }
        }
        if (record != nullptr && !isOverflow) {    // nie znaleziono wolnego miejsca pomiędzy odpowiednimi rekordami
            isOverflow = true;
            overflowFrom = smallerIndex;
        }
    }
    else {  // wszystkie klucze na stronie są mniejsze od wprowadzanego klucza
        mainStream.seekg(0, ios::beg);
        beg = mainStream.tellg();
        mainStream.seekg(0, ios::end);
        end = mainStream.tellg();

        if (selectedPage == (end - beg) / sizeof(Record) / PAGE_SIZE) {  // wybrana strona jest ostatnia
            addPage(&mainStream, &mainFilePosition, &accessCounter, record);

            indexStream.seekg(0, ios::beg);
            beg = indexStream.tellg();
            indexStream.seekg(0, ios::end);
            end = indexStream.tellg();

            indexFilePosition = (end - beg) / sizeof(IndexRecord) / PAGE_SIZE - 1;
            delete indexPage;
            indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);

            int emptyElement = -1;
            for (int i = 0; i < indexPage->size(); i++) {
                if (indexPage->at(i).key == -1) {
                    emptyElement = i;
                    break;
                }
            }

            if (emptyElement != -1) {   // znaleziono wolne miejsce na stronie
                indexPage->at(emptyElement).key = record->key;
                indexPage->at(emptyElement).pageNumber = selectedPage + 1;

                IndexRecord tmpPage[PAGE_SIZE];
                for (int i = 0; i < PAGE_SIZE; i++) {
                    tmpPage[i] = indexPage->at(i);
                }
                indexStream.seekg(indexFilePosition * sizeof(IndexRecord) * PAGE_SIZE, ios::beg);
                indexStream.write((char*)tmpPage, PAGE_SIZE*sizeof(IndexRecord));
                accessCounter++;
            }
            else
                addIndexPage(&indexStream, &indexFilePosition, &accessCounter, record->key, selectedPage + 1);

            (*mainRecords)++;
            delete record;
        }
        else {  // do części przepełnień od ostatniego klucza na wybranej stronie
            isOverflow = true;
            overflowFrom = smallerIndex;
        }
    }

    if (isOverflow) {
        beg = overflowStream.tellg();
        overflowStream.seekg(0, ios::end);
        end = overflowStream.tellg();
        int overflowPages = (end - beg) / sizeof(Record) / PAGE_SIZE;
        overflowFilePosition = overflowPages == 0 ? 0 : overflowPages - 1;

        overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
        int index = -1;
        for (int i = 0; i < overflowPage->size(); i++) {
            if (overflowPage->at(i).key == -1) {
                index = i;
                break;
            }
        }

        if (index != -1) {  // wstawianie rekordu w wolne miejsce na ostatniej stronie
            overflowPage->at(index) = (*record);
            Record tmpPage[PAGE_SIZE];
            for (int i = 0; i < PAGE_SIZE; i++) {
                tmpPage[i] = overflowPage->at(i);
            }
            overflowStream.seekg(overflowFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
            overflowStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
            accessCounter++;
        }
        else {  // brak wolnych miejsc
            addPage(&overflowStream, &overflowFilePosition, &accessCounter, record);
            index = 0;
        }

        (*overflowRecords)++;

        // przypisanie wskaźnika na rekord do rekordu w części głównej
        if (mainPage->at(overflowFrom).overflowPointer == -1) {
            mainPage->at(overflowFrom).overflowPointer = (overflowFilePosition * PAGE_SIZE) + index;
            Record tmpPage[PAGE_SIZE];
            for (int i = 0; i < PAGE_SIZE; i++) {
                tmpPage[i] = mainPage->at(i);
            }
            mainStream.seekg(mainFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
            mainStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
            accessCounter++;
        }
        else {  // rekord w części głównej już ma wskaźnik na przepełnienie
            int newRecordIndex = overflowFilePosition * PAGE_SIZE + index;
            Record prevRecord = mainPage->at(overflowFrom);
            int prevIndex = -1;
            bool isPrevMain = true;

            while (true) {  // szukanie miejsca zmiany wskaźników
                overflowFilePosition = prevRecord.overflowPointer / PAGE_SIZE;
                overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

                // jeśli rekord o podanym kluczu już istnieje
                if (overflowPage->at(prevRecord.overflowPointer % PAGE_SIZE).key == record->key) {
                    if (!isUpdate)
                        cout << "Rekord o podanym kluczu juz istnieje" << endl;

                    if (overflowFilePosition != newRecordIndex / PAGE_SIZE) {
                        overflowFilePosition = newRecordIndex / PAGE_SIZE;
                        overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
                    }
                    overflowPage->at(newRecordIndex % PAGE_SIZE).key = -1;
                    Record tmpPage[PAGE_SIZE];
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        tmpPage[i] = overflowPage->at(i);
                    }
                    overflowStream.seekg(overflowFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                    overflowStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                    accessCounter++;
                    (*overflowRecords)--;
                    if (isUpdate)
                        accessCounter = -1;
                    break;
                }

                // jeśli wskazywany rekord posiada klucz większy od wprowadzanego
                if (overflowPage->at(prevRecord.overflowPointer % PAGE_SIZE).key > record->key) {
                    // pobranie na co wskazywał poprzedni rekord, aby przypisać to do wprowadzanego rekordu
                    int newRecordOverflowPointer = prevRecord.overflowPointer;

                    // przypisanie do poprzedniego rekordu wskaźnika na wprowadzony rekord
                    if (isPrevMain) {   // jeśli poprzedni rekord znajduje się w części głównej
                        mainPage->at(overflowFrom).overflowPointer = newRecordIndex;
                        Record tmpPage[PAGE_SIZE];
                        for (int i = 0; i < PAGE_SIZE; i++) {
                            tmpPage[i] = mainPage->at(i);
                        }
                        mainStream.seekg(mainFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                        mainStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                        accessCounter++;
                    }
                    else {  // jeśli poprzedni rekord jest w części przepełnień
                        if (overflowFilePosition != prevIndex / PAGE_SIZE) {
                            overflowFilePosition = prevIndex / PAGE_SIZE;
                            overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
                        }
                        overflowPage->at(prevIndex % PAGE_SIZE).overflowPointer = newRecordIndex;
                        Record tmpPage[PAGE_SIZE];
                        for (int i = 0; i < PAGE_SIZE; i++) {
                            tmpPage[i] = overflowPage->at(i);
                        }
                        overflowStream.seekg(overflowFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                        overflowStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                        accessCounter++;
                    }

                    // odczytanie strony z nowym rekordem, jeśli nie jest odczytana
                    if (overflowFilePosition != newRecordIndex / PAGE_SIZE) {
                        overflowFilePosition = newRecordIndex / PAGE_SIZE;
                        overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
                    }

                    // przypisanie do wprowadzonego rekordu wskaźnika na rekord o większym kluczu
                    overflowPage->at(newRecordIndex % PAGE_SIZE).overflowPointer = newRecordOverflowPointer;
                    Record tmpPage[PAGE_SIZE];
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        tmpPage[i] = overflowPage->at(i);
                    }
                    overflowStream.seekg(overflowFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                    overflowStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                    accessCounter++;
                    break;
                }

                // obecny klucz jest mniejszy od klucza wprowadzonego rekordu i nie wskazuje na nic
                if (overflowPage->at(prevRecord.overflowPointer % PAGE_SIZE).overflowPointer == -1) {
                    overflowPage->at(prevRecord.overflowPointer % PAGE_SIZE).overflowPointer = newRecordIndex;
                    Record tmpPage[PAGE_SIZE];
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        tmpPage[i] = overflowPage->at(i);
                    }
                    overflowStream.seekg(overflowFilePosition * sizeof(Record) * PAGE_SIZE, ios::beg);
                    overflowStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
                    accessCounter++;
                    break;
                }

                // obecny klucz jest mniejszy od klucza wprowadzonego rekordu i wskazuje na jakiś rekord
                prevIndex = prevRecord.overflowPointer;
                prevRecord = overflowPage->at(prevRecord.overflowPointer % PAGE_SIZE);
                isPrevMain = false;
            }
        }

        delete record;
    }

    delete indexPage;
    delete mainPage;
    delete overflowPage;

    indexStream.close();
    mainStream.close();
    overflowStream.close();

    if (((float)(*overflowRecords) / (float)(*mainRecords) >= REORG_COND) && AUTO_REORG) {
        cout << "\nWykonanie reorganizacji pliku." << endl;
        //accessCounter += reorganize(mainRecords, overflowRecords);
    }

    if (!isUpdate) {
        cout << "\nLiczba dostepow do dysku: " << accessCounter << endl;
        cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
        cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;
    }

    return accessCounter;
}

void insertRecordOption(int* mainRecords, int* overflowRecords) {
    string answer;
    bool ifDisplay = false;

    while (true) {
        cout << "Czy wyswietlic zawartosc pliku po operacji? (t/n)" << endl;
        cin >> answer;
        if (answer == "t") {
            ifDisplay = true;
            break;
        }
        if (answer == "n") {
            ifDisplay = false;
            break;
        }

        cout << "Niepoprawna odpowiedz" << endl;
    }

    Record* record = readRecord();

    if (record->key <= 0) {
        cout << "Niepoprawna wartosc klucza" << endl;
        delete record;
    }

    insertRecord(mainRecords, overflowRecords, record);

    if (ifDisplay)
        displayFile();
}

void addPage(fstream *mainStream, int* mainFilePosition, int* accessCounter, Record* record) {
    mainStream->seekg(0, ios::beg);
    int beg = mainStream->tellg();
    mainStream->seekg(0, ios::end);
    int end = mainStream->tellg();

    (*mainFilePosition) = (end - beg) / sizeof(Record) / PAGE_SIZE;

    Record tmpBlock[PAGE_SIZE];

    tmpBlock[0] = *record;

    for (int i = 1; i < PAGE_SIZE; i++) {
        tmpBlock[i].key = -1;
        tmpBlock[i].mass = 0;
        tmpBlock[i].specificHeat = 0;
        tmpBlock[i].temperatureChange = 0;
        tmpBlock[i].overflowPointer = -1;
    }

    mainStream->write((char*)tmpBlock, PAGE_SIZE*sizeof(Record));
    (*accessCounter)++;
}

void addIndexPage(fstream *indexStream, int* indexFilePosition, int* accessCounter, int key, int pageNumber) {
    indexStream->seekg(0, ios::beg);
    int beg = indexStream->tellg();
    indexStream->seekg(0, ios::end);
    int end = indexStream->tellg();

    (*indexFilePosition) = (end-beg) / sizeof(IndexRecord) / PAGE_SIZE;

    IndexRecord tmpBlock[PAGE_SIZE];

    tmpBlock[0] = {key, pageNumber};

    for (int i = 1; i < PAGE_SIZE; i++) {
        tmpBlock[i].key = -1;
        tmpBlock[i].pageNumber = 0;
    }

    indexStream->write((char*)tmpBlock, PAGE_SIZE*sizeof(IndexRecord));
    (*accessCounter)++;
}

void displayRecord() {
    int key;
    cout << "Podaj klucz rekordu: ";
    cin >> key;

    if (key <= 0) {
        cout << "Niepoprawny klucz." << endl;
        return;
    }

    fstream indexStream("../index.dat", ios::in | ios::out | ios::binary);
    fstream mainStream("../main.dat", ios::in | ios::out | ios::binary);
    fstream overflowStream("../overflow.dat", ios::in | ios::out | ios::binary);

    if (!indexStream || !mainStream || !overflowStream) {
        cout << "Wystapil blad" << endl;
        return;
    }

    string noExistText = "Rekord o podanej wartosci klucza nie istnieje.";

    int indexFilePosition = 0;

    int accessCounter = 0;
    int selectedPage = -1;
    int selectedOverflow = -2;

    findPage(&indexStream, &indexFilePosition, &accessCounter, key, &selectedPage);

    int mainFilePosition = selectedPage - 1;
    vector<Record>* mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);
    int overflowFilePosition = 0;
    vector<Record>* overflowPage = nullptr;
    Record tmpPage[PAGE_SIZE];

    for (int i = 0; i < mainPage->size(); i++) {
        if (mainPage->at(i).key == key) {
            cout << mainPage->at(i).key << " " << mainPage->at(i).mass << " " << mainPage->at(i).specificHeat << " " << mainPage->at(i).temperatureChange << endl;
            cout << "Liczba dostepow do dysku: " << accessCounter << endl;
            delete mainPage;
            indexStream.close();
            mainStream.close();
            overflowStream.close();
            return;
        }

        if (mainPage->at(i).key == -1 && mainPage->at(i).overflowPointer != -1) {
            overflowFilePosition = mainPage->at(i).overflowPointer / PAGE_SIZE;
            overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

            if (overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key == key) {
                cout << overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key << " " << overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).mass << " " << overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).specificHeat << " " << overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).temperatureChange << endl;
                cout << "Liczba dostepow do dysku: " << accessCounter << endl;
                delete mainPage;
                delete overflowPage;
                indexStream.close();
                mainStream.close();
                overflowStream.close();
                return;
            }

            if (overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key > key)
                break;

            selectedOverflow = mainPage->at(i).overflowPointer;
        }

        if (mainPage->at(i).key > key) {
            selectedOverflow = mainPage->at(i - 1).overflowPointer;
            break;
        }
    }

    if (selectedOverflow == -2)
        selectedOverflow = mainPage->at(mainPage->size() - 1).overflowPointer;

    if (selectedOverflow == -1) {
        cout << noExistText << endl;
        delete mainPage;
        indexStream.close();
        mainStream.close();
        overflowStream.close();
        return;
    }

    overflowFilePosition = selectedOverflow / PAGE_SIZE;
    overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

    while (true) {
        if (overflowPage->at(selectedOverflow % PAGE_SIZE).key == key) {
            cout << overflowPage->at(selectedOverflow % PAGE_SIZE).key << " " << overflowPage->at(selectedOverflow % PAGE_SIZE).mass << " " << overflowPage->at(selectedOverflow % PAGE_SIZE).specificHeat << " " << overflowPage->at(selectedOverflow % PAGE_SIZE).temperatureChange << endl;
            cout << "Liczba dostepow do dysku: " << accessCounter << endl;
            break;
        }

        if (overflowPage->at(selectedOverflow % PAGE_SIZE).key > key) {
            cout << noExistText << endl;
            break;
        }

        if (overflowPage->at(selectedOverflow % PAGE_SIZE).overflowPointer == -1) {
            cout << noExistText << endl;
            break;
        }

        selectedOverflow = overflowPage->at(selectedOverflow % PAGE_SIZE).overflowPointer;
        overflowFilePosition = selectedOverflow / PAGE_SIZE;
        delete overflowPage;
        overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
    }

    delete mainPage;
    delete overflowPage;

    indexStream.close();
    mainStream.close();
    overflowStream.close();
}

void displayFile() {
    fstream indexStream("../index.dat", ios::in | ios::out | ios::binary);
    fstream mainStream("../main.dat", ios::in | ios::out | ios::binary);
    fstream overflowStream("../overflow.dat", ios::in | ios::out | ios::binary);

    if (!indexStream || !mainStream || !overflowStream) {
        cout << "Wystapil blad" << endl;
        return;
    }

    int indexFilePosition = 0;
    int mainFilePosition = 0;
    int overflowFilePosition = 0;

    int accessCounter = 0;

    vector<IndexRecord>* indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);
    vector<Record>* mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);
    vector<Record>* overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

    cout << "\nStruktury plikow\n";

    cout << "Indeks" << endl;
    cout << "klucz | nr strony" << endl;
    while (true) {
        if (indexPage->empty())
            break;

        cout << "Strona " << indexFilePosition + 1 << endl;
        for (int i = 0; i < indexPage->size(); i++) {
            if (indexPage->at(i).key == -1) {
                cout << "<puste>" << endl;
            }
            else {
                cout << indexPage->at(i).key << " " << indexPage->at(i).pageNumber << endl;
            }
        }

        indexFilePosition++;
        delete indexPage;
        indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);
    }

    cout << "\nGlowny plik" << endl;
    cout << "klucz | masa | cieplo wlasciwe | roznica temperatur | wskaznik przepelnien" << endl;
    while (true) {
        if (mainPage->empty())
            break;

        cout << "Strona " << mainFilePosition + 1 << endl;
        for (int i = 0; i < mainPage->size(); i++) {
            if (mainPage->at(i).key == -1 && mainPage->at(i).overflowPointer == -1) {
                cout << "<puste>" << endl;
            }
            else if (mainPage->at(i).key == -1 && mainPage->at(i).overflowPointer != -1) {
                cout << "<usuniete>" << endl;
            }
            else {
                cout << mainPage->at(i).key << " " << mainPage->at(i).mass << " " << mainPage->at(i).specificHeat << " " << mainPage->at(i).temperatureChange << " ";
                if (mainPage->at(i).overflowPointer == -1)
                    cout << "-" << endl;
                else
                    cout << mainPage->at(i).overflowPointer << endl;
            }
        }

        mainFilePosition++;
        delete mainPage;
        mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);
    }

    cout << "\nCzesc przepelnien" << endl;
    cout << "klucz | masa | cieplo wlasciwe | roznica temperatur | wskaznik przepelnien" << endl;
    while (true) {
        if (overflowPage->empty())
            break;

        cout << "Strona " << overflowFilePosition + 1 << endl;
        for (int i = 0; i < overflowPage->size(); i++) {
            if (overflowPage->at(i).key == -1) {
                cout << "<puste>" << endl;
            }
            else if (overflowPage->at(i).key == -1 && overflowPage->at(i).overflowPointer != -1) {
                cout << "<usuniete>" << endl;
            }
            else {
                cout << overflowPage->at(i).key << " " << overflowPage->at(i).mass << " " << overflowPage->at(i).specificHeat << " " << overflowPage->at(i).temperatureChange << " ";
                if (overflowPage->at(i).overflowPointer == -1)
                    cout << "-" << endl;
                else
                    cout << overflowPage->at(i).overflowPointer << endl;
            }
        }

        overflowFilePosition++;
        delete overflowPage;
        overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
    }

    cout << "\nRekordy zgodnie z kolejnoscia wartosci klucza" << endl;
    cout << "klucz | masa | cieplo wlasciwe | roznica temperatur | pochodzenie" << endl;

    mainFilePosition = 0;
    overflowFilePosition = 0;

    mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);
    overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

    int mainIndex = 0;
    bool isOverflow = false;

    Record prev;

    while (true) {
        if (mainPage->empty())
            break;

        if (!isOverflow) {
            if (mainPage->at(mainIndex).key != -1)
                cout << mainPage->at(mainIndex).key << " " << mainPage->at(mainIndex).mass << " " << mainPage->at(mainIndex).specificHeat << " " << mainPage->at(mainIndex).temperatureChange << " main" << endl;

            if (mainPage->at(mainIndex).overflowPointer != -1) {
                isOverflow = true;
                prev = mainPage->at(mainIndex);
            }

            mainIndex++;
        }
        else {
            if (overflowFilePosition != prev.overflowPointer / PAGE_SIZE) {
                overflowFilePosition = prev.overflowPointer / PAGE_SIZE;
                overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
            }

            cout << overflowPage->at(prev.overflowPointer % PAGE_SIZE).key << " " << overflowPage->at(prev.overflowPointer % PAGE_SIZE).mass << " " << overflowPage->at(prev.overflowPointer % PAGE_SIZE).specificHeat << " " << overflowPage->at(prev.overflowPointer % PAGE_SIZE).temperatureChange << " overflow" << endl;

            if (overflowPage->at(prev.overflowPointer % PAGE_SIZE).overflowPointer != -1)
                prev = overflowPage->at(prev.overflowPointer % PAGE_SIZE);
            else
                isOverflow = false;
        }

        if (mainIndex == PAGE_SIZE) {
            mainIndex = 0;
            mainFilePosition++;
            mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);
        }
    }

    delete indexPage;
    delete mainPage;
    delete overflowPage;

    indexStream.close();
    mainStream.close();
    overflowStream.close();
}

void findPage(fstream* indexStream, int* indexFilePosition, int* accessCounter, int key, int* selectedPage) {
    vector<IndexRecord>* indexPage = getPage<IndexRecord>(indexStream, *indexFilePosition, accessCounter);

    bool found = false;
    int beg, end;

    while (true) {
        for (int i = 0; i < indexPage->size(); i++) {
            // jeśli znaleziono większy klucz lub pusty rekord to wybiera stronę wskazaną przez poprzedni klucz
            if (indexPage->at(i).key > key || indexPage->at(i).key == -1) {
                found = true;
                (*selectedPage) = i == 0 ? -1 : indexPage->at(i - 1).pageNumber;
                break;
            }
        }

        if (found) {
            // jeśli wskazano poprzedni klucz
            if ((*selectedPage) == -1) {
                (*indexFilePosition)--;
                delete indexPage;
                indexPage = getPage<IndexRecord>(indexStream, *indexFilePosition, accessCounter);
                (*selectedPage) = indexPage->at(indexPage->size() - 1).pageNumber;
                break;
            }

            // jeśli wskazano już konkretną stronę
            break;
        }

        indexStream->seekg(0, ios::beg);
        beg = indexStream->tellg();
        indexStream->seekg(0, ios::end);
        end = indexStream->tellg();

        // jeśli wskaźnik w pliku przeszedł już ostatnią stronę w indeksie
        if (((*indexFilePosition) + 1) == (end - beg) / sizeof(IndexRecord) / PAGE_SIZE) {
            (*selectedPage) = indexPage->at(indexPage->size() - 1).pageNumber;
            break;
        }

        (*indexFilePosition)++;
        delete indexPage;
        indexPage = getPage<IndexRecord>(indexStream, *indexFilePosition, accessCounter);
    }

    delete indexPage;
}

int reorganize(int *mainRecords, int *overflowRecords) {
    rename("../index.dat", "../index_old.dat");
    rename("../main.dat", "../main_old.dat");
    rename("../overflow.dat", "../overflow_old.dat");

    fstream mainStreamOld("../main_old.dat", ios::in | ios::out | ios::binary);
    fstream overflowStreamOld("../overflow_old.dat", ios::in | ios::out | ios::binary);

    if (!mainStreamOld || !overflowStreamOld) {
        cout << "Wystapil blad" << endl;
        return 0;
    }

    fstream indexStream("../index.dat", ios::out | ios::binary);
    indexStream.close();
    fstream mainStream("../main.dat", ios::out | ios::binary);
    mainStream.close();
    fstream overflowStream("../overflow.dat", ios::out | ios::binary);
    overflowStream.close();

    indexStream.open("../index.dat", ios::in | ios::out | ios::binary);
    mainStream.open("../main.dat", ios::in | ios::out | ios::binary);
    overflowStream.open("../overflow.dat", ios::in | ios::out | ios::binary);

    if (!indexStream || !mainStream || !overflowStream) {
        cout << "Wystapil blad" << endl;
        return 0;
    }

    int accessCounter = 0;

    int oldMainFilePosition = 0;
    int oldOverflowFilePosition = 0;

    int indexFilePosition = 0;

    vector<Record>* mainOldPage = getPage<Record>(&mainStreamOld, oldMainFilePosition, &accessCounter);
    vector<Record>* overflowOldPage = getPage<Record>(&overflowStreamOld, oldOverflowFilePosition, &accessCounter);

    vector<IndexRecord>* indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);


    vector<Record> tmpVector;
    Record tmpPage[PAGE_SIZE];

    for (int i = 0; i < PAGE_SIZE; i++)
        tmpPage[i] = {-1, 0, 0, 0, -1};

    int mainIndex = 0;
    bool isOverflow = false;
    int pages = 0;

    Record prev;
    Record tmpRecord;

    while (true) {
        if (mainOldPage->empty())
            break;

        if (!isOverflow) {
            if (mainOldPage->at(mainIndex).key != -1) {
                tmpRecord = mainOldPage->at(mainIndex);
                tmpRecord.overflowPointer = -1;
                tmpVector.push_back(tmpRecord);
            }

            if (mainOldPage->at(mainIndex).overflowPointer != -1) {
                isOverflow = true;
                prev = mainOldPage->at(mainIndex);
            }

            mainIndex++;
        }
        else {
            if (oldOverflowFilePosition != prev.overflowPointer / PAGE_SIZE) {  // czy szukana strona nie jest już odczytana
                oldOverflowFilePosition = prev.overflowPointer / PAGE_SIZE;
                delete overflowOldPage;
                overflowOldPage = getPage<Record>(&overflowStreamOld, oldOverflowFilePosition, &accessCounter);
            }

            tmpRecord = overflowOldPage->at(prev.overflowPointer % PAGE_SIZE);
            tmpRecord.overflowPointer = -1;
            tmpVector.push_back(tmpRecord);

            if (overflowOldPage->at(prev.overflowPointer % PAGE_SIZE).overflowPointer != -1)
                prev = overflowOldPage->at(prev.overflowPointer % PAGE_SIZE);
            else
                isOverflow = false;
        }

        if (mainIndex == PAGE_SIZE) {
            mainIndex = 0;
            oldMainFilePosition++;
            delete mainOldPage;
            mainOldPage = getPage<Record>(&mainStreamOld, oldMainFilePosition, &accessCounter);
        }

        if (tmpVector.size() >= PAGE_SIZE * ALPHA) {
            pages++;
            int empty = -1;
            for (int i = 0; i < indexPage->size(); i++) {
                if (indexPage->at(i).key == -1) {
                    empty = i;
                    break;
                }
            }
            if (empty != -1) {
                indexPage->at(empty).key = tmpVector.at(0).key;
                indexPage->at(empty).pageNumber = pages;
                IndexRecord tmpIndex[PAGE_SIZE];
                for (int i = 0; i < PAGE_SIZE; i++)
                    tmpIndex[i] = indexPage->at(i);
                indexStream.seekg(indexFilePosition*sizeof(IndexRecord)*PAGE_SIZE, ios::beg);
                indexStream.write((char*)tmpIndex, sizeof(IndexRecord)*PAGE_SIZE);
            }
            else {
                addIndexPage(&indexStream, &indexFilePosition, &accessCounter, tmpVector.at(0).key, pages);
                indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);
            }

            for (int i = 0; i < tmpVector.size(); i++)
                tmpPage[i] = tmpVector.at(i);

            tmpVector.clear();
            mainStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
            accessCounter++;

            for (int i = 0; i < PAGE_SIZE; i++)
                tmpPage[i] = {-1, 0, 0, 0, -1};
        }
    }

    if (!tmpVector.empty()) {
        pages++;
        int empty = -1;
        for (int i = 0; i < indexPage->size(); i++) {
            if (indexPage->at(i).key == -1) {
                empty = i;
                break;
            }
        }
        if (empty != -1) {
            indexPage->at(empty).key = tmpVector.at(0).key;
            indexPage->at(empty).pageNumber = pages;
            IndexRecord tmpIndex[PAGE_SIZE];
            for (int i = 0; i < PAGE_SIZE; i++)
                tmpIndex[i] = indexPage->at(i);
            indexStream.seekg(indexFilePosition*sizeof(IndexRecord)*PAGE_SIZE, ios::beg);
            indexStream.write((char*)tmpIndex, sizeof(IndexRecord)*PAGE_SIZE);
        }
        else {
            addIndexPage(&indexStream, &indexFilePosition, &accessCounter, tmpVector.at(0).key, pages);
            indexPage = getPage<IndexRecord>(&indexStream, indexFilePosition, &accessCounter);
        }

        for (int i = 0; i < tmpVector.size(); i++)
            tmpPage[i] = tmpVector.at(i);

        mainStream.write((char*)tmpPage, PAGE_SIZE*sizeof(Record));
        accessCounter++;
    }

    (*mainRecords) += (*overflowRecords);
    (*overflowRecords) = 0;

    delete mainOldPage;
    delete overflowOldPage;
    delete indexPage;

    mainStreamOld.close();
    overflowStreamOld.close();
    indexStream.close();
    mainStream.close();
    overflowStream.close();

    remove("../index_old.dat");
    remove("../main_old.dat");
    remove("../overflow_old.dat");

    return accessCounter;
}

void reorganizeOption(int* mainRecords, int* overflowRecords) {
    string answer;
    bool ifDisplay = false;

    while (true) {
        cout << "Czy wyswietlic zawartosc pliku po operacji? (t/n)" << endl;
        cin >> answer;
        if (answer == "t") {
            ifDisplay = true;
            break;
        }
        if (answer == "n") {
            ifDisplay = false;
            break;
        }

        cout << "Niepoprawna odpowiedz" << endl;
    }

    int accessCounter = reorganize(mainRecords, overflowRecords);

    if (ifDisplay)
        displayFile();

    cout << "Liczba dostepow do dysku: " << accessCounter << endl;
    cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
    cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;
}

int deleteRecord(int *mainRecords, int *overflowRecords, int key, bool isUpdate) {
    fstream indexStream("../index.dat", ios::in | ios::out | ios::binary);
    fstream mainStream("../main.dat", ios::in | ios::out | ios::binary);
    fstream overflowStream("../overflow.dat", ios::in | ios::out | ios::binary);

    if (!indexStream || !mainStream || !overflowStream) {
        cout << "Wystapil blad" << endl;
        return -1;
    }

    string noExistText = "Rekord o podanej wartosci klucza nie istnieje.";

    int indexFilePosition = 0;

    int accessCounter = 0;
    int selectedPage = -1;
    Record prev;
    int prevIndex = -1;

    findPage(&indexStream, &indexFilePosition, &accessCounter, key, &selectedPage);

    int mainFilePosition = selectedPage - 1;
    vector<Record>* mainPage = getPage<Record>(&mainStream, mainFilePosition, &accessCounter);
    int overflowFilePosition = 0;
    vector<Record>* overflowPage = nullptr;
    Record tmpPage[PAGE_SIZE];

    for (int i = 0; i < mainPage->size(); i++) {
        if (mainPage->at(i).key == key) {
            mainPage->at(i).key = -1;
            for (int j = 0; j < PAGE_SIZE; j++)
                tmpPage[j] = mainPage->at(j);
            mainStream.seekg(mainFilePosition*sizeof(Record)*PAGE_SIZE, ios::beg);
            mainStream.write((char*)tmpPage, sizeof(Record)*PAGE_SIZE);

            (*mainRecords)--;
            if (!isUpdate) {
                cout << "Usunieto" << endl;
                cout << "\nLiczba dostepow do dysku: " << accessCounter << endl;
                cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
                cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;
            }
            delete mainPage;
            indexStream.close();
            mainStream.close();
            overflowStream.close();
            return accessCounter;
        }

        if (mainPage->at(i).key == -1 && mainPage->at(i).overflowPointer != -1) {
            overflowFilePosition = mainPage->at(i).overflowPointer / PAGE_SIZE;
            overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

            if (overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key == key) {
                overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key = -1;
                for (int j = 0; j < PAGE_SIZE; j++)
                    tmpPage[j] = overflowPage->at(j);
                overflowStream.seekg(overflowFilePosition*sizeof(Record)*PAGE_SIZE, ios::beg);
                overflowStream.write((char*)tmpPage, sizeof(Record)*PAGE_SIZE);

                mainPage->at(i).overflowPointer = overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).overflowPointer;
                for (int j = 0; j < PAGE_SIZE; j++)
                    tmpPage[j] = mainPage->at(j);
                mainStream.seekg(mainFilePosition*sizeof(Record)*PAGE_SIZE, ios::beg);
                mainStream.write((char*)tmpPage, sizeof(Record)*PAGE_SIZE);

                (*mainRecords)--;
                if (!isUpdate) {
                    cout << "Usunieto" << endl;
                    cout << "\nLiczba dostepow do dysku: " << accessCounter << endl;
                    cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
                    cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;
                }
                delete mainPage;
                delete overflowPage;
                indexStream.close();
                mainStream.close();
                overflowStream.close();
                return accessCounter;
            }

            if (overflowPage->at(mainPage->at(i).overflowPointer % PAGE_SIZE).key > key)
                break;

            prev = mainPage->at(i);
            prevIndex = i;
        }

        if (mainPage->at(i).key > key) {
            prev = mainPage->at(i - 1);
            prevIndex = i - 1;
            break;
        }
    }

    if (prevIndex == -1) {
        prev = mainPage->at(mainPage->size() - 1);
        prevIndex = mainPage->size() - 1;
    }

    if (prev.overflowPointer == -1) {
        if (!isUpdate)
            cout << noExistText << endl;
        delete mainPage;
        indexStream.close();
        mainStream.close();
        overflowStream.close();
        return -1;
    }

    bool isPrevMain = true;
    overflowFilePosition = prev.overflowPointer / PAGE_SIZE;
    overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);

    while (true) {
        if (overflowPage->at(prev.overflowPointer % PAGE_SIZE).key == key) {
            overflowPage->at(prev.overflowPointer % PAGE_SIZE).key = -1;
            for (int i = 0; i < PAGE_SIZE; i++)
                tmpPage[i] = overflowPage->at(i);
            overflowStream.seekg(overflowFilePosition*sizeof(Record)*PAGE_SIZE, ios::beg);
            overflowStream.write((char*)tmpPage, sizeof(Record)*PAGE_SIZE);

            prev.overflowPointer = overflowPage->at(prev.overflowPointer % PAGE_SIZE).overflowPointer;

            if (isPrevMain) {
                mainPage->at(prevIndex) = prev;
                for (int i = 0; i < PAGE_SIZE; i++)
                    tmpPage[i] = mainPage->at(i);
                mainStream.seekg(mainFilePosition*sizeof(Record)*PAGE_SIZE, ios::beg);
                mainStream.write((char*)tmpPage, sizeof(Record)*PAGE_SIZE);
            }
            else {
                if (overflowFilePosition != prevIndex / PAGE_SIZE) {
                    overflowFilePosition = prevIndex / PAGE_SIZE;
                    overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
                }
                overflowPage->at(prevIndex % PAGE_SIZE) = prev;
                for (int i = 0; i < PAGE_SIZE; i++)
                    tmpPage[i] = overflowPage->at(i);
                overflowStream.seekg(overflowFilePosition*sizeof(Record)*PAGE_SIZE, ios::beg);
                overflowStream.write((char*)tmpPage, sizeof(Record)*PAGE_SIZE);
            }

            (*overflowRecords)--;
            if (!isUpdate) {
                cout << "Usunieto" << endl;
                cout << "\nLiczba dostepow do dysku: " << accessCounter << endl;
                cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
                cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;
            }
            break;
        }

        if (overflowPage->at(prev.overflowPointer % PAGE_SIZE).key > key) {
            if (!isUpdate)
                cout << noExistText << endl;
            accessCounter = -1;
            break;
        }

        if (overflowPage->at(prev.overflowPointer % PAGE_SIZE).overflowPointer == -1) {
            if (!isUpdate)
                cout << noExistText << endl;
            accessCounter = -1;
            break;
        }

        // klucz mniejszy od wprowadzonego i wskaźnik wskazuje na kolejny rekord
        prevIndex = prev.overflowPointer;
        prev = overflowPage->at(prev.overflowPointer % PAGE_SIZE);
        overflowFilePosition = prev.overflowPointer / PAGE_SIZE;
        delete overflowPage;
        overflowPage = getPage<Record>(&overflowStream, overflowFilePosition, &accessCounter);
        isPrevMain = false;
    }

    delete mainPage;
    delete overflowPage;

    indexStream.close();
    mainStream.close();
    overflowStream.close();

    return accessCounter;
}

void deleteRecordOption(int *mainRecords, int *overflowRecords) {
    string answer;
    bool ifDisplay = false;

    while (true) {
        cout << "Czy wyswietlic zawartosc pliku po operacji? (t/n)" << endl;
        cin >> answer;
        if (answer == "t") {
            ifDisplay = true;
            break;
        }
        if (answer == "n") {
            ifDisplay = false;
            break;
        }

        cout << "Niepoprawna odpowiedz" << endl;
    }

    int key;
    cout << "Podaj klucz rekordu: ";
    cin >> key;

    if (key <= 0) {
        cout << "Niepoprawny klucz." << endl;
        return;
    }

    deleteRecord(mainRecords, overflowRecords, key);

    if (ifDisplay)
        displayFile();
}

void updateRecord(int* mainRecords, int* overflowRecords) {
    string answer;
    bool ifDisplay = false;

    while (true) {
        cout << "Czy wyswietlic zawartosc pliku po operacji? (t/n)" << endl;
        cin >> answer;
        if (answer == "t") {
            ifDisplay = true;
            break;
        }
        if (answer == "n") {
            ifDisplay = false;
            break;
        }

        cout << "Niepoprawna odpowiedz" << endl;
    }

    int key;
    cout << "Podaj klucz rekordu do zaktualizowania: ";
    cin >> key;

    if (key <= 0) {
        cout << "Niepoprawny klucz." << endl;
        return;
    }

    int accessCounter = 0;

    accessCounter = deleteRecord(mainRecords, overflowRecords, key, true);

    if (accessCounter == -1) {
        cout << "Rekord o podanym kluczu nie istnieje." << endl;
        return;
    }

    cout << "Wprowadzanie nowego rekordu" << endl;
    Record* record = readRecord();

    if (record->key <= 0) {
        cout << "Niepoprawna wartosc klucza" << endl;
        delete record;
    }

    int tmp = insertRecord(mainRecords, overflowRecords, record, true);

    if (tmp == -1) {
        cout << "Rekord o podanym kluczu juz istnieje." << endl;
        return;
    }

    accessCounter += tmp;

    cout << "\nLiczba dostepow do dysku: " << accessCounter << endl;
    cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
    cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;

    if (ifDisplay)
        displayFile();
}

void readFromFile(int *mainRecords, int *overflowRecords) {
    /*
     *  Format pliku testowego:
     *  <opcja> <dane>
     *  gdzie:
     *  wstawianie - 1, usuwanie - 2, aktualizacja - 3
     *
     *  1   klucz   masa   cieplo  temperatura
     *  2   klucz
     *  3   klucz_stary  klucz_nowy   masa   cieplo   temperatura
     */

    string answer;
    bool ifDisplay = false;

    while (true) {
        cout << "Czy wyswietlic zawartosc pliku po operacjach? (t/n)" << endl;
        cin >> answer;
        if (answer == "t") {
            ifDisplay = true;
            break;
        }
        if (answer == "n") {
            ifDisplay = false;
            break;
        }

        cout << "Niepoprawna odpowiedz" << endl;
    }

    fstream testFile("../test.txt", ios::in);

    if (!testFile) {
        cout << "Plik testowy nie istnieje." << endl;
        return;
    }

    int option, key, key2, accessCounter;
    double mass, specificHeat, temperatureChange;
    Record* record = nullptr;

    while (testFile >> option) {
        switch (option) {
            case 1:     // wstawianie
                testFile >> key >> mass >> specificHeat >> temperatureChange;
                cout << "\nWstawianie rekordu o kluczu " << key << endl;

                if (key <= 0) {
                    cout << "Niepoprawny klucz." << endl;
                    continue;
                }

                record = new Record(key, mass, specificHeat, temperatureChange, -1);
                accessCounter = insertRecord(mainRecords, overflowRecords, record, true);

                if (accessCounter == -1)
                    cout << "Rekord o podanym kluczu juz istnieje." << endl;
                else
                    cout << "Liczba dostepow do dysku: " << accessCounter << endl;

                break;
            case 2:     // usuwanie
                testFile >> key;
                cout << "\nUsuwanie rekordu o kluczu " << key << endl;

                if (key <= 0) {
                    cout << "Niepoprawny klucz." << endl;
                    continue;
                }
                accessCounter = deleteRecord(mainRecords, overflowRecords, key, true);

                if (accessCounter == -1)
                    cout << "Rekord o podanym kluczu nie istnieje." << endl;
                else
                    cout << "Liczba dostepow do dysku: " << accessCounter << endl;

                break;
            case 3:     // aktualizacja
                testFile >> key >> key2 >> mass >> specificHeat >> temperatureChange;
                cout << "\nAktualizowanie rekordu o kluczu " << key << endl;

                if (key <= 0 || key2 <= 0) {
                    cout << "Niepoprawny klucz." << endl;
                    continue;
                }

                accessCounter = deleteRecord(mainRecords, overflowRecords, key, true);
                if (accessCounter == -1)
                    cout << "Rekord o podanym kluczu nie istnieje." << endl;

                record = new Record(key2, mass, specificHeat, temperatureChange, -1);
                int tmp = insertRecord(mainRecords, overflowRecords, record, true);

                if (tmp == -1)
                    cout << "Rekord o podanym kluczu nie istnieje." << endl;
                else
                    cout << "Liczba dostepow do dysku: " << accessCounter + tmp << endl;

                break;
        }
    }

    cout << "\nKoniec pliku testowego" << endl;
    cout << "Liczba rekordow w czesci glownej: " << (*mainRecords) << endl;
    cout << "Liczba rekordow w czesci przepelnien: " << (*overflowRecords) << endl;

    if (ifDisplay)
        displayFile();
}
