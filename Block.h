#pragma once

#include<fstream>
#include<string>
#include<vector>
#include <sstream>
#include <queue>
#include "const.h"

using namespace std;

class BTreeNode;

class Block
{
	fstream file;
	string filename;

	char *buffer;

	int read_block;
	int written_block;
	int read_records;
	int buffer_size;
	int current_node;

	bool end_of_file;
	bool txt;

	void load_block();
	void load_block(int page);
	void write_block();
	void write_block(int page);

public:
	Block();
	Block(string filename, int size);
	Block(string filename, int size, int pointer_in_file);
	~Block();

	vector<int> get_next_record();
	vector<int> get_next_record(int page);
	void write_record(vector<int>& record);
	void rewrite_record(int page, int key);
	void close_file();
	void clear_file();
	int index_record;
	int read_write();
	int get_read_blocks();
	void write_node(BTreeNode node, int page);
	void write_new_node(BTreeNode node);
	void write_node(BTreeNode node, vector<int> nodes_order);
	BTreeNode read_node(int page);
	int getCurrentNode();
	void removeFile();
};