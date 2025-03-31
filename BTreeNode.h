#pragma once
#include<iostream>
#include<utility>
#include<vector>

using namespace std;

class Block;


class BTreeNode
{
    int* keys;  // Lista z kluczami rekordów
    int* addresses;  // Lista z adresami rekordów 
    int* children; // Lista z dzieæmi 
    int t;      // Minimalna iloœæ dzieci
    int n;     // Obecna iloœæ kluczy
    bool leaf; // Info o tym czy node jest liœciem
    int pointerInFile; // Miejsce w pliku indeksowym

public:

    BTreeNode();
    BTreeNode(int _t, bool _leaf); // do tworzenia nowych nodów
    BTreeNode(int _t, vector<int> node, int pointer);  // do odczytywania z pliku

    // funkcja przechodz¹ca przez poddrzewo wraz z tym nodem
    void traverse(Block* file);

    // funkcja do szukania klucza w drzewie
    int search(int k, Block* file);   // zwraca adres (pliku g³ównego) z nodem, który posiada klucz k

    // funkcja do zwracania pierwszego wiêkszego lub równego klucza 
    int findKey(int k);

    // dodawanie klucza do niepe³nego node'a
    void insertNonFull(int k, int a, Block* file);

    // split i-tego dziecka (y), dziecko musi byæ pe³ne
    void splitChild(int i, BTreeNode* y, Block* file);

    // rekurencyjna, g³ówna funkcja do usuwania klucza
    int remove(int k, Block* file);

    // funkcja do usuwania z liœcia klucza na idx pozycji
    void removeFromLeaf(int idx, Block* file);

    // funkcja do usuwania klucza na idx pozycji z nie liœcia
    void removeFromNonLeaf(int idx, Block* file);

    // pobieranie poprzednika klucza znajduj¹cego siê pod idx
    pair<int, int> getPred(int idx, Block* file);

    // pobieranie nastêpce klucza znajduj¹cego siê pod idx
    pair<int, int> getSucc(int idx, Block* file);

    // dope³nianie dziecka pod idx, które ma mniej ni¿ t-1 kluczy
    void fill(int idx, BTreeNode* child, Block* file);

    // po¿yczanie klucza (przez idx dziecko) od lewego (idx-1) rodzeñstwa
    void borrowFromPrev(int idx, BTreeNode* child, BTreeNode* sibling, Block* file);

    // po¿yczanie klucza (przez idx dziecko) od prawego (idx+1) rodzeñstwa
    void borrowFromNext(int idx, BTreeNode* child, BTreeNode* sibling, Block* file);

    // funkcja do merge'a (idx) dziecka z (idx+1) dzieckiem
    void merge(int idx, BTreeNode* child, BTreeNode* sibling, Block* file);

    friend class BTree;
    friend class Block;
};
