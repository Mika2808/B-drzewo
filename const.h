#pragma once

// Dane do pliku g³ównego
#define SIZE_RECORD 7 // Liczba intów w rekordzie
#define SIZE_BLOCK 10 // N rekordów w systemie blokowym
#define SIZE_INT 4 // Rozmiar Inta- powinna byæ nazwa SIZE_ATTRIBIUTE dla szybkiego zmieniania
#define SIZE_BUFFER_RECORDS SIZE_RECORD*SIZE_BLOCK*SIZE_INT // Rozmiar bufora do wczytywania blokowego rekordów

// Dane do B-drzewa
#define T 50 // Minimalna liczba dzieci
#define SIZE_DATA 2 // Ile danych zawiera klucz
#define SIZE_BUFFER_NODE ((2*T-1)*SIZE_DATA + 2*T)*SIZE_INT // buffer size b-tree: (keys) (2t-1)*SIZE_DATA + (pointers) 2*t