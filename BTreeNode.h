#pragma once
#include<iostream>
#include<utility>
#include<vector>

using namespace std;

class Block;


class BTreeNode
{
    int* keys;  // Lista z kluczami rekord�w
    int* addresses;  // Lista z adresami rekord�w 
    int* children; // Lista z dzie�mi 
    int t;      // Minimalna ilo�� dzieci
    int n;     // Obecna ilo�� kluczy
    bool leaf; // Info o tym czy node jest li�ciem
    int pointerInFile; // Miejsce w pliku indeksowym

public:

    BTreeNode();
    BTreeNode(int _t, bool _leaf); // do tworzenia nowych nod�w
    BTreeNode(int _t, vector<int> node, int pointer);  // do odczytywania z pliku

    // funkcja przechodz�ca przez poddrzewo wraz z tym nodem
    void traverse(Block* file);

    // funkcja do szukania klucza w drzewie
    int search(int k, Block* file);   // zwraca adres (pliku g��wnego) z nodem, kt�ry posiada klucz k

    // funkcja do zwracania pierwszego wi�kszego lub r�wnego klucza 
    int findKey(int k);

    // dodawanie klucza do niepe�nego node'a
    void insertNonFull(int k, int a, Block* file);

    // split i-tego dziecka (y), dziecko musi by� pe�ne
    void splitChild(int i, BTreeNode* y, Block* file);

    // rekurencyjna, g��wna funkcja do usuwania klucza
    int remove(int k, Block* file);

    // funkcja do usuwania z li�cia klucza na idx pozycji
    void removeFromLeaf(int idx, Block* file);

    // funkcja do usuwania klucza na idx pozycji z nie li�cia
    void removeFromNonLeaf(int idx, Block* file);

    // pobieranie poprzednika klucza znajduj�cego si� pod idx
    pair<int, int> getPred(int idx, Block* file);

    // pobieranie nast�pce klucza znajduj�cego si� pod idx
    pair<int, int> getSucc(int idx, Block* file);

    // dope�nianie dziecka pod idx, kt�re ma mniej ni� t-1 kluczy
    void fill(int idx, BTreeNode* child, Block* file);

    // po�yczanie klucza (przez idx dziecko) od lewego (idx-1) rodze�stwa
    void borrowFromPrev(int idx, BTreeNode* child, BTreeNode* sibling, Block* file);

    // po�yczanie klucza (przez idx dziecko) od prawego (idx+1) rodze�stwa
    void borrowFromNext(int idx, BTreeNode* child, BTreeNode* sibling, Block* file);

    // funkcja do merge'a (idx) dziecka z (idx+1) dzieckiem
    void merge(int idx, BTreeNode* child, BTreeNode* sibling, Block* file);

    friend class BTree;
    friend class Block;
};
