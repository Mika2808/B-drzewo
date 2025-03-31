#pragma once

#include"BTreeNode.h"

class Block;

class BTree
{
    BTreeNode* root; // Wska¿nik na roota
    Block* file; // System blokowy
    string filename; // Nazwa pliku indeksowego
    int rootPage; // Strona roota w pliku
    int t;  // Minimalna iloœæ dzieci
    int height; // Wysokoœæ drzewa
    int diskOperation; // Operacje dyskowe

    // funkcja do sprawdzania czy reorganizacji indeksu jest potrzebna
    void ifReorginize();

public:

    // konstruktory
    BTree(int d1, const string filename);
    BTree(int t1, const string filename1, int _rootPage);

    void traverse();

    // funkcja do zwracania numeru strony w pliku g³ównym klucza k
    pair<int, int> search(int k);

    // g³ówna funkcja inserta do drzewa
    int insert(int k, int a);

    // g³ówna funkcja do usuwania
    pair<int, int> remove(int k);

    // funkcja do rysowania drzewa
    int printTree(int node_page, int depth = 0);

    // funkcja do zwracania page'a z rootem
    int getRootPage();

    // funkcja do zwracania operacji dyskowych
    int getDiskOperation();

    // funkcja do reaorganizacji pliku indeksowego
    void reorginize();
};
