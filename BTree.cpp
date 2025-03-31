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

        // root jest pe³ny
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

            // wysokoœæ++
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

    // zwrócenie wykonanych operacji dyskowych
    return (diskOperation - tmp);
}

pair<int, int> BTree::remove(int k) {
    if (rootPage == -1) {
        cout << "The tree is empty\n";
        return {- 1, 0};
    }

    int address = root->remove(k, file);

    // ustawianie nowego roota, na wypadek gdyby ten by³ pusty
    if (root->n == 0)
    {
        // przechowanie starego roota
        BTreeNode* tmp = root;

        // jeœli root jest liœciem to drzewo jest puste
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

        // aktualizacja wysokoœci drzewa
        height--;
    }

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;
    int tmp2 = diskOperation = file->read_write();

    // sprawdzenie czy reorganizacja w pliku jest potrzebna
    ifReorginize();

    // zwrócenie wykonanych operacji dyskowych
    return { address, (tmp2 - tmp) };
}


int BTree::printTree(int node_page, int depth) {

    // pobranie node'a z pliku
    BTreeNode node = file->read_node(node_page);

    // dodanie spacji dla g³êbokoœci drzewa
    for (int i = 0; i < depth; ++i) {
        cout << "  ";
    }

    cout << "Node: ";

    // wyœwietlanie kluczy w danym nodzie
    for (int i = 0; i < node.n; i++) {
        cout << node.keys[i] << " ";
    }
    cout << endl;

    // wyœwietlanie dzieci jeœli to nie liœæ
    if (!node.leaf) {
        for (int i = 0; i < node.n + 1; i++) {
            printTree(node.children[i], depth + 1);
        }
    }

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;
    diskOperation = file->read_write();

    // zwrócenie wykonanych operacji dyskowych
    return (diskOperation - tmp);
}

int BTree::getRootPage() {
    return rootPage;
}

pair<int, int> BTree::search(int k) {

    // aktualizacja operacji dyskowych
    int tmp = diskOperation;

    // jeœli roota nie ma to ni ma 
    if (rootPage == NULL)
        return { -1, 0 };

    // rekurencyjne szukanie adresu
    int page = root->search(k, file);
    diskOperation = file->read_write();

    if (page == -2)
        // jesli nie ma klucza k w drzewie
        return { -1 , diskOperation - tmp };
    else if (page != -1)
        // jeœli klucz by³ w rootcie to zwracamy adres spod klucza
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

    // Funkcja ma na celu przejechaæ obecne drzewo za pomoc¹ bfs'a i
    // zapisaæ przebyt¹ drogê do drugiego pliku. Ka¿dy element zostanie 
    // wpisany do wektora, który bêdzie odzwierciedla³ zale¿noœæ node-page w pliku.
    // Algorytm bfs pozwala na bie¿¹ce aktualizowanie pozycji dzieci w nowym indeksie
    // Na sam, koniec usuwamy orginalny plik i zastêpujemy go nowym


    // jeœli drzewo nie jest puste
    if (root) {

        // utworzenie bloku do wpisywania nowej struktury
        string newFilename;
        filename=="index.bin" ? newFilename= "new_index.bin" : newFilename = "index.bin";

        Block* newFile = new Block(newFilename, SIZE_BUFFER_NODE, 0);
        int operationsOld = diskOperation;

        // wektor przechowuj¹cy nowe pozycje
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

            // przejœcie do dzieci (jeœli !leaf)
            if (!tmp.leaf) {

                // dodanie do kolejki dzieci
                for (int i = 0; i < tmp.n + 1; i++) {
                    bfs.push(tmp.children[i]);
                    fileOrder.push_back(tmp.children[i]);
                }
            }

            // wpisanie tmp do nowego pliku
            newFile->write_node(tmp, fileOrder);

            // usuniêcie pierwszego elementu i pobranie kolejnego node'a
            bfs.pop();

            if (bfs.size())
                tmp = file->read_node(bfs.front());
        }

        // iloœæ operacji dyskowych starego pliku
        operationsOld = file->read_write() - operationsOld;

        // zamkniêcie starego pliku
        delete file;

        // usuniêcie starego pliku
        std::remove(filename.c_str());
        
        // nadpisanie starego pliku
        std::rename(newFilename.c_str(), filename.c_str());

        // aktualizacja bloku i
        file = newFile;
        
        // aktualizacja operacji dyskowych nowego pliku
        diskOperation = file->read_write();

        // wyœwietlenie operacji dyskowych
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

    // obliczanie maksymalnej iloœci nodów rpzy danej wysokoœci
    int maxNodes = (pow(2 * t, height + 1) - 1) / 2 * t - 1;

    // jeœli plik zawiera za du¿o usuniêtych z drzewa nodów 
    // to nale¿y przeorganizowaæ plik
    if (file->getCurrentNode() > maxNodes)
        reorginize();
}