#include "queue.h"

// vrací zvýšení index o 1 nebo přesune index na začátek 
void incrementIndex(queue_t *queue, int *index)
{
    // jeslti že se capacita rovná indexu +1, tak se index přesune na 0
    if (queue->capacity == *index + 1) 
    {
        *index = 0; // přesun indexu na pozici 0
    } else {
        (*index) += 1; // zvýšení indexu o 1
    }
}

// vrací jestli je prvek v poli a taky nastavý jaký index se bude vracet
bool addtoIndex(queue_t *queue, int *index, int dif)
{    
    // kontorla jeslti jeho x-tá souřadnice v poli a  taky jestli je nezáporná
    if (dif >= get_queue_size(queue) || dif < 0)
    {
        return false;
    } 

    // kontrola jestli index se přehoupna přes konec pole
    // chtěná x-tá pozice je větší než počet prvků za start indexem
    if (dif > queue->capacity-(queue->indexStart+1)) 
    {
        // přehoupnutí přes konec na pozici 0
        dif = dif - (queue->capacity - (queue->indexStart + 1));

        // nastavý vrátcený index od 0 a -1 je kvůli tomu, že je prvek počítaný od 0 
        *index = ((dif));  
    } else{
        // možnost kdy se index nepřehoupne přes konec pole,
        // ale jenom se nastavý index za (queue->indexStart)
        *index = (dif) + (queue->indexStart); 
    }

    return true;
}



// vytvoření pole a ostatní potřebné hodnoty
queue_t* create_queue(int capacity)
{
    // vytvoření queue
    queue_t* myQ = malloc(sizeof(queue_t)); // vytvoří paměť pro strukturu
    if (myQ == NULL)
    {
        fprintf(stderr, "ERROR: little space for memory alocation!!\n");
        return NULL;
    } 
    myQ->capacity = capacity; // uložení počtu dat 
    myQ->indexStart = 0; // počáteční pozice
    myQ->indexEnd = 0; // koncová pozice
    myQ->availability = 0; // obsazenost

    //alokace pole ukazatelů
    myQ->storage = malloc(sizeof(void*) * (capacity));
    if (myQ->storage == NULL)
    {
        fprintf(stderr, "ERROR: little space for memory alocation!!\n");
        return NULL;
    } 
    
    return myQ; 
}


bool push_to_queue(queue_t *queue, void *data)
{
    // kontrola jeslti je start rovný end a zároveň jeslti je obsazenost rovná maximální capacitě 
    if ((queue->availability) == (queue->capacity)) 
    {
        return false;
    }
    
    queue->storage[queue->indexEnd] = data; // uložení dat do místa ve frontě 
    (queue->availability) += 1; // při přidání prvku se obsazenost zvýší
    incrementIndex(queue, &(queue->indexEnd));

    return true;
}

void* pop_from_queue(queue_t *queue)
{
    // kontrola jestli se start rovná end a zároveň validita je nulová
    if((queue->indexStart == queue->indexEnd) &&(queue->availability) == 0)
    {
        return NULL;
    }

    // uložení prvnu do lokální proměné, aby se nepletl s navrácením
    void* tmp = queue->storage[queue->indexStart]; 
    queue->storage[queue->indexStart] = NULL; // resetování indexu
    queue->availability -= 1; // odečtení obsazenosti
    incrementIndex(queue,&(queue->indexStart));

    return tmp;
}

void* get_from_queue(queue_t *queue, int idx)
{
    // pozice x-tého indexu
    int pozition = 0;
    
    // idx je chtěný index prvku na x-té pozici od začátku
    // zjištění jestli je hodnota validní 
    _Bool Valid = addtoIndex(queue, &pozition, idx);

    // kontrola Validity pozice
    if(!(Valid))
    {
        return NULL;
    } 

    return queue->storage[pozition];
}

void delete_queue(queue_t *queue)
{
    free(queue->storage);
    free(queue);
}

int get_queue_size(queue_t *queue)
{
    if ((queue->indexStart == queue->indexEnd) && (queue->availability == 0))
    {
        return 0;
    } else if (queue->indexStart < queue->indexEnd)
    {
        return (queue->indexEnd - queue->indexStart);
    } else                                  
    {
        return ((queue->capacity - queue->indexStart) + (queue->indexEnd)); 
    }
}


