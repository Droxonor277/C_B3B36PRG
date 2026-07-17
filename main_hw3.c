#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SIZE 10


// načte stringy a zkontroluje znaky
char* ReadInput(int* len1, _Bool *pismena);


// Vrátí počet shodných písmen
int CorrectLetters(int lenght,char* string1,char* string2);

// Posun o +1 v každém znaku stringu
void ShiftByOne(int lenght, char* string1);   

// Podmínky posunu
char RotateByOne(char string1);

// Vypisuje výsledek na výstup
void printMsg(char *msg, int len);

/* The main program */
int main(int argc, char *argv[])
{
  // TODO - insert your code here

    // Používané proměné v programu
    int len1 = 0; // delka prvního stringu
    int len2 = 0; // delka druhého stringu

    _Bool pismena1 = true; // kontrolní konstata správnosti vstupu
    _Bool pismena2 = true; // kontrolní konstata správnosti vstupu
    
    char* string1 = NULL; // rotovaný string
    char* string2 = NULL; // odposlechnutý string
    char* string3 = NULL; // nejlepší schodný string
    
    int beforecorretNumbers = 0; // nejvyší počet schodných znaků před rotací znaků
    int aftercorretNumbers = 0; // nejvyší počet schodných znaků  po rotaci znaků
    int ret = 0; // navrátová hodnota


    // Načtení vstupů 
    string1 = ReadInput(&len1, &pismena1);
    if (string1) 
    {
        string2 = ReadInput(&len2, &pismena2);
    }


    // vytvoření paměti pro nejlepší schodný string
    string3 = (char*)malloc(len1+2);



    //Kontrola délky obou stringů
    if (len1 != len2)
    {
        ret = 101;
    }



    //Kontrola jestli vstupy obsahují jenom stringy    
    
    if (!(pismena1) || !(pismena2))
    {
        ret = 100;
    }






    // navrecení chybových hodnot
    if (ret != 0)
    {
        if (ret == 100)
        {
            fprintf(stderr, "Error: Chybny vstup!\n");
        }
        if (ret == 101)
        {
            fprintf(stderr, "Error: Chybna delka vstupu!\n");           
        }
    }

    // Kontrola správnosti 
    if (ret == 0)
    {
        
        for (int i = 0; i <=51; i++)
        {
            
            ShiftByOne(len1, string1);
            
            aftercorretNumbers = CorrectLetters(len1,string1,string2);
            
            if (aftercorretNumbers > beforecorretNumbers)
            {    
                
                beforecorretNumbers = aftercorretNumbers;
                
                for(int i = 0; i < len1 -1 ; i++ )
                {
                    string3[i] = string1[i];
                }
                
            }
            
        }
        printMsg(string3, len1);    
        
    }

    free(string1);
    free(string2);
    free(string3);
    return ret;
}


char* ReadInput(int *len1, _Bool *pismena)
{
    char *str = (char*)malloc(SIZE); // načtený string
   
    int len = 0; // délka stringu
   
    int capacity = SIZE; // capacita paměti

    int c; // proměné pro načítání vstupních znaků
    while ((c = getchar()) != EOF && c != '\n') // načítá stringy dokud neskončí výpis nebo se nepřeskočí na další řádek po písmenu
    {
        
        if ((len + 3) == capacity)// kontrola jestli je potřeba zvětšit paměť kvůli nedostatku paměti
        {
            
            char *tmp = realloc(str, capacity * 2); // zvýšení paměti na *2
            if (tmp == NULL) // pokud se tato operace nepovede
            {
                fprintf(stderr, "ERROR malloc!");
                free(str); // uvolním pamět
                str = NULL; // Nahlásím, že to nevyšlo 
                len = 0; // délka zprávy je 0
                return NULL;
            }
            capacity *= 2;
            str = tmp;
        }

        // kontrola vstupních znaků 
        if(!('a' <= c && c <= 'z') && !('A' <= c && c <= 'Z'))
        {
            *pismena = false;
        }
        str[len++] = c;// načítá pomalu znaky a dává je do jednoho stringu pomocí zvyšování délky +1

        
    }    

     
    
    len++;
    str[len] = '\0'; 
    *len1 = len;
    return str;
}

int CorrectLetters(int lenght,char* string1,char* string2)
{
    // předpokládá stejnou délku stringů
    int correct_letters = 0;
    for (int i = 0; i < lenght - 1; i++)
    {
        if (string1[i] == string2[i])
        {
            correct_letters++;
        }
    }
    return correct_letters;
}

void ShiftByOne(int lenght, char* string1)
{
    for(int i = 0; i < lenght - 1; i++)
    {
        string1[i] = RotateByOne(string1[i]);
    }

}  

char RotateByOne(char c)

{
    
    if (c == 'Z')
    {
        return 'a';
    }
    if (c == 'z')
    {
        return 'A';
    } 
    if(c == '\0')
    {
        return c;
    }     
    else
    {
        return (c + 1);
    }

}


void printMsg(char *msg, int len)
{
    if  (msg)
    {
        for (int i = 0; i < len -1; ++i)
        {
            putchar(msg[i]);
      
        }   
        putchar('\n');
    }   
}
