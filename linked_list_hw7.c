#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "linked_list.h"

// zadefinování struktury
typedef struct node
{
    int data;
    struct node *next;
} node;

node *List; // vytvoření globálního ukazatele, který bude ukazovat na začátek

node *createnode(int data, node *nextnode)
{
    // - alokace nové paměti pro další jeden prvek 
    node* mynode = malloc(sizeof(node));

    // - kontrola jestli je na alokaci dost místa
    if (mynode == NULL)
    {
        fprintf(stderr, "ERROR: Low space for memory alocation!!\n");
        return NULL;
    } 
    mynode->next = nextnode; // předání ukazatele, aby ukazoval na další prvek, než ten přidaný
    mynode->data = data; // vložení dat do fronty

    return mynode;
}

_Bool push(int entry)
{
    // - kontrola správné vstupní hodnoty
    if ( entry < 0)
    {
        return false;
    }

    // - Pokud-li je seznám prázdný, tak vložit prvek na konec seznamu, jako na jeho začátek
    // - jinak procházek prvek od 2 pozice až na konec fronty, kam ho následně umístit
    if (List == NULL)
    {
        List = createnode(entry, NULL);
        return true;
    } else {
        node *temp = List; // vytvoření dalšího ukazatele
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = createnode(entry, NULL);
        return true;
    }
}

int pop()
{
    // kotrola jestli list není prázdný
    if (List == NULL)
    {
        return -1;
    }

    int ret = 0; // kontrolní navratová hodnota

    // - pokud-li je ve frontě jenom jeden prvek, tak ho odstanit první prvek a nastavit ukazatel
    // a uvolnit paměť
    // - jinak pokud-li je v více prvků ve frontě, tak smazat první prvek a nastavit List jako 
    // první ukazatel na druhý prvek od začátku a uvolnit paměť
    if (List->next == NULL)
    {
        ret = List->data;
        free(List);
        List = NULL;        
        return ret;
    } else {
        node *temp2 = List;
        ret = List->data;
        List = temp2->next;
        free(temp2);
        return ret;
    }
}

_Bool insert(int entry)
{
    // - Kontrola správnosti čísla
    if ( entry < 0)
    {
        return false;
    } 

    // - pokud-li je fronta prázdná, tak vytvořit novou a zadat do ní první číslo
    if( List == NULL)
    {
        List = createnode(entry, NULL);
        return true;
    }

    // - pokud-li je první číslo měnší nebo rovno vkládanému číslu,
    // tak vložit vkládané číslo před něj
    if (List->data <= entry)
    {
        List = createnode(entry, List);
        return true;
    }

    // - procházení frontou až na konec a dání čísla před první menší nebo stejné číslo,
    // jinak ho vložit až na konec fronty 
    node *temp = List; // vytvoření dalšího ukazatele
    while(temp->next != NULL && temp->next->data > entry)
    {
        temp = temp->next;
    }

    // - vytvoření dalšího místa pro vkládaný prvek a posunutí fronty za něj
    // - platí to i pro to vložení prvku na konec (temp->next = NULL, pak)
    temp->next = createnode(entry, temp->next);
    
    return true;
}


_Bool erase(int entry)
{
    // - Kontrola správnosti čísla
    if ( entry < 0)
    {
        return false;
    }

    _Bool ret = false; // kontrolní návratová hodnota pravdivosti

    if (List)
    {
        node *temp = List; // vytvoření dalšího ukazatele

        // - cyklus na odstranění všech shodných čísel od 2 pozice až do konce
        while(temp->next != NULL)
        {
            node *todel; // vytvoření dalšího ukazatele

            // - kontrola jeslti se číslo rovná hledanému číslu
            // - jinak pokračovat v prohledávání 
            if (temp->next->data == entry)
            {
                todel = temp->next;
                temp->next = temp->next->next;
                free(todel);
                ret = true;
            } else {
            temp = temp->next;
            }
        }

        // - odstranění prvku na první pozici, pokud je shodný s hledaným číslem
        if (List->data == entry)
        {
            temp = List;
            List = List->next;
            free(temp);
            ret = true;
        }
    }

    return ret;
}

int getEntry(int idx)
{
    // lokální proměná na porovnání s idx, abychom věděli, že jsme narazili na pravou pozici
    int pozition = 0; 
    
    // kontrola, jeslti nám uživatel zadal správnou pozici
    if ( idx < 0)
    {
        return -1;
    } 

    int ret = -1; // napratová hodnota, která je předem nastavené na chybu

    node *temp = List; // nastavení *temp na stený odkaz jako je List, tj. na začátek
    // - nastavujeme to protože potřebujeme index a nechceme přenastavovat index List,
    // který nám předefinovaně ukazuje na začátek


    // - podmínka na zrychlení programu, aby nám zbytečne neprocházel prvky,
    // když while to prochází, až od 2. prvku
    if (idx > 0)
    {    
        // - while cyklus to bude prokledávat od druhého prvku
        // - pokud-li se pozition == idx, tak se zastavý nebo pokud
        // je temp->next ukazuje na NULL
        while (idx != pozition && temp->next != NULL)
        {
            pozition++; 
            temp = temp->next;
        }
    }

    // - pokud-li je pozition se nerovná idx, ale temp->next se rovná NULL
    // - toto je kontrola, aby nám ret vracela vhodnou návratovou hodnotu
    // pokud-li je idx > pozition -----> chyba (-1) 
    if (idx == pozition) // && idx != 0)
    {
        ret = temp->data;
    }

    return ret;
}

int size(void)
{
    int counter = 0; // počítadlo obsazenosti
    node *temp = List; // vytvoření dalšího ukazatele

    // Kontrola neprázdné fronty
    if (List) 
    {
        // +1 za první prvek
        counter++;

        // - cyklus nám počítá kolik je prvků v List
        // - počítá nám to až od druhého prvku do konce
        while (temp->next != NULL)
        {
            counter++;
            temp = temp->next;
        }
    }

    return counter;
}

void clear()
{
    // - kontrola neprázdné fronty
    if (List)
    {
        node *temp = List; // vytvoření dalšího ukazatele

        // - uvolní nám prvky od začátku až do konce fronty
        while (temp != NULL)
        {
            node *prev = temp;
            temp = temp->next;
            free(prev);
        }
        List = 0;
    }
}
