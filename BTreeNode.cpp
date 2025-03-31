#include "BTreeNode.h"
#include"Block.h"

BTreeNode::BTreeNode() {}

BTreeNode::BTreeNode(int t1, bool leaf1) {
    t = t1;
    leaf = leaf1;

    keys = new int[2 * t - 1];
    addresses = new int[2 * t - 1];
    children = new int[2 * t];

    n = 0;
}


// Konstruktor Node'a wczytanego z pliku. Wektor posiada wszystkie 
// inty z node'a 
BTreeNode::BTreeNode(int t1, vector<int> node, int pointer) {

    // inicjalizacja zmiennych
    t = t1;
    keys = new int[2 * t - 1];
    addresses = new int[2 * t - 1];
    children = new int[2 * t];
    n = 0;
    pointerInFile = pointer;
    node[0] != -1 ? leaf = false : leaf = true;

    // przepisanie wartoœci z odczytanego wektora do node'a
    int index_x = 0, index_a = 0, index_c = 0;
    for (int i = 0; i < node.size(); i++) {
        if (i % 3 == 0) {
            children[index_c] = node[i];
            index_c++;
        }
        else if (i % 3 == 1) {
            if (node[i] == -1) {
                break;
            }
            keys[index_x] = node[i];
            index_x++;
            n++;
        }
        else if (i % 3 == 2) {
            addresses[index_a] = node[i];
            index_a++;
        }
    }
}

// Funkcja do zwracania pierwszego wiêkszego lub 
// równego klucza 
int BTreeNode::findKey(int k) {
    int idx = 0;
    while (idx < n && keys[idx] < k)
        ++idx;
    return idx;
}

// Funkcja do usuwania klucza k z poddrzewa (wraz z tym nodem)
int BTreeNode::remove(int k, Block* file) {
    
    // odszukanie odpowiedniego miejsca na klucz
    int idx = findKey(k);

    // klucz jest w tymm nodzie
    if (idx < n && keys[idx] == k) {
        
        int address = addresses[idx];

        // wywo³anie odpowiedniego usuwania elementu w zale¿noœci 
        // od tego czy node jest liœciem
        if (leaf)
            removeFromLeaf(idx, file);
        else
            removeFromNonLeaf(idx, file);

        return address;
    }
    else {

        // jeœli to liœæ to nie ma danego klucza
        if (leaf) {
            return -1; // zwracamy -1 bo nie ma klucza k w drzewie
        }

        // The key to be removed is present in the sub-tree rooted with this node
        // The flag indicates whether the key is present in the sub-tree rooted
        // with the last child of this node
        bool flag = ((idx == n) ? true : false);

        // wype³nienie dziecka jeœli ma mniej ni¿ t kluczy
        BTreeNode child = file->read_node(children[idx]);
        if (child.n < t)
            fill(idx, &child, file);

        // If the last child has been merged, it must have merged with the previous
        // child and so we recurse on the (idx-1)th child. Else, we recurse on the
        // (idx)th child which now has atleast t keys
        if (flag && idx > n) {
            BTreeNode child1 = file->read_node(children[idx - 1]);
            child1.remove(k, file);
        }
        else
            child.remove(k, file);
    }
}

// Funkcja do usuwanie idx klucza z liœcia
void BTreeNode::removeFromLeaf(int idx, Block* file) {

    // cofanie o jedn¹ pozycjê klcuzy i adresów
    for (int i = idx + 1; i < n; ++i) {
        keys[i - 1] = keys[i];
        addresses[i - 1] = addresses[i];
    }

    // zmniejszenie iloœci kluczy
    n--;

    // wpisanie zauktualizowanego node'a do pliku
    file->write_node(*this, pointerInFile);

    return;
}

// Funkcja do usuwanie idx klucza z nie liœcia
void BTreeNode::removeFromNonLeaf(int idx, Block* file) {

    int k = keys[idx];

    // If the child that precedes k (C[idx]) has atleast t keys,
    // find the predecessor 'pred' of k in the subtree rooted at
    // C[idx]. Replace k by pred. Recursively delete pred
    // in C[idx]
    BTreeNode child = file->read_node(children[idx]);
    BTreeNode child1 = file->read_node(children[idx + 1]);

    if (child.n >= t) {
        pair<int, int> pred = getPred(idx, file);
        keys[idx] = pred.first;
        addresses[idx] = pred.second;

        // wpisanie zaktualizowanego node'a do pliku
        file->write_node(*this, pointerInFile);

        // usuniêcie przeniesionego elementu
        child.remove(pred.first, file);
    }

    // If the child C[idx] has less that t keys, examine C[idx+1].
    // If C[idx+1] has atleast t keys, find the successor 'succ' of k in
    // the subtree rooted at C[idx+1]
    // Replace k by succ
    // Recursively delete succ in C[idx+1]
    else if (child1.n >= t)
    {
        pair<int, int> succ = getSucc(idx, file);
        keys[idx] = succ.first;
        addresses[idx] = succ.second;

        // wpisanie zakktualizowanego node'a do pliku
        file->write_node(*this, pointerInFile);

        // usuniêcie przeniesionego elementu
        child1.remove(succ.first, file);
    }

    // If both C[idx] and C[idx+1] has less that t keys,merge k and all of C[idx+1]
    // into C[idx]
    // Now C[idx] contains 2t-1 keys
    // Free C[idx+1] and recursively delete k from C[idx]
    else
    {
        merge(idx, &child, &child1, file);
        child.remove(k, file);
    }
    return;
}

// Funkcja do zwracanie maksymalnego elementu z liœci poprzednich
pair<int, int> BTreeNode::getPred(int idx, Block* file) {
    // szukaniego najprawszego liœcia
    BTreeNode cur = file->read_node(children[idx]);
    while (!cur.leaf)
        cur = file->read_node(cur.children[cur.n]);

    // zwrócenie ostatniego klucza
    return { cur.keys[cur.n - 1], cur.addresses[cur.n - 1] };
}

pair<int, int> BTreeNode::getSucc(int idx, Block* file) {

    // szukanie najlewszego liœcia
    BTreeNode cur = file->read_node(children[idx + 1]);
    while (!cur.leaf)
        cur = file->read_node(cur.children[0]);

    // zwrócenie pierwszego klucza
    return { cur.keys[0], cur.addresses[0] };
}

// Wype³nianie idx dziecka, które ma mniej ni¿ t kluczy
void BTreeNode::fill(int idx, BTreeNode* child, Block* file) {

    // wczytanie rodzeñstwa
    BTreeNode leftSibling;
    BTreeNode rightSibling;

    if (idx != 0)
        leftSibling = file->read_node(children[idx - 1]);
    else if (idx != n)
        rightSibling = file->read_node(children[idx + 1]);

    // po¿yczanie klucza od lewego rodzeñstwa
    if (idx != 0 && leftSibling.n >= t)
        borrowFromPrev(idx, child, &leftSibling, file);

    // po¿yczanie klucza od prawego rodzeñstwa
    else if (idx != n && rightSibling.n >= t)
        borrowFromNext(idx, child, &rightSibling, file);

    // Merge C[idx] with its sibling
    // If C[idx] is the last child, merge it with its previous sibling
    // Otherwise merge it with its next sibling
    else
    {
        if (idx != n)
            merge(idx, child, &rightSibling, file);
        else
            merge(idx - 1, &leftSibling, child, file);
    }
    return;
}

// Funkcja zapo¿yczania klucza z lewego rodzeñstwa do idx dziecka
void BTreeNode::borrowFromPrev(int idx, BTreeNode* child, BTreeNode* sibling, Block* file) {
    
    // Ostatni klucz lewego rodzica przechodzi do rodzica a klucz z rodzica
    // przechodzi do dziecka idx. Czyli potrzebujemy trzy node'y do tej 
    // operacji: dziecko, rodzeñstwo i rodzic

    // przesuwanie kluczy do przodu u dziecka
    for (int i = child->n - 1; i >= 0; --i) {
        child->keys[i + 1] = child->keys[i];
        child->addresses[i + 1] = child->addresses[i];
    }

    // jeœli dziecko nie jest liœciem to musimy przenieœæ wskaŸniki na dzieci
    if (!child->leaf)
    {
        for (int i = child->n; i >= 0; --i)
            child->children[i + 1] = child->children[i];
    }

    // ustawianie klucza rodzica u dziecka
    child->keys[0] = keys[idx - 1];
    child->addresses[0] = addresses[idx - 1];

    // przes³anie wskaŸników na dzieci
    if (!child->leaf)
        child->children[0] = sibling->children[sibling->n];

    // ustawienie u rodzica klucza z rodzeñstwa
    keys[idx - 1] = sibling->keys[sibling->n - 1];
    addresses[idx - 1] = sibling->addresses[sibling->n - 1];

    // aktualizacja iloœci kluczy
    child->n += 1;
    sibling->n -= 1;

    // wpisanie wszystkich nodów do pliku
    file->write_node(*child, child->pointerInFile);
    file->write_node(*sibling, sibling->pointerInFile);
    file->write_node(*this, pointerInFile);

    return;
}

// Funkcja zapo¿yczania klucza z prawego rodzeñstwa do idx dziecka
void BTreeNode::borrowFromNext(int idx, BTreeNode* child, BTreeNode* sibling, Block* file) {

    // Analogiczna funkcja co do tej wy¿ej

    // przepisanie do dziecka klucza od rodzica
    child->keys[(child->n)] = keys[idx];
    child->addresses[(child->n)] = addresses[idx];

    // przepisanie wskaŸnika na dzieci od rodzeñstwa, je¿eli jakieœ jest (!leaf)
    if (!(child->leaf))
        child->children[(child->n) + 1] = sibling->children[0];

    // przepisanie do rodzica pierwszego klucza z rodzeñstwa
    keys[idx] = sibling->keys[0];
    addresses[idx] = sibling->addresses[0];

    // przesuniêcie u rodzeñstwa kluczy do przodu
    for (int i = 1; i < sibling->n; ++i) {
        sibling->keys[i - 1] = sibling->keys[i];
        sibling->addresses[i - 1] = sibling->addresses[i];
    }

    // przesuniêcie wskaŸników na dzieci (jesli !leaf)
    if (!sibling->leaf)
    {
        for (int i = 1; i <= sibling->n; ++i)
            sibling->children[i - 1] = sibling->children[i];
    }

    // aktualizacja kluczy
    child->n += 1;
    sibling->n -= 1;

    // wpisanie wszystkich nodów do pliku
    file->write_node(*child, child->pointerInFile);
    file->write_node(*sibling, sibling->pointerInFile);
    file->write_node(*this, pointerInFile);

    return;
}

// Funkcja merguj¹ca dziecko i rodzeñstwo
void BTreeNode::merge(int idx, BTreeNode* child, BTreeNode* sibling, Block* file) {

    // przeniesienie klucza z rodzica na œrodek dziecka
    child->keys[t - 1] = keys[idx];
    child->addresses[t - 1] = addresses[idx];

    // przeniesienie kluczy i adresów z rodzeñstwa do dziecka
    for (int i = 0; i < sibling->n; ++i) {
        child->keys[i + t] = sibling->keys[i];
        child->addresses[i + t] = sibling->addresses[i];
    }

    // przenisienie wskaŸników na dzieci (jeœli !leaf)
    if (!child->leaf) {
        for (int i = 0; i <= sibling->n; ++i)
            child->children[i + t] = sibling->children[i];
    }

    // przeniesienie o jeden w ty³ kluczy u rodzica 
    // ¿eby uzupe³niæ lukê
    for (int i = idx + 1; i < n; ++i) {
        keys[i - 1] = keys[i];
        addresses[i - 1] = addresses[i];
    }

    // przeniesienie pointerów na dzieci jeden do przodu u rodzica
    for (int i = idx + 2; i <= n; ++i)
        children[i - 1] = children[i];

    // aktualizowanie liczby kluczy
    child->n += sibling->n + 1;
    n--;

    // wpisanie do pliku zaktualizowanego rodzica i dziecka
    file->write_node(*child, child->pointerInFile);
    file->write_node(*this, pointerInFile);

    return;
}

void BTreeNode::insertNonFull(int k, int a, Block* file) {

    // indekks najprawszego elementu
    int i = n - 1;

    // Jesli to liœæ to mo¿emy dodaæ klucz
    if (leaf == true) {

        // szukanie miejsca dla nowego klucza i przesuwanie kluczy
        while (i >= 0 && keys[i] > k) {
            keys[i + 1] = keys[i];
            addresses[i + 1] = addresses[i];
            i--;
        }

        // dodanie klucza i atualizacja liczby kluczy
        keys[i + 1] = k;
        addresses[i + 1] = a;
        n++;

        // aktualizacja w pliku
        file->write_node(*this, pointerInFile);
    }
    else {
        // szukanie odpowiendiego dziecka dla klucza
        while (i >= 0 && keys[i] > k)
            i--;
        BTreeNode child = file->read_node(children[i + 1]);

        // sprawdzenie czy dziecko jest pe³ne
        if (child.n == 2 * t - 1) {

            // split jak dzieciak jest pe³ny
            splitChild(i + 1, &child, file);

            // After split, the middle key of C[i] goes up and
            // C[i] is splitted into two.  See which of the two
            // is going to have the new key
            if (keys[i + 1] < k)
                child = file->read_node(children[i + 2]);
        }
        child.insertNonFull(k, a, file);
    }
}

// Funkcja do splitowania pe³nego dziecka 
void BTreeNode::splitChild(int i, BTreeNode* child, Block* file) {

    // tworzenie nowego node'a, który przechowa t-1 kluczy 
    // od dziecka y
    BTreeNode z = BTreeNode(child->t, child->leaf);
    z.n = t - 1;

    // przenoszenie (t-1) kluczy z y do z
    for (int j = 0; j < t - 1; j++) {
        z.keys[j] = child->keys[j + t];
        z.addresses[j] = child->addresses[j + t];
    }

    // przenoszenie t dzieci z y do z (jesli dziecko !leaf)
    if (child->leaf == false) {
        for (int j = 0; j < t; j++)
            z.children[j] = child->children[j + t];
    }

    // aktualizacja iloœci kluczy u dziecka
    child->n = t - 1;

    // tworzenie miesjca na nowe dziecko u rodzica
    for (int j = n; j >= i + 1; j--)
        children[j + 1] = children[j];

    // przypsianie wskaxnika w pliku do rodzica
    children[i + 1] = file->getCurrentNode() + 1;

    // przeniesienie klucza dziecka do rodzica
    for (int j = n - 1; j >= i; j--) {
        keys[j + 1] = keys[j];
        addresses[j + 1] = addresses[j];
    }

    // przeniesienie œrodkowego klucza z dziecka do rodzica
    keys[i] = child->keys[t - 1];
    addresses[i] = child->addresses[t - 1];

    // aktualizacja liczby kluczy u rodzica
    n = n + 1;
    file->write_node(*this, pointerInFile);
    file->write_node(*child, child->pointerInFile);
    file->write_new_node(z);

}

// Funkcja do przechodzenie przez poddrzewo ³¹cznie z tym nodem
void BTreeNode::traverse(Block* file) {

    int i;
    for (i = 0; i < n; i++) {

        // jeœli !leaf to wyœwietlanie dzieci
        if (leaf == false) {
            BTreeNode child = file->read_node(children[i]);
            child.traverse(file);
        }

        // wyœweitlanie klucza rodzica
        cout << " " << keys[i];
    }

    //  ostatnie dziecko
    if (leaf == false) {
        BTreeNode child = file->read_node(children[i]);
        child.traverse(file);
    }
}

// Funkcja do wyszukiwania klucza k w poddrzewie
// z tym nodemm jako rodzicem
int BTreeNode::search(int k, Block* file) {

    // szukanie pierwszego klucza wiêkszego lub równego k
    int i = 0;
    while (i < n && k > keys[i])
        i++;

    // zwrócenie adresu je¿eli znaleziono klucz
    if (keys[i] == k)
        return addresses[i];

    // jeœli nie ma a to liœæ to nie ma danego elementu w drzewie
    if (leaf == true)
        return -2;

    // przejœcie do dziecka
    return file->read_node(children[i]).search(k, file);
}