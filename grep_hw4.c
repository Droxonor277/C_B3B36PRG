#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Vrací trur když je v řádku pattern
_Bool matchPatterRex(char *line,char* pattern, int patternword );

// Vrací trur když je v řádku pattern
_Bool matchPatter(char *line,char* pattern, int patternword );

// Vrací načtený řádek ze souboru
char *readLine(FILE *f);

// vrací velikost daného patternu
int len(char* pattern);

_Bool checkparameter(char *argv[]);
#define SIZE 10

/* The main program */
int main(int argc, char *argv[])
{
    // TODO - insert your code here


    //načtení pamametrů
    char *parametr = NULL;
    char *pattern = NULL;
    const char *filename = NULL;
    if (argc > 3)
    {
        parametr = argc > 1 ? argv[1] : NULL;
        pattern = argc > 2 ? argv[2] : NULL;
        filename = argc > 3 ? argv[3] : NULL;    
    } else {
        pattern = argc > 1 ? argv[1] : NULL;
        filename = argc > 2 ? argv[2] : NULL;
    }
   
    //printf("%s %s %s\n",parametr,pattern,filename);
    
    FILE *f;    
    if(filename != NULL)
    {    
        f = fopen(filename, "r");
    } else{
        f = stdin;
    }


    // kontrola, jestli jsou přítomny oba argumenty
    if (!pattern)
    {
        fprintf(stderr, "Error: wrong input\n");
        return 101;
    }


    // načtení souboru
    //FILE *f = fopen(filename, "r");

    int lenword = len(pattern);
    //printf("%d\n",lenword);

    // kontrola správného načtení souboru
    if (f == NULL)
    {
        fprintf(stderr, "Error: cannot open file %s\n", filename);
        return 102;
    }

    // načítání pořádku v souboru
    char *line = readLine(f);


    int returnvalue = 0;
    while (line != NULL)
    {

        if(parametr)
        {    
            //kontrola schody s patternem
            if (matchPatterRex(line, pattern, lenword))
            {
            printf("%s\n", line);
            returnvalue += 1;
            }
        } else{
            if (matchPatter(line, pattern, lenword))
            {
            printf("%s\n", line);
            returnvalue += 1;                
            }
        }
        free(line);
        line = readLine(f);
    }

    if(returnvalue == 0)
    {
      return 1;
    }  


    return 0;
}


_Bool checkparameter(char *argv[1])
{
    char *controlword = "-E";
    int numberofcontrolword = 0;
    for (int i = 0; controlword[i] != '\0'; ++i)
    {
        if (controlword[i] == argv[1][i])
        {
            numberofcontrolword++;

        }
    }
    if (numberofcontrolword == 2)
    {
        return true;
    }
    return false;
    
}

int len(char* pattern)
{
    int sign = 0;
    while(pattern[sign] != '\0')
    {
        sign++;
    }
    return sign;
}


char *readLine(FILE *f)
{
    int capacity = SIZE;
    char *ret = malloc(capacity + 1);
    int len = 0;
    int r;

    // cyklus, který načítá znak po znaku v řádku a ukládáje do stringu
    while ((r = getc(f)) && r != EOF && r != '\n')    
    {
        if (len == capacity)
        {
            char *t = realloc(ret, capacity + SIZE + 1);
            if (t == NULL)
            {
                free(ret);
                fprintf(stderr, "ERROR: Cannot realloc\n");
                exit(-1);
            }
            ret = t;
            capacity += SIZE;
        }
        ret[len++] = r;
    } // end reading chars

    if (r == EOF && len == 0)
    {
        free(ret);
        ret = NULL;
    }
    else
    {
        ret[len] = '\0';
    }
    return ret;
}

_Bool matchPatterRex(char *line, char *word, int patternword)
{
  
    
    
    int stav = 0; // stav kontroly
    
    
    int j = 0; // pozice v paternu
    //char word[20] = "colou*r";
    //char line[20] = "cocolouuurxd";
    //int r = scanf("%s" "%s", word, line);
    //printf("%s" "%s", word, line);

    //int patternword = 7; // délka patternu



    int index = 0; // počet shodných písmen
    int jedenznak = 0; // kontrola jednoho znaku
    int chyba3 = 0; // chyba ve stavu 3
    int jedna = 0;

    int tri = 0;
    for (int i = 0; line[i] != '\0'; i++)
    {
        //printf("%d - index každého kroku po %d\n", index, i);
        //printf("%c - string %c - patternword\n", line[i], word[j]);
        // kontrola jestli je ve slově "?" -> přesun do 1
        jedna = 0;
        chyba3 = 0;
        jedenznak = 0;
        
        if (word[j+1] == '?')
        {
            //printf("%d - moc\n", j);
            stav = 1;
        }

        // kontrola jestli je ve slově "*" -> přesun do 2
        if (word[j+1] == '*')
        {
            stav = 2;
        }

        // kontrola jestli je ve slově "+" -> přesun do 3
        if (word[j+1] == '+')
        {
            
            stav = 3;
        }       



        //printf("%c\n", line[i]);


        // doplňková kontrola stringu pokud je stav == 1
        if (stav == 1)
        {
            //printf("%c != %c\n",word[j], line[i]);


            // podmínka, když tam není předešlé písmeno ve stringu
            if (word[j] != line[i])
            {
                j += 2;
                index += 2;
                jedna++;
            }

            //printf("%c == %c\n",word[j], line[i]);
            
            // podmínka, když tam je předešlé písmeno ve stringu
            if (word[j] == line[i] && jedna == 0)
            {
                j += 2;
                i++;
                //dva++;
                index += 2;
                //printf("%c == %c\n",word[j], line[i]);
            }
            
            //printf("%d\n",index);
            //stav = 0;
            //printf("%d\n",index);
            stav = 0;
        }
   

        //printf("%c == %c - po stavu 1\n",word[j], line[i]);

        // doplňková kontrola stringu pokud je stav == 2 "*"
        if (stav == 2)
        {
            //printf("%d - pozice v paternu - stav\n", j);

            //printf("%c != %c\n",word[j], line[i]);
            // podmínka, když tam není předešlé písmeno ve stringu
            if (word[j] != line[i])
            {
                j += 2;
                jedenznak++;
                index += 2;
            }


            //printf("%c == %c\n",word[j], line[i]);
            if (jedenznak == 0)
            {
                //prochází stringem Line dokud je v řádku za sebou je písmeno, které je před stavovým znakem
                while (word[j] == line[i])
                {
                    i++;
                    tri++;
                
                }
        
                if (tri > 0)
                {
                    index++;
                    j += 2;
                }
            }
            //printf("%c == %c\n",word[j], line[i]);

            
            if (word[j] == line[i] && jedenznak == 0)
            {
                index++;
            }
            
            stav = 0;
        }

/*
        printf("%d - konečný\n",index);
        printf("%c == %c - po stavu 2\n",word[j], line[i]);
*/

        //-------------------------------------------------
        // doplňková kontrola stringu pokud je stav == 3 "+"
        
        
        
        if (stav == 3)
        {
            
            //printf("%c == %c\n",word[j], line[i]);

            // kotrola, jestli je tam předchozí znak
            if ( word[j] != line[i])
            {
                chyba3++;
            }

            //prochází stringem Line dokud je v řádku za sebou je písmeno, které je před stavovým znakem
            while ( word[j] == line[i])
            {
                i++;
            }
            
            //printf("%c == %c - před\n",word[j], line[i]);
         
            // posun před regulární znak 
            j += 2;

            //printf("%c == %c - za\n",word[j], line[i]);


            // přičtení schodných znaků, které jsme se přeskočili
            index += 2;
            stav = 0;
        }
        

        //-------------------------------------------


        //printf("%d %d - j + chyba3\n", j, chyba3);

        if (word[j] != line[i] || chyba3 != 0 )
        {
            //printf("%c == %c\n",word[j], line[i]);
            j = 0;
            index = 0;            
        }

        
        //printf("%d\n", index);                
        if (word[j] == line[i])
        {
            //printf("%d - word %d - string\n", j, i);
            //printf("%d index\n", index);
            //printf("%c - string %c - patternword\n", line[i], word[j]);
            j++;
            index++;
        }

        //----------------------------------


/*
        printf("%d - pozice v paternu\n", j);
        printf("%d - počet shodných písmen\n", index);
        printf("%d - stav\n", stav);
*/


        // pokud- li to nepřejde do žádného stavu, tak to kontroluje jako normálně 
        if(stav == 0)
        {
            //
            if(index == patternword)
            {
                //printf("%d - index každého kroku konec %d\n", index, i);
                //printf("%c - string %c - patternword\n", line[i], word[j]);
                //printf("ANO\n");
                return true;
            }               
        }

        //printf("%d - index každého kroku před %d\n", index, i);
    }
    //printf("%d\n", j);
    
    //printf("NE\n");
    return false;
    
}

_Bool matchPatter(char *line, char *word, int patternword)
{
  
    
    
    int j = 0;
    int index = 0;
    for( int i = 0; line[i] != '\0'; i++)
    {


        if (word[j] != line[i])
        {
            j = 0;
            index = 0;            
        }
 
            
        
        if (word[j] == line[i])
        {
            
            j++;
            index++;
            
        }

        if(index == patternword)
        {
            return true;
        }
    }
    return false;
}
