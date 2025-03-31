#include "Block.h"
#include "BTreeNode.h"

Block::Block(string filename, int size) : filename(filename), read_block(0), written_block(0), read_records(0), index_record(0), end_of_file(false) {
	buffer_size = size;
	buffer = new char[size];
	int dot_position = filename.find_last_of(".");
	filename.substr(dot_position + 1) == "txt" ? txt = true : txt = false;

	!txt ? file.open(filename, ios::in | ios::out | ios::binary) : file.open(filename, ios::in | ios::out);
}

Block::Block(string filename, int size, int pointer_in_file) : filename(filename), read_block(0),
written_block(0), read_records(0), index_record(0), end_of_file(false), txt(false), current_node(0) {

	// ustawianie odpowiedniego size'a na bufor
	buffer_size = size;
	buffer = new char[size];

	// otworzenie pliku z flag¹ trunc
	file.open(filename, ios::in | ios::out | ios::binary | ios::trunc);

	// zainicjalizowanie bajta w pliku do porpawnego dzia³ania wskaŸników
	file.write("", 0);
}

Block::~Block() {

	if (index_record) {
		write_block();
	}

	if (file.is_open()) {
		file.close();
	}
	delete[] buffer;
}

void Block::load_block() {

	if (!txt) {
		file.read(buffer, SIZE_BUFFER_RECORDS);

		read_records = file.gcount() / (SIZE_INT * SIZE_RECORD);
		read_records < SIZE_BLOCK ? end_of_file = true : end_of_file = false;

	}
	else {
		vector<int> records_buffer;

		string block;

		getline(file, block);

		int integer;
		istringstream stream(block);

		while (stream >> integer) {
			records_buffer.push_back(integer);
		}

		if (records_buffer.size() < SIZE_RECORD * SIZE_BLOCK) {
			end_of_file = true;
		}

		read_records = records_buffer.size() / SIZE_RECORD;

		for (int i = 0; i < records_buffer.size(); ++i) {

			std::memcpy(buffer + i * SIZE_INT, &records_buffer[i], SIZE_INT);
		}

	}

	index_record = 0;
	read_block++;
}

void Block::load_block(int page) {
	// ustawienie wskaŸnika w pliku
	file.seekg(SIZE_BUFFER_RECORDS * (page - 1), std::ios::beg);
	file.clear();

	// odczytanie bloku
	file.read(buffer, SIZE_BUFFER_RECORDS);

	// aktualizacji liczby przeczytanych rekordów
	read_records = file.gcount() / (SIZE_INT * SIZE_RECORD);
	index_record = 0;
	read_block++;
}

void Block::write_block(int page) {
	//; ustawienie wskaŸnika w pliku
	file.clear();
	file.seekp(SIZE_BUFFER_RECORDS * (page - 1), std::ios::beg);
	index_record = 0;
	// wpisanie bufora ze zmienionym rekordem
	file.write(buffer, read_records * SIZE_RECORD * SIZE_INT);


	// inkrementacja operacji dyskowych
	written_block++;
}

void Block::write_block() {
	if (!txt) {
		if (index_record) {
			file.write(buffer, index_record * SIZE_RECORD * SIZE_INT);
		}
	}
	else {
		vector<int> records_buffer;

		for (int i = 0; i < index_record * SIZE_RECORD; i++) {
			int integer;
			std::memcpy(&integer, &buffer[i * SIZE_INT], SIZE_INT);

			records_buffer.push_back(integer);
		}

		stringstream stream;

		for (int i = 0; i < records_buffer.size(); ++i) {
			stream << records_buffer[i];
			if (i != records_buffer.size() - 1) {
				stream << " ";
			}
		}

		file << stream.str() << endl;
	}

	index_record = 0;
	written_block++;
}

vector<int> Block::get_next_record() {

	if (index_record == read_records && !end_of_file) {
		load_block();
	}

	if (index_record < read_records || !end_of_file) {

		vector<int> record(SIZE_RECORD);

		for (int i = 0; i < SIZE_RECORD; i++) {
			memcpy(&record[i], buffer + (index_record * SIZE_RECORD + i) * SIZE_INT, SIZE_INT);
		}

		index_record++;

		return record;
	}
	else {

		return {};
	}
}

// Funkcja do pobierania rekordów z bloku page 
// znajduj¹cegop siê w pliku g³ównym
vector<int> Block::get_next_record(int page) {

	if (index_record == read_records && !end_of_file) {
		load_block(page);
	}

	if (index_record < read_records || !end_of_file) {

		vector<int> record(SIZE_RECORD);

		for (int i = 0; i < SIZE_RECORD; i++) {
			memcpy(&record[i], buffer + (index_record * SIZE_RECORD + i) * SIZE_INT, SIZE_INT);
		}

		index_record++;

		return record;
	}
	else {

		return {};
	}
}

void Block::rewrite_record(int page, int key) {
	
	// pobieranie rekkordów z danej strony
	vector<int> record = get_next_record(page);

	// wyszukanie rekordu
	while (record.size()) {

		// zmiana klcuza na -1
		if (record[SIZE_RECORD - 1] == key) {
			record[SIZE_RECORD - 1] = -1;
			break;
		}

		// pobieranie nastêpnego rekordu
		record = get_next_record();
	}
	
	// wpisanie zaktualizowanego rekordu do bufora
	memcpy(buffer + index_record * SIZE_RECORD * SIZE_INT, record.data(), record.size() * sizeof(int));

	// wpisanie bloku
	write_block(page);
}


void Block::write_record(vector<int>& record) {
	memcpy(buffer + index_record * SIZE_RECORD * SIZE_INT, record.data(), record.size() * sizeof(int));

	index_record++;
	if (index_record == SIZE_BLOCK) {
		write_block();
	}
}

void Block::close_file() {
	if (index_record) {
		write_block();
	}

	file.close();
	//!txt ? file.open(filename, ios::in | ios::out | ios::binary) : file.open(filename, ios::in | ios::out);
}

void Block::clear_file() {
	file.close();
	ofstream file(filename, ios::binary | ios::trunc);
	file.close();
	end_of_file = false;
	index_record = 0;
	read_records = 0;
	file.open(filename, ios::in | ios::out | ios::binary);
}

int Block::read_write() {
	return (read_block + written_block);
}

int Block::get_read_blocks() {
	return read_block;
}


// Funkcja do nadpisywania nodów bêd¹cych ju¿ w pliku
void Block::write_node(BTreeNode node, int page) {

	// zamiana struktury node'a na wektor który przechowuje 
	// inty z node'a w kolejnoœci:
	// child[0],key[0],address[0],child[1],key[1],address[1],..., child[n+1]
	vector<int> node_to_file((2 * T - 1) * SIZE_DATA + 2 * T, -1);

	int index_x = 0, index_a = 0, index_c = 0;
	for (int i = 0; i < (3 * node.n + 1); i++) {
		if (i % 3 == 0 && !node.leaf) {
			node_to_file[i] = node.children[index_c];
			index_c++;
		}
		else if (i % 3 == 1) {
			node_to_file[i] = node.keys[index_x];
			index_x++;
		}
		else if (i % 3 == 2) {
			node_to_file[i] = node.addresses[index_a];
			index_a++;
		}
	}

	// wpisanie wektora do bufora
	memcpy(buffer, node_to_file.data(), node_to_file.size() * sizeof(int));

	// ustawienie odpowiedniej strony w pliku
	file.seekp(SIZE_BUFFER_NODE * (page - 1), std::ios::beg);

	// wpisanie bufora do pliku
	file.write(buffer, buffer_size);

	// inkrementacja wpisanych nodów
	written_block++;
}

// Funkcja do wpisywania nowych nodów
void Block::write_new_node(BTreeNode node) {

	// transfoirmacja node'a jak w funkcji powy¿ej
	vector<int> node_to_file((2 * T - 1) * SIZE_DATA + 2 * T, -1);

	int index_x = 0, index_a = 0, index_c = 0;
	for (int i = 0; i < (3 * node.n + 1); i++) {
		if (i % 3 == 0 && !node.leaf) {
			node_to_file[i] = node.children[index_c];
			index_c++;
		}
		else if (i % 3 == 1) {
			node_to_file[i] = node.keys[index_x];
			index_x++;
		}
		else if (i % 3 == 2) {
			node_to_file[i] = node.addresses[index_a];
			index_a++;
		}
	}

	// kopiowanie danym do bufora
	memcpy(buffer, node_to_file.data(), node_to_file.size() * sizeof(int));

	// ustawienie wskaŸnika na koniec pliku
	file.seekp(0, std::ios::end);

	// wpisanie nowego bufora
	file.write(buffer, buffer_size);

	// inkrementacja nowej pozycji dostêpnej dla kolejnego node'a
	current_node++;

	// inkrementacja wpisanych nodów
	written_block++;

	///*--------ROBOCZE--------*/
	///*Funkcja do wyœwietlania wielkoœci pliku w bajtach. 
	//Po ka¿dym nowym plik powinien mieæ o buffer_size wiêcej bajtów*/
	//streampos file_size = file.tellg();
	//cout << "File size after write: " << file_size << " bytes" << endl;
}


// Funkcja do wpisywania nodów do pliku. Ustala po³o¿enie dzieci w 
// pliku i zapisuje je w nodzie przed write do pliku 
void Block::write_node(BTreeNode node, vector<int> nodes_order) {
	vector<int> node_to_file((2 * T - 1) * SIZE_DATA + 2 * T, -1);
	vector<int> children_pages(node.n + 1, - 1);

	// getting addresses of children
	if (!node.leaf) {
		for (int i = 0; i < node.n + 1; i++) {
			for (int j = 0; j < nodes_order.size(); j++) {
				if (node.children[i] == nodes_order[j]) {
					children_pages[i] = j + 1;
					break;
				}
			}
		}
	}

	int index_x = 0, index_a = 0, index_c = 0;
	for (int i = 0; i < (3 * node.n + 1); i++) {
		if (i % 3 == 0) {
			node_to_file[i] = children_pages[index_c];
			index_c++;
		}
		else if (i % 3 == 1) {
			node_to_file[i] = node.keys[index_x];
			index_x++;
		}
		else {
			node_to_file[i] = node.addresses[index_a];
			index_a++;
		}
	}

	// kopiowanie danym do bufora
	memcpy(buffer, node_to_file.data(), node_to_file.size() * sizeof(int));

	// ustawienie wskaŸnika na koniec pliku
	file.seekp(0, std::ios::end);

	// wpisanie nowego bufora
	file.write(buffer, buffer_size);

	//	/*--------ROBOCZE--------*/
	///*Funkcja do wyœwietlania wielkoœci pliku w bajtach.
	//Po ka¿dym nowym plik powinien mieæ o buffer_size wiêcej bajtów*/
	//	streampos file_size = file.tellg();
	//	cout << "File size after write: " << file_size << " bytes" << endl;

	// inkrementacja nowej pozycji dostêpnej dla kolejnego node'a
	current_node++;

	// inkrementacja wpisanych nodów
	written_block++;
}

// Funkcja do wczytywania pojedynczego node'a 
// z odpowiedniej strony w pliku
BTreeNode Block::read_node(int page) {

	// ustawienie wskaŸnika na odpowiendiej stronie
	file.seekg(SIZE_BUFFER_NODE * (page - 1), std::ios::beg);

	// wczytanie strony
	file.read(buffer, buffer_size);

	// inkrementacja odczytanych nodów
	read_block++;

	// transformacja charów na inty
	int* int_buffer = reinterpret_cast<int*>(buffer);

	// zwrócenie BTreeNode'a, który w konstruktorze zawiera wektor z intami node'a
	return BTreeNode(T, std::vector<int>(int_buffer, int_buffer + (2 * T - 1) * SIZE_DATA + 2 * T), page);
}

// Funkcja do zwracania obecnie wpisanej 
// iloœci nodów do piku
int Block::getCurrentNode() {
	return current_node;
}

void Block::removeFile() {
	file.flush();
	file.close();
	if (std::remove(filename.c_str()) != 0) {
		perror("Error deleting file");
	}
}