#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>

#define MAX_SET_SIZE 15
#define BUFOR_SIZE 4

using namespace std;

struct Record {
    int key;
    double numbers[MAX_SET_SIZE];
    int pointer;
    bool first = 0;

    void empty() {
        key = -1;
        for (int i = 0; i < MAX_SET_SIZE; i++)
            numbers[i] = 0;
        pointer = -1;
    }
};

struct Index {
    int key;
    int page_number;

    void first() {
        key = -1;
        page_number = 0;
    }
};

int overflow_pages = 0;
int primary_pages = 0;
vector<Index> all_index;
int readings = 0;
int writtings = 0;

struct IndexFile {
    Index bufor[BUFOR_SIZE];
    string file_name = { "index.txt" };
    int last = 0;
    bool end = false;
    int place = 0;
    int in_bufor = 0;
    int place_to_write = 0;

    void create() {
        ofstream file(file_name);
        Index first_index;
        first_index.first();
        file.write((char*)&first_index, sizeof(Index));
        file.close();
    }

    void read() {
        readings++;
        end = false;
        ifstream file(file_name, std::ios::binary | std::ios::out | std::ios::in);
        file.seekg(place);
        in_bufor = 0;
        for (in_bufor = 0; in_bufor < BUFOR_SIZE; in_bufor++) {
            Index index;
            file.read((char*)&index, sizeof(Index));
            if (!file.eof()) {
                bufor[in_bufor] = index;
            }
            else {
                end = true;
                break;
            }
        }
        if (file.eof())
            place = 0;
        else
            place = file.tellg();
        file.close();
    }

    void write_page() {
        writtings++;
        ofstream file(file_name, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(place_to_write);
        if (last == BUFOR_SIZE)
            file.write((char*)bufor, sizeof(Index) * BUFOR_SIZE);
        else {
            for (int i = 0; i < last; i++) {
                file.write((char*)&bufor[i], sizeof(Index));
            }
        }
        place_to_write = file.tellp();
        file.close();
        last = 0;
    }


    void insert(int key, int page_number) {
        bufor[last].key = key;
        bufor[last].page_number = page_number;
        last++;
        if (last == BUFOR_SIZE) {
            write_page();
        }
    }
};



void createVector() {
    all_index.clear();
    IndexFile file;
    while (!file.end) {
        file.read();
        for (int i = 0; i < file.in_bufor; i++) {
            all_index.push_back(file.bufor[i]);
        }
    }
    file.in_bufor = 0;
}


struct DataFile {
    string file_name = "data.txt";
    Record bufor[BUFOR_SIZE];
    int pages_read = 0;
    int actual = 0;

    void new_bufor() {
        Record record;
        record.empty();
        for (int i = 0; i < BUFOR_SIZE; i++) {
            bufor[i] = record;
        }
    }

    void create() {
        ofstream file(file_name);
        new_bufor();
        bufor[0].first = 1;
        file.write((char*)bufor, sizeof(Record) * BUFOR_SIZE);
        bufor[0].first = 0;
        overflow_pages = 1;
        primary_pages = 1;
        file.write((char*)bufor, sizeof(Record) * BUFOR_SIZE);
        file.close();
    }

    void show() {
        ifstream file(file_name, std::ios::binary | std::ios::out | std::ios::in);
        file.seekg(0);
        cout << "Plik z danymi" << endl;
        int page_number = 1;
        while (!file.eof()) {
            file.read((char*)bufor, sizeof(Record) * BUFOR_SIZE);
            if (file.eof())
                break;
            cout << "Numer strony: " << page_number << endl;
            for (int i = 0; i < BUFOR_SIZE; i++) {
                cout << "Klucz: " << bufor[i].key << " zbior: ";
                for (int j = 0; j < MAX_SET_SIZE; j++) {
                    cout << bufor[i].numbers[j] << " ";
                }
                cout << "wskaznik: " << bufor[i].pointer << " czy pierwszy: " << bufor[i].first << endl;
            }
            page_number++;
            if (file.tellg() == primary_pages * BUFOR_SIZE * sizeof(Record)) {
                cout << "Overflow:" << endl;
                page_number = 1;
            }
        }
        file.close();
    }

    Record getRecord(DataFile& overflow, int pointer, int& page) {
        if (pointer != -1) {
            int overflow_page = pointer / BUFOR_SIZE + primary_pages;
            int place = pointer % BUFOR_SIZE;
            if (overflow_page != page) {
                overflow.read_page(overflow_page);
                page = overflow_page;
            }
            return overflow.bufor[place];

        }
        if (pages_read > primary_pages) {
            Record empty;
            empty.key = -2;
            return empty;
        }
        actual++;
        if (bufor[actual - 1].key == -1 && bufor[actual - 1].first != 1) {
            read_page(pages_read);
            actual = 0;
            return getRecord(overflow, -1, page);
        }
        if (actual == BUFOR_SIZE) {
            Record end_record = bufor[actual - 1];
            actual = 0;
            read_page(pages_read);
            return end_record;
        }
        return bufor[actual - 1];
    }

    void moveAndAdd(Record record, int record_index) {
        for (int j = BUFOR_SIZE - 1; j > record_index; j--) {
            bufor[j] = bufor[j - 1];
        }
        bufor[record_index] = record;
    }

    void copy_bufor(Record bufor1[BUFOR_SIZE], Record bufor2[BUFOR_SIZE]) {
        for (int i = 0; i < BUFOR_SIZE; i++)
            bufor1[i] = bufor2[i];
    }


    void reorganization() {
        double alfa = 0.5;
        int records = 0;
        DataFile data;
        DataFile new_data;
        new_data.file_name = "data2.txt";
        ofstream new_file("data2.txt");
        new_file.close();
        IndexFile index;
        DataFile overflow;
        int page = 0;
        int records_in_page = 0;
        int overflow_page = 0;
        new_data.new_bufor();
        Record record;
        int pointer = -1;
        data.read_page(0);
        int primary_pages_new = 0;

        while (true) {
            record = data.getRecord(overflow, pointer, overflow_page);
            pointer = record.pointer;
            record.pointer = -1;
            if (records_in_page == alfa * BUFOR_SIZE) {
                index.insert(new_data.bufor[0].key, page);
                new_data.write_page(page);
                primary_pages_new++;
                records_in_page = 0;
                page++;
                new_data.new_bufor();
            }
            if (record.key == -2) {
                if (records_in_page != 0) {
                    index.insert(new_data.bufor[0].key, page);
                    new_data.write_page(page);
                    primary_pages_new++;
                }
                break;
            }
            new_data.bufor[records_in_page] = record;
            records_in_page++;
        }
        int overflow_pages_new = ceil(primary_pages_new * 0.2);
        for (int i = 0; i < overflow_pages_new; i++) {
            new_data.new_bufor();
            new_data.write_page(primary_pages_new + i);
        }

        overflow_pages = overflow_pages_new;
        primary_pages = primary_pages_new;
        index.write_page();

        remove("data.txt");
        rename("data2.txt", "data.txt");
        all_index.clear();
    }


    bool addToOverflow(Record record, int pointer, int p_to_change, DataFile overflow, int page) {
        if (pointer == -1) {
            int ov_page_number = 0;
            bool same_page = false;
            while (ov_page_number < overflow_pages) {
                if (page < primary_pages + ov_page_number)
                    overflow.read_page(primary_pages + ov_page_number);
                else {
                    copy_bufor(overflow.bufor, bufor);
                    ov_page_number = page - primary_pages;
                    same_page = true;
                }
                for (int i = 0; i < BUFOR_SIZE; i++) {
                    if (overflow.bufor[i].key == record.key)
                        return 0;
                    if (overflow.bufor[i].key == -1) {
                        if (same_page)
                            bufor[i] = record;
                        bufor[p_to_change].pointer = i + ov_page_number * BUFOR_SIZE;
                        write_page(page);
                        if (!same_page) {
                            overflow.bufor[i] = record;
                            overflow.write_page(primary_pages + ov_page_number);
                        }
                        if (i == BUFOR_SIZE - 1) {
                            reorganization();
                            createVector();
                        }
                        return 1;
                    }
                }
                ov_page_number++;
            }
        }
        else {
            int overflow_page = pointer / BUFOR_SIZE + primary_pages;
            int prev_p_to_change = p_to_change;
            p_to_change = pointer % BUFOR_SIZE;
            if (overflow_page != page)
                overflow.read_page(overflow_page);
            if (overflow.bufor[p_to_change].key == record.key)
                return 0;
            if (overflow.bufor[p_to_change].key > record.key) {
                record.pointer = pointer;
                return addToOverflow(record, -1, prev_p_to_change, overflow, page);
            }
            copy_bufor(bufor, overflow.bufor);
            return addToOverflow(record, bufor[p_to_change].pointer, p_to_change, overflow, overflow_page);
        }
    }

    bool add(Record record, int page) {
        for (int i = 0; i < BUFOR_SIZE; i++) {
            if (bufor[i].key < record.key && (bufor[i].key != -1 || bufor[i].first == true)) {
                if (i < BUFOR_SIZE - 1) {
                    if (bufor[i + 1].key == record.key)
                        return 0;
                    if (bufor[i + 1].key > record.key) {
                        bool is_empty = false;
                        for (int j = i + 2; j < BUFOR_SIZE; j++) {
                            if (bufor[j].key == -1) {
                                is_empty = true;
                                break;
                            }
                        }
                        if (is_empty == true) {
                            moveAndAdd(record, i + 1);
                            write_page(page);
                            break;
                        }
                        else {
                            DataFile overflow;
                            return addToOverflow(record, bufor[i].pointer, i, overflow, page);
                        }
                    }
                }
                else {
                    DataFile overflow;
                    return addToOverflow(record, bufor[i].pointer, i, overflow, page);
                }

            }
            else if (bufor[i].key == record.key)
                return 0;
            else if (bufor[i].key == -1 && bufor[i].first == false) {
                bufor[i] = record;
                write_page(page);
                break;
            }
        }

        return 1;
    }

    void read_page(int page) {
        readings++;
        ifstream file(file_name, std::ios::binary | std::ios::out | std::ios::in);
        file.seekg(page * sizeof(Record) * BUFOR_SIZE);
        file.read((char*)bufor, sizeof(Record) * BUFOR_SIZE);
        file.close();
        pages_read++;
    }

    void write_page(int page) {
        writtings++;
        ofstream file(file_name, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(sizeof(Record) * (BUFOR_SIZE)*page);
        file.write((char*)bufor, sizeof(Record) * BUFOR_SIZE);
        file.close();
    }
};


void showRecord(Record record) {
    cout << "Klucz: " << record.key << " zbior: ";
    for (int j = 0; j < MAX_SET_SIZE; j++) {
        cout << record.numbers[j] << " ";
    }
    cout << "wskaznik: " << record.pointer << " czy pierwszy: " << record.first << endl;
}

int search_page(int key) {
    if (all_index.empty())
        createVector();
    for (int i = 0; i < all_index.size(); i++) {
        if (all_index[i].key < key) {
            if (i != all_index.size() - 1) {
                if (all_index[i + 1].key > key) {
                    return i;
                }
            }
            else
                return all_index[i].page_number;
        }
        if (all_index[i].key == key)
            return -1;
    }
}

Record findRecord(int key) {
    int page = search_page(key);
    if (page == -1) {
        for (int i = 0; i < all_index.size(); i++) {
            if (all_index[i].key == key)
                page = all_index[i].page_number;
        }
    }
    DataFile data_file;
    data_file.read_page(page);
    for (int i = 0; i < BUFOR_SIZE; i++) {
        if (data_file.bufor[i].key == key)
            return data_file.bufor[i];
        if (i < BUFOR_SIZE - 1) {
            if (data_file.bufor[i + 1].key > key) {
                if (data_file.bufor[i].pointer != -1) {
                    DataFile overflow;
                    Record record;
                    int pointer = data_file.bufor[i].pointer;
                    int overflow_page = 0;
                    while (true) {
                        record = data_file.getRecord(overflow, pointer, overflow_page);
                        pointer = record.pointer;
                        if (record.key == -2 || record.key == key)
                            return record;
                        else if (record.pointer == -1) {
                            record.key = -2;
                            return record;
                        }

                    }
                }
                else {
                    data_file.bufor[i].key = -2;
                    return data_file.bufor[i];
                }
            }
        }
    }
    Record record;
    record.key = -2;
    return record;
}

void showByKey() {
    DataFile data;
    DataFile overflow;
    Record record;
    int pointer = -1;
    data.read_page(0);
    int overflow_page = 0;

    while (true) {
        record = data.getRecord(overflow, pointer, overflow_page);
        pointer = record.pointer;
        if (record.key == -2)
            break;
        else if (record.first != true) {
            showRecord(record);
        }
    }
}


bool addRecord(Record record) {
    int page = search_page(record.key);
    if (page == -1)
        return 0;
    DataFile data_file;
    data_file.read_page(page);
    bool is_ok = data_file.add(record, page);
    if (!is_ok)
        return 0;
    return 1;

}



//S�uzylo do przeprowadzenia eksperymentu
void create() {
    IndexFile index_file;
    index_file.create();
    DataFile data_file;
    data_file.create();
    data_file.show();
    createVector();
    Record record;
    srand(time(NULL));
    readings = 0;
    writtings = 0;
    double amount = 200;
    for (int i = 0; i < amount; i++) {

        //Record record;
        record.first = 0;
        record.pointer = -1;
        record.key = rand();
        for (int i = 0; i < MAX_SET_SIZE; i++) {
            record.numbers[i] = (rand() / (double)RAND_MAX) * 9 + 1;
        }
        // cout << "key: " << record.key << endl;
        addRecord(record);
        // showByKey();
        Record record2 = findRecord(2587);
        if (record2.key == -2)
            cout << "Rekord o podanym kluczu nie istnieje" << endl;
        else
            showRecord(record2);
        //data_file.reorganization();
        data_file.show();
    }
    cout << "writting " << writtings / amount << endl;
    cout << "readings" << readings / amount;
}

void readFromConsole(bool show_file) {
    IndexFile index_file;
    index_file.create();
    DataFile data_file;
    data_file.create();
    createVector();
    while (true) {
        bool ok = false;
        string option;
        while (!ok) {
            std::cout << "Wybierz operacje" << std::endl << "d - dodaj nowy rekord" << std::endl;
            std::cout << "o - oczytaj rekord o zadanym kluczu" << std::endl << "r - przeprowadz reorganizacje" << endl << "w - wyswietl plik zgodnie z zawartoscia klucza" << endl << "z - zakoncz" << endl;
            std::cin >> option;
            if (option != "d" && option != "o" && option != "r" && option != "w" && option != "z") {
                std::cout << "Wybrana operacja nie istnieje" << std::endl;
            }
            else
                ok = true;
        }
        if (option == "d") {
            Record record;
            std::cout << "Podaj klucz" << std::endl;
            int key;
            cin >> record.key;
            std::cout << "Czy chcesz wygenerowac losowy zbior? [tak/nie]" << std::endl;
            ok = false;
            std::string gen;
            while (!ok) {
                std::cin >> gen;
                if (gen != "tak" && gen != "nie")
                    std::cout << "Niepoprawna odpowiedz" << std::endl;
                else
                    ok = true;
            }
            if (gen == "tak") {
                for (int i = 0; i < MAX_SET_SIZE; i++) {
                    record.numbers[i] = (rand() / (double)RAND_MAX) * 9 + 1;
                }
            }
            else {
                cout << "Podaj zbior" << endl;
                for (int i = 0; i < MAX_SET_SIZE; i++) {
                    cin >> record.numbers[i];
                }
            }
            record.pointer = -1;
            record.first = false;
            bool not_rep = addRecord(record);
            if (not_rep) {
                if (show_file)
                    data_file.show();
            }
            else
                cout << "Podany klucz ju� istnieje" << endl;
        }
        else if (option == "o") {
            cout << "Podaj klucz kt�rego rekord chcesz odczyta�" << endl;
            Record record;
            int key;
            cin >> record.key;
            record = findRecord(record.key);
            if (record.key == -2)
                cout << "Rekord o podanym kluczu nie istnieje" << endl;
            else
                showRecord(record);
        }
        else if (option == "r") {
            data_file.reorganization();
            if (show_file)
                data_file.show();
        }
        else if (option == "w") {
            showByKey();
        }
        else if (option == "k")
            break;
    }
}


void readFromFile(bool show_file) {
    IndexFile index_file;
    index_file.create();
    DataFile data_file;
    data_file.create();
    createVector();
    cout << "Podaj nazwe pliku" << endl;
    string name;
    cin >> name;
    fstream file(name);
    while (true) {
        char option;
        file >> option;
        cout << option;
        if (option == 'd') {
            Record record;
            file >> record.key;
            cout << " " << record.key;
            for (int i = 0; i < MAX_SET_SIZE; i++) {
                file >> record.numbers[i];
                cout << record.numbers[i] << " ";
            }
            cout << endl;
            record.pointer = -1;
            record.first = false;
            bool not_rep = addRecord(record);
            if (not_rep) {
                if (show_file)
                    data_file.show();
            }
            else
                cout << "Podany klucz ju� istnieje" << endl;
        }
        else if (option == 'o') {
            Record record;
            file >> record.key;
            cout << " " << record.key << endl;
            record = findRecord(record.key);
            if (record.key == -2)
                cout << "Rekord o podanym kluczu nie istnieje" << endl;
            else
                showRecord(record);
        }
        else if (option == 'r') {
            data_file.reorganization();
            if (show_file)
                data_file.show();
        }
        else if (option == 'w') {
            showByKey();
        }
        else if (option == 'k')
            break;
        cout << endl;
    }
}


void menu() {
    bool ok = false;
    bool show_file;
    std::string number;
    while (!ok) {
        std::cout << "Wybierz tryb" << std::endl << "1 - wczytaj operacje z konsoli" << std::endl;
        std::cout << "2 - wczytaj operacje z pliku" << std::endl;
        std::cin >> number;
        if (number != "1" && number != "2") {
            std::cout << "Wybrany tryb nie istnieje" << std::endl;
        }
        else
            ok = true;
    }
    std::cout << "Czy chcesz wyswietlac zawartosc pliku po kazdej operacji? [tak/nie]" << std::endl;
    ok = false;
    while (!ok) {
        std::string show;
        std::cin >> show;
        if (show != "tak" && show != "nie")
            std::cout << "Niepoprawna odpowiedz" << std::endl;
        else {
            if (show == "tak")
                show_file = true;
            else
                show_file = false;
            ok = true;
        }
    }
    if (number == "1")
        readFromConsole(show_file);
    else if (number == "2")
        readFromFile(show_file);

}

int main()
{
    srand(time(NULL));
    //menu();
    create();
}