#include<algorithm>
#include<random>

#include"BTree.h"
#include"Block.h"

using namespace std;

void adding_record(const string filename, vector<int> numbers, BTree* tree) {

	ofstream file(filename, ios::binary | ios::app);

	file.write(reinterpret_cast<const char*>(numbers.data()), numbers.size() * SIZE_INT);

	file.close();

	std::ifstream file1(filename, std::ios::binary | std::ios::ate);

	int size_of_block = SIZE_BUFFER_RECORDS;
	streamsize size_of_file = file1.tellg();
	
	cout << "REcord with key: " << tree->insert(numbers[SIZE_RECORD - 1] , (size_of_file / size_of_block) + 1) << " was added to the file." << endl;
	cout << "Disk oparations: " << tree->insert(numbers[SIZE_RECORD - 1], (size_of_file / size_of_block) + 1) << endl;

	file1.close();
}

void generating_records(const string filename, int records_numbers) {

	// Funkcja do generowania losowych rekordów.
	// Losowe inty do rekordów, 
	// Co do kluczy to wpisanie zinkrementowanych n kluczy 
	// nastêpnie wymieszanie ich losow i nadanie rekordom.. W ten sposób ka¿dy rekord bêdzie mia³ inny klucz. 
	// A klucze bêdê siê pojawiaæ w dowolnej sekwencji

	ofstream file(filename, ios::binary | ios::trunc);

	srand(time(NULL));
	
	vector<int> key_generator;
	
	for (int i = 0; i < records_numbers; i ++) {
		key_generator.push_back(i + 1);
	}
	
	random_device rd;
	mt19937 gen(rd());
	shuffle(key_generator.begin(), key_generator.end(), gen);

	for (int i = 0; i < records_numbers; i++) {
		vector<int> numbers;
		for (int ii = 0; ii < SIZE_RECORD - 1; ii++) {
			numbers.push_back(rand() % 1000000);
		}
		numbers.push_back(key_generator.back());
		key_generator.pop_back();
		file.write(reinterpret_cast<const char*>(numbers.data()), numbers.size() * SIZE_INT);
	}

	file.close();
}

// Funckja do wyœwietlania pliku
void show_file(const string filename) {
	Block read_file(filename, SIZE_BUFFER_RECORDS);

	vector<int> record = read_file.get_next_record();
	cout << "\nRecords in " << filename << ":\n";
	int inc = 1;

	while (record.size()) {
		if (record[SIZE_RECORD - 1] != -1) {
			cout << inc << ". ";

			for (int ii = 0; ii < SIZE_RECORD - 1; ii++) {
				cout << record[ii] << " ";
			}

			cout << "Key: " << record[SIZE_RECORD - 1] << " Page: " << read_file.get_read_blocks() << endl;

			inc++;
		}

		record = read_file.get_next_record();
	}
}


// Funkcja do wyœwietlania pojedynczego rekordu ze strony page
void show_record(const string filename, int key, pair<int, int> page) {
	Block read_file(filename, SIZE_BUFFER_RECORDS);

	vector<int> record = read_file.get_next_record(page.first);
	cout << "\nRecord found on page " << page.first << ":\n";

	while (record.size()) {
		
		if (record[SIZE_RECORD - 1] == key && key != -1) {
			for (int ii = 0; ii < SIZE_RECORD - 1; ii++) {
				cout << record[ii] << " ";
			}

			cout << "Key: " << record[SIZE_RECORD - 1] << " Page: " << read_file.get_read_blocks() << endl;
			cout << "Disk operation: " << read_file.read_write() + page.second << endl;
		}
		record = read_file.get_next_record();
	}
}

void delete_record(const string filename, int key, pair<int, int> page) {
	
	Block read_file(filename, SIZE_BUFFER_RECORDS);
	
	// przekazanie do bloku klucza i strony na której 
	// znajduje siê rekord do usuniêcia
	read_file.rewrite_record(page.first, key);

	// wyœwietlenie komunikatu
	cout << "\nRecord deleted from page " << page.first << ":\n";
	cout << "Disk operation: " << read_file.read_write() + page.second << endl;
}
 
int  createTree(const string filename, BTree* tree) {
	int diskOperation = 0;

	Block read_file(filename, SIZE_BUFFER_RECORDS);

	vector<int> record = read_file.get_next_record();

	while (record.size()) {
		diskOperation += tree->insert(record[SIZE_RECORD - 1], read_file.get_read_blocks());
		record = read_file.get_next_record();
	}
	
	return diskOperation;
}

void start_program(const string filename, const string index_file) {

	BTree* tree = new BTree(T, index_file);

	while (true) {
		cout << "\nChoose: (write 1-4)" << endl;
		cout << "1. Print file" << endl;
		cout << "2. Generate data" << endl;
		cout << "3. Add record" << endl;
		cout << "4. Print tree" << endl;
		cout << "5. Printing sorted Tree" << endl;
		cout << "6. Find record" << endl;
		cout << "7. Remove element" << endl;
		cout << "8. Create a tree" << endl;
		cout << "9. Reorginize index file" << endl;
		cout << "10. Quit\n" << endl;

		int choice;
		cin >> choice;

		if (choice == 1) {
			
			// wyœwietlanie g³ównego pliku
			show_file(filename);
		}
		else if (choice == 2) {

			// generowanie nowych rekordów
			cout << "Enter number of records: ";
			cin >> choice;
			generating_records(filename, choice);
		}
		else if (choice == 3) {

			// dodanie nowego rekordu
			while (true) {
				vector<int> new_record;

				cout << "Enter values for new record (6 integers after spaces): ";
				for (int i = 0; i < SIZE_RECORD - 1; i++) {
					cin >> choice;
					new_record.push_back(choice);
				}

				cout << "Enter the key: ";
				cin >> choice;
				new_record.push_back(choice);

				pair<int, int> address = tree->search(choice); // zwróci -1 jak bdrzewo nie ma klucza k

				if (address.first == -1)
					adding_record(filename, new_record, tree);
				else
					cout << endl << "There is already record with that key" << endl;

				// wywo³anie funkcji do dopisania rekordu w pliku g³ównym i do indeksu
				cout << "Do you want to add more records? (Choose: 0(NO)/ YES)";
				cin >> choice;

				if (!choice) break;
			}
		}
		else if (choice == 4) {
			
			// funkcja do wyœwietlania drzewa
			if (tree->getRootPage()!= -1)
				tree->printTree(tree->getRootPage());
			else
				cout << "Tree is empty"<< endl;
		}
		else if (choice == 5) {

			// funkcja do wyœwietlania drogi (wszystkie podrzewa ³¹cznie z rodzicem)
			if (tree->getRootPage())
				tree->traverse();
			else
				cout << "Tree is empty" << endl;
		}
		else if (choice == 6) {
			// funkcja do wyszukiwania klucza w drzewie
			cout << "Enter the key: ";
			cin >> choice;

			// funkcja search zwraca adres w pliku g³ównym 
			pair<int, int> address = tree->search(choice); // zwróci -1 jak bdrzewo nie ma klucza k
			
			if (address.first == -1) 
				cout << endl << "There is no element with that key" << endl;			
			else 
				// wywo³anie funkcji do wyœwietlania rekordu z kluczem k
				show_record(filename, choice, address);
			
		}
		else if (choice == 7) {
			// funkcja do usuwania klucza k
			cout << "Enter the key: ";
			cin >> choice;

			// wyœwietlanie liczby operacji dyskowych
			pair<int, int> address = tree->remove(choice); // zwróci -1 jak bdrzewo nie ma klucza k

			if (address.first == -1)
				cout << endl << "There is no element with that key" << endl;
			else
				// wywo³anie funkcji do wyœwietlania rekordu z kluczem k
				delete_record(filename, choice, address);
		}
		else if (choice == 8) {
			// usuwanie starego drzewwa i nicjalizacja nowego
			delete tree;
			tree = new BTree(T, index_file);
			
			// wywo³anie funkcji tworz¹cej drzewo z pliku g³ównego
			int operations = createTree(filename, tree);

			// komunikat i wyœwietlenie operacji dyskowych
			cout << endl<< "Tree has been created" << endl;
			cout << "Disk operations: " << operations << endl << endl;
		}
		else if (choice == 9) {
			
			// wywo³anie funkcji reorganizacji pliku indeksowego
			tree->reorginize();
		}
		else if (choice == 10) {
			break;
		}
		else {
			cout << "Wrong option. Try again.\n\n";
		}
	}

	delete tree;
}


int main() {

	string filename = "data.bin", index_file = "index.bin";
	start_program(filename, index_file);

	return 0;
}