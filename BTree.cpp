#include "BTree.h"
#include "Block.h"
#include <cstdio>
#include <cmath>

BTree::BTree(int t1, const string filename1) : root(NULL), rootPage(-1), t(t1), height(0), diskOperation(0), filename(filename1) {
    file = new Block(filename1, SIZE_BUFFER_NODE, 0);
}

BTree::BTree(int t1, const string filename1, int _rootPage) : root(NULL), rootPage(_rootPage), t(t1), height(0), diskOperation(0), filename(filename1) {
    file = new Block(filename1, SIZE_BUFFER_NODE, 0);
    root = new BTreeNode(file->read_node(rootPage));
}

int BTree::insert(int k, int a) {

    // nie ma roota
    if (root == NULL) {

        // inicjalizacja nowego roota
        root = new BTreeNode(t, true);
        root->keys[0] = k;
        root->addresses[0] = a;
        root->n = 1;
        rootPage = 1;
        root->pointerInFile = 1;

        // wpisanie roota do pliku
        file->write_new_node(*root);
    }
    else {

        // root jest pe�ny
        if (root->n == 2 * t - 1) {

            // tworzenie nowego roota
            BTreeNode* newRoot = new BTreeNode(t, false);

            // dodanie nowego roota do pliku indeksowego
            file->write_new_node(*newRoot);
            newRoot->pointerInFile = file->getCurrentNode();

            // dodanie starego roota jako dziecka i aktualizacja roota
            newRoot->children[0] = rootPage;
            rootPage = newRoot->pointerInFile;

            // split starego roota i dodanie klucza do nowego
            newRoot->splitChild(0, root, file);

            // dodanie nowego klucza do odpowiedniego dziecka
            int i = 0;
            if (newRoot->keys[0] < k)
                i++;

            // pobranie dziecka i wpisanie do niego nowego klucza
            BTreeNode child = file->read_node(newRoot->children[i]);
            child.insertNonFull(k, a, file);

            // zmiana roota
            delete root;
            root = newRoot;

            // wysoko��++
            height++;
        }
        else  // root ma wolne miejsce
            root->insertNonFull(k, a, file);
    }

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;
    diskOperation = file->read_write();

    // sprawdzenie czy reorganizacja w pliku jest potrzebna
    ifReorginize();

    // zwr�cenie wykonanych operacji dyskowych
    return (diskOperation - tmp);
}

pair<int, int> BTree::remove(int k) {
    if (rootPage == -1) {
        cout << "The tree is empty\n";
        return {- 1, 0};
    }

    int address = root->remove(k, file);

    // ustawianie nowego roota, na wypadek gdyby ten by� pusty
    if (root->n == 0)
    {
        // przechowanie starego roota
        BTreeNode* tmp = root;

        // je�li root jest li�ciem to drzewo jest puste
        if (root->leaf) {
            rootPage = -1;
            root = NULL;
        }
        else {
            // aktualizacja nowego roota i jego pozycji w pliku
            rootPage = root->children[0];
            root = new BTreeNode(file->read_node(root->children[0]));
        }

        // delete starego roota
        delete tmp;

        // aktualizacja wysoko�ci drzewa
        height--;
    }

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;
    int tmp2 = diskOperation = file->read_write();

    // sprawdzenie czy reorganizacja w pliku jest potrzebna
    ifReorginize();

    // zwr�cenie wykonanych operacji dyskowych
    return { address, (tmp2 - tmp) };
}


int BTree::printTree(int node_page, int depth) {

    // pobranie node'a z pliku
    BTreeNode node = file->read_node(node_page);

    // dodanie spacji dla g��boko�ci drzewa
    for (int i = 0; i < depth; ++i) {
        cout << "  ";
    }

    cout << "Node: ";

    // wy�wietlanie kluczy w danym nodzie
    for (int i = 0; i < node.n; i++) {
        cout << node.keys[i] << " ";
    }
    cout << endl;

    // wy�wietlanie dzieci je�li to nie li��
    if (!node.leaf) {
        for (int i = 0; i < node.n + 1; i++) {
            printTree(node.children[i], depth + 1);
        }
    }

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;
    diskOperation = file->read_write();

    // zwr�cenie wykonanych operacji dyskowych
    return (diskOperation - tmp);
}

int BTree::getRootPage() {
    return rootPage;
}

pair<int, int> BTree::search(int k) {

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;

    // je�li roota nie ma to ni ma 
    if (rootPage == NULL)
        return { -1, 0 };

    // rekurencyjne szukanie adresu
    int page = root->search(k, file);
    diskOperation = file->read_write();

    if (page == -2)
        // jesli nie ma klucza k w drzewie
        return { -1 , diskOperation - tmp };
    else if (page != -1)
        // je�li klucz by� w rootcie to zwracamy adres spod klucza
        return { page , diskOperation - tmp };
    else {
        // przeszukanie dzieci
        BTreeNode child = file->read_node(page);
        for (int i = 0; i < child.n; i++) {
            if (child.keys[i] == k) {
                diskOperation = file->read_write();
                return { child.addresses[i], diskOperation - tmp };
            }
        }
    }

}

void BTree::reorginize() {

    // Funkcja ma na celu przejecha� obecne drzewo za pomoc� bfs'a i
    // zapisa� przebyt� drog� do drugiego pliku. Ka�dy element zostanie 
    // wpisany do wektora, kt�ry b�dzie odzwierciedla� zale�no�� node-page w pliku.
    // Algorytm bfs pozwala na bie��ce aktualizowanie pozycji dzieci w nowym indeksie
    // Na sam, koniec usuwamy orginalny plik i zast�pujemy go nowym


    // je�li drzewo nie jest puste
    if (root) {

        // utworzenie bloku do wpisywania nowej struktury
        string newFilename;
        filename=="index.bin" ? newFilename= "new_index.bin" : newFilename = "index.bin";

        Block* newFile = new Block(newFilename, SIZE_BUFFER_NODE, 0);
        int operationsOld = diskOperation;

        // wektor przechowuj�cy nowe pozycje
        vector<int> fileOrder;

        // kolejka odwiedzin
        queue<int> bfs;

        // dodanie roota jako pierwszego elementu
        bfs.push(rootPage);
        fileOrder.push_back(rootPage);
        BTreeNode tmp = *root;

        // aktualizacja strony "nowego" roota
        rootPage = 1;

        while (bfs.size()) {

            // przej�cie do dzieci (je�li !leaf)
            if (!tmp.leaf) {

                // dodanie do kolejki dzieci
                for (int i = 0; i < tmp.n + 1; i++) {
                    bfs.push(tmp.children[i]);
                    fileOrder.push_back(tmp.children[i]);
                }
            }

            // wpisanie tmp do nowego pliku
            newFile->write_node(tmp, fileOrder);

            // usuni�cie pierwszego elementu i pobranie kolejnego node'a
            bfs.pop();

            if (bfs.size())
                tmp = file->read_node(bfs.front());
        }

        // ilo�� operacji dyskowych starego pliku
        operationsOld = file->read_write() - operationsOld;

        // zamkni�cie starego pliku
        delete file;

        // usuni�cie starego pliku
        std::remove(filename.c_str());
        
        // nadpisanie starego pliku
        std::rename(newFilename.c_str(), filename.c_str());

        // aktualizacja bloku i
        file = newFile;
        
        // aktualizacja operacji dyskowych nowego pliku
        diskOperation = file->read_write();

        // wy�wietlenie operacji dyskowych
        cout << "File reorginized: " << endl;
        cout << "Disk operations: " << diskOperation + operationsOld << endl << endl;
    }
}

void BTree::traverse() {
    if (root != NULL) root->traverse(file);
}

int BTree::getDiskOperation() {
    return diskOperation;
}

void BTree::ifReorginize() {

    // obliczanie maksymalnej ilo�ci nod�w rpzy danej wysoko�ci
    int maxNodes = (pow(2 * t, height + 1) - 1) / 2 * t - 1;

    // je�li plik zawiera za du�o usuni�tych z drzewa nod�w 
    // to nale�y przeorganizowa� plik
    if (file->getCurrentNode() > maxNodes)
        reorginize();
}