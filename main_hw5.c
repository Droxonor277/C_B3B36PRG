#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Ńa vstup vydá matici po řádcích a její rozměry řádek, sloupec
void printMatrix(int** pole, int rows, int cols);

// Načte rozměry matice
int load_Dimenzion_Matrix(int* rows, int* cols);

// Načte matici
int** loadMatrix(int rows, int cols, int *number_of_fails);


// Provede s dvěmi maticemi operaci sčítání
void scitaniMatrix(int** pole1, int** pole2, int rows1, int cols1);

// Provede s dvěmi maticemi operaci odčítání
void odcitaniMatrix(int** pole1, int** pole2, int rows1, int cols1);


// Provede s dvěmi maticemi operaci násobení
void nasobeniMatrix(int** pole1, int** pole2, int rows1, int cols1, int rows2, int cols2, int** pole4);

// převod pomocné matice 4 do matice 1
void prevodMatrix(int** pole1, int** pole4, int cols1, int rows2);

// vytvoření pomoné matice podle zadaných paramaterů
int** makeMatrix(int rows, int cols); 

int main()
{


    int rows1 = 0; // počet řádků první matice
    int cols1 = 0; // počet sloupců první matice
    int rows2 = 0; // počet řádků druhé matice
    int cols2 = 0; // počet sloupců druhé matice
    int rows3 = 0;
    int cols3 = 0;
    int rows4 = 0;

    char znaminko1; // znak matematické operace mezi matrix1 a matrix2
    char znaminko2; // znak matematické operace mezi matrix2 a matrix3
    int number_of_numbers1 = 0; // číslo na kontrolu načtený znaménka1
    int number_of_numbers2 = 0; // číslo na kontrolu načtených znamnka2
    int** pole1; // matice 1
    int** pole2; // matice 2
    int** pole3; // matice 3
    int** pole4; // matice 4
    int number_of_fails = 0; // počet chyb v programu
    int ret = 0; // konečná návratová hodnota
    int konec = 0; // podmínka na ukončení cyklu, když už nic nenačte
    int nasobeni = 0; // podmínka k uvodnění paměti v cyklech
    int cyklus = 0;
    int matrix3 = 0; // kontrola na uvolnění matrix3
    _Bool operace = true; // kotrola jestli proběhla operace
    int dimenzion2 = 0;

    while (konec != 1 )
    {
        operace = true;
        cyklus++;
        number_of_fails = 0;
        znaminko2 = 0;
        //printf("\n");
        //printf("%d - ČÍSLO CYKLU\n", cyklus);
/*    
        
        
        printf("%d - rows1, %d - cols1\n", rows1, cols1);
        printf("%d - rows2, %d - cols2\n", rows2, cols2);
        printf("%d - rows3, %d - cols3\n", rows3, cols3);
        */
/*
        if (number_of_numbers2 == 1 && znaminko2 != '*')
        {

            
            //printf("asdasd\n");
            for (int i = 0; i < rows2; i++)
            {
                free(pole2[i]);
            }
            free(pole2);
   
        }
*/
        /*
            if (cyklus > 1)
            {
              printf("%d - cyklus\n", cyklus);
              printf("\n");
              printf("rozbiá matice možná?\n");
              printMatrix(pole2, rows2, cols2);
              printf("\n");
            }
        */



        //podmínka na kontrolu jestli je ze začátku prázná a potom již ji nenačítat při dalším cyklu
        //podmínka na kontrolu jestli je ze začátku prázné znaménko a potom ho nenačítat znovu při dalším cyklu
        if (number_of_numbers1 == 0)
        {
            if (load_Dimenzion_Matrix(&rows1,&cols1))
            {
                ret = 100;
            }
            pole1 = loadMatrix(rows1,cols1,&number_of_fails);            
            number_of_numbers1 = scanf(" %c", &znaminko1);
            //printf("alokace matrix1\n");
            
            //printf("matrix1\n");
            //printMatrix(pole1,rows1,cols1);
            // kotrolola jestli jsou všechny znaky čísla 
            if(number_of_fails != 0)
            {
                ret = 100;      
            }
        }
/*
        if (cyklus > 1)
        {
            //printf("matrix1\n");
            //printMatrix(pole1,rows1,cols1);          
        }
*/        
        //podmínka na kontrolu jestli je ze začátku prázná a potom již ji nenačítat při dalším cyklu
        // KDYŽ PROBĚHNE NADŘEZENÁ MATEMATICKÁ OPERACE
        
        
        
        if (matrix3 == 0 && number_of_fails == 0)  //znaminko2 != '*')
        {
            //printf("%d - non2, %c - znaminko2, %d - cyklus\n", number_of_numbers2, znaminko2, cyklus);
            if (number_of_numbers2 == 1 && znaminko2 != '*')
            { 
                for (int i = 0; i < rows2 ; i++)
                {
                    free(pole2[i]);
                    //printf("uvolnění matrix2\n");
                }

                free(pole2);
                //rows2 = 0;
                //cols2 = 0;)
                dimenzion2 = 0;
            }
            /*
            if (cyklus > 1)
            {
              printf("\n");
              printf("%d - cyklus\n", cyklus);
              printf("matrix2\n");
              //printMatrix(pole2, rows2, cols2);
              printf("%d - rows1, %d - cols1, %c - znaminko1\n", rows1, cols1, znaminko1);
              printf("%d - rows2, %d - cols2\n", rows2, cols2);
              printf("\n");
            } 
            */        
            // načtení matrix po matrix1 s jeho rozměry
            if (load_Dimenzion_Matrix(&rows2,&cols2))
            {
                ret = 100;
            }
       





            pole2 = loadMatrix(rows2,cols2,&number_of_fails);
            //printf("alokace matrix2\n");
            //printf("matrix2\n");
            //printMatrix(pole2,rows2,cols2);
            dimenzion2++;
            // kotrolola jestli jsou všechny znaky čísla 
            if(number_of_fails != 0)
            {
                ret = 100;      
            }            
        }        

        //vynulování na kontrolu dalších načtených čísel v dalších cyklech
        number_of_numbers2 = 0;

        matrix3 = 0;
  
        number_of_numbers2 =  scanf(" %c", &znaminko2);





        // jestliže se načte znaménko *, tak se program pokusí načít program matice 3
        if(znaminko2 == '*'&& znaminko1 != '*' && operace == true && ret == 0)
        {

            load_Dimenzion_Matrix(&rows3,&cols3);

            pole3 = loadMatrix(rows3,cols3,&number_of_fails);




 
            if (cols2 != rows3)
            {

                ret = 100;
            }


            if (ret == 0)
            {   


                if(nasobeni != 0)
                {
                    
                    for (int i = 0; i < rows4; i++)
                    {
                        free(pole4[i]);
                    }
                
                    
                    free(pole4);
                    nasobeni = 0;
                }



                pole4 = makeMatrix(rows2,cols3);


                nasobeniMatrix( pole2, pole3, rows2, cols2, rows3, cols3, pole4);
                for (int i = 0; i < rows2; i++)
                {
                    free(pole2[i]);
                }

                free(pole2);


                pole2 = makeMatrix(rows2,cols3);

                prevodMatrix(pole2, pole4, cols3, rows2);

                nasobeni++;
                matrix3++;
                cols2 = cols3;
                rows4 = rows2;
               
            }


            // vrácení paměti počítači 
            for (int i = 0; i < rows3; i++)
            {
                free(pole3[i]);
            }
            free(pole3);


            operace = false;
        }
        //printMatrix(pole3, rows3, cols3);

 




        
        // provedení operace na základě znaménka +
        if ( znaminko1 == '+' && matrix3 == 0 && operace == true)
        {
            //printf("sčítání bez přednosti\n");
            // kontrola jestli se shodují rozměry matic pro provedení operace +
            if (rows1 != rows2 || cols1 != cols2)
            {
                //fprintf(stderr, "ERROR: Chybní vstup!\n");
                
                ret = 100;
            }

            if (ret == 0)
            {
            scitaniMatrix( pole1, pole2, rows1, cols1);
            }
            operace = false;

        }


        // provedení operace na základě znaménka -
        if ( znaminko1 == '-' && matrix3 == 0 && operace == true)
        {
   
            // kontrola jestli se shodují rozměry matic pro provedení operace -
            if (rows1 != rows2 || cols1 != cols2)
            {
   
                ret = 100;
            }
            if (ret == 0)
            {
                odcitaniMatrix( pole1, pole2, rows1, cols1);
            }
            operace = false;

        }






        // provedení operace na základě znaménka *
        if ( znaminko1 == '*' && matrix3 == 0 && operace == true)
        {

            // kontrola jestli se shodují rozměry matic pro provedení operace *
            if (cols1 != rows2)
            {

                ret = 100;
            }
            
            // ověřit jestli stejný row1 == cols2
            if (ret == 0)
            {   
                
                if(nasobeni != 0)
                {
                    for (int i = 0; i < rows4; i++)
                    {
                        //printf("uvolnění matrix4\n");
                        free(pole4[i]);
                    }
                
                    free(pole4);
                    nasobeni = 0;
                }

                
                // vytvoření domplňkové matice
                pole4 = makeMatrix(rows1,cols2);
                //printf("alokace matrix4\n");

                nasobeniMatrix( pole1, pole2, rows1, cols1, rows2, cols2, pole4);


                nasobeni++;
                for (int i = 0; i < rows1; i++)
                {
                    //printf("uvolnění matrix1\n");
                    free(pole1[i]);
                }


                free(pole1);
                
                
                pole1 = makeMatrix(rows1,cols2);
                //printf("alokace matrix1\n");
                
                prevodMatrix(pole1, pole4, cols2, rows1);

                cols1 = cols2;
                rows4 = rows1;


            }
            operace = false;
        }


        // Jestliže proběhne operace sčítání a odčítání, tak se uvolní matrix2 a zároveň znaménko2 se stane znamenkém1
        if ((znaminko1 == '+' || znaminko1 == '-' || znaminko1 == '*') && matrix3 == 0) // too potom upravit '*'
        {
            znaminko1 = znaminko2;
        }

        // Jestliže se nenačte už znaménko, tak se ukončí cyklus a začne se s výpisem
        if(number_of_numbers1 == -1|| number_of_numbers2 == -1 ||ret != 0)
        {
            konec++;
        }
        /*
        printf("matrix1\n");
        printMatrix(pole1, rows1, cols1);
        printf("matrix2\n");
        printMatrix(pole2, rows2, cols2);
        */
        //printf("%d - znaminko1, %d - znaminko2\n", znaminko1, znaminko2);

    }
    // konec cyklů --------------------------------------------------------------------------------------------------------------------


    
    if (ret == 0)
    {
        // Výpis matice1, znamenika1 a matice2
        printMatrix(pole1,rows1,cols1);
    }


    // vrácení paměti počítači matrix1 a matrix2
    for (int i = 0; i < rows1; i++)
    {
      free(pole1[i]);
      //printf("uvolnění matrix1\n");
    }
    free(pole1);

    if (dimenzion2 != 0)
    {
      for (int i = 0; i < rows2; i++)
      {
        //printf("uvolnění matrix2\n");
        free(pole2[i]);
      }

      
      free(pole2);
    }




    //printf("%d - nasobeni\n", nasobeni);
    // uvolnění paměti pomocné matice
    if (nasobeni != 0)
    {
        for (int i = 0; i < rows4; i++)
        {
        free(pole4[i]);
        }
        //printf("uvolnění matrix4\n");
        free(pole4);
    }

    if (ret != 0)
    {
        fprintf(stderr,"Error: Chybny vstup!");
        fprintf(stderr,"\n");  
    }
    return ret;
}
















int load_Dimenzion_Matrix(int* r, int* c)
{

    int z = 0;
    z = scanf("%4d %4d", r, c);
    if (z != 2)
    {
      return 1;
    } else
    {
      return 0;
    }
} 
/*
    if (z != 2)
    {
      return 1;
    } else
    {
      return 0;
    }
    */

    /*
    int z = 0;
    int k = 0;
    while((z = getc(stdin)) && z != '\n' && z != EOF)
    {
        
        if( k == 0)
        {
          *r = z;
          k++;
        } else if( *r != z)
        {
          *c = z;
          k++;
        }       
    }
    if (k == 3)
    {
      return 0;
    }
    return 1;
*/    


int** loadMatrix(int row, int cols, int* number_of_fails)
{
    int** pole;
    int fails;
    //scanf("%d %d", row, cols);
    pole = (int**)malloc(row * sizeof(int*));
    for (int i = 0; i < row; i++)
	  	pole[i] = (int*)malloc(cols * sizeof(int));

    for (int r = 0; r < row; r++ )
    {
        for (int c = 0; c < cols; c++)
        {
            fails = 0;
            fails = scanf("%4d", &pole[r][c]);
            if(fails != 1)
            {
            *number_of_fails += 1;
            }
        }
    }

    return pole;
}


void printMatrix(int** pole1, int row1, int cols1)
{
    printf("%-d %-d\n", row1, cols1);
    for (int r = 0; r < row1; r++ )
    {
        for (int c = 0; c < cols1; c++)
        {
            if ( (c + 1) != cols1)
            {
                printf("%-d ", pole1[r][c]);
            } else{
                printf("%-d",pole1[r][c] ); 
            }
        }
        printf("\n");
    }
}



void scitaniMatrix(int** pole1, int** pole2, int row1, int cols1)
{
    //printf("%-d ""%-d", row1, cols1);
    //printf("\n");  
    for (int r = 0; r < row1; r++ )
    {
        for (int c = 0; c < cols1; c++)
        {

            pole1[r][c] = pole1[r][c] + pole2[r][c];
/*
            if ((c + 1) != cols1)
            {
                printf("%-d ", pole1[r][c] + pole2[r][c]);     
            } else{
                printf("%-d",pole1[r][c] + pole2[r][c]);
            }
*/
        }
        //printf("\n");
    }
}



void odcitaniMatrix(int** pole1, int** pole2, int row1, int cols1)
{
    //printf("%-d ""%-d", row1, cols1);
    //printf("\n");   
    for (int r = 0; r < row1; r++ )
    {
        for (int c = 0; c < cols1; c++)
        {
            
            pole1[r][c] = pole1[r][c] - pole2[r][c];
/*            
            if ((c + 1) != cols1)
            {
                printf("%-d ", pole1[r][c] - pole2[r][c]);     
            } else{
                printf("%-d",pole1[r][c] - pole2[r][c]);
            }     
*/
        }
        //printf("\n");
    }


}

void nasobeniMatrix(int** pole1, int** pole2, int row1, int cols1, int row2, int cols2,  int** pole4)
{
    int soucet;
    //printf("%-d " "%-d", row1, cols2);
    //printf("\n");  
    for (int r = 0; r < row1; r++ )
    {
        for (int c = 0; c < cols2; c++)
        {
            soucet = 0;
            for (int sum = 0; sum < cols1; sum++)
            {
                soucet += pole1[r][sum] * pole2[sum][c];
                //printf("%d - pole1[r][sum] %d - pole2[sum][c]\n",pole1[r][sum], pole2[sum][c]);
                //printf("%d\n", soucet);  
            }

            //printf("dalsi\n");
            pole4[r][c] = soucet;
            /*
            if ((c + 1) != cols2)
            {
            printf("%-d ", soucet);     
            } else {
              printf("%-d", soucet);
            }
            */
        }
        //printf("\n");
    }
}

void prevodMatrix(int** pole1, int** pole4, int cols1, int rows2)
{
    for (int r = 0; r < rows2; r++ )
    {
        for (int c = 0; c < cols1; c++)
        {    

            pole1[r][c] = pole4[r][c];

        }
    }
}


int** makeMatrix(int rows, int cols)
{

    int** pole;
    //printf("%d - rows, %d - cols\n", rows, cols);
    pole = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++)
	  	pole[i] = (int*)malloc(cols * sizeof(int));
    //printMatrix(pole,rows,cols);
    for (int r = 0; r < rows; r++ )
    {
        for (int c = 0; c < cols; c++)
        {
            pole[r][c] = 0;
        }
    }
    
    return pole;    
}
