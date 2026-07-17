#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define MAX_VALUE 69
#define MIN_VALUE 3









void report_error(int error);
_Bool ERROR_INPUT(int s, int v, int p);

/* The main program */
//int argc, char *argv[]
int main(void)
{
  int s = 0, v = 0, p = 0; // s- sirka, v- výska, p-plot
  int counter = 1; //počítadlo vnitřku
  int plot = 0;  //hradla)
  _Bool fence = false; // polovení nebo zamítnutí plotu


//Počítadla na generování  chyb
  int ret = 0; // číslo chyby
  // int c = 0; // kontrola jestli se načetl plot
  // int moc = 0;
  _Bool chyba = true; // zkouška jestli je ve vstupu chyba
  // int m = 0; // kontrola nenačtení vstupu


  // chyby 








// začátek kódu - budování    
  if (scanf("%d %d", &s , &v) == 2) { // pokud je vstup roven 2 načte vstup
    if (s == v){   // kontrola jesli je šířka a výška stejná
      if (scanf("%d", &p) == 1){ // pokud je načten ještě vstup roven jedné načtte ho
        fence = true; 
      

      } else { // pokud- li bude víc nebo míň vstupů nebo vstupní hodnoty neboudou čísla
        chyba = false;
        ret = 100;
      }
    }

  } else { // pokud- li bude víc nebo míň vstupů nebo vstupní hodnoty neboudou čísla
    chyba = false; 
    ret = 100;
  } 



  /*
  if (s >= MIN_VALUE && s <= MAX_VALUE && v >= MIN_VALUE && v <= MAX_VALUE && s < MIN_VALUE && s > MAX_VALUE && v < MIN_VALUE && v >MAX_VALUE ){ // kontrola, jestli jsou obě čísla číslicemi
    c += 1; // zjistit, že vstup musí být číslo

  }  
  if (c == 1){ // konečné vyhodnocení existence číslic na vstupu
    //fprintf(stderr,"Error: Chybny vstup!\n");
    chyba = false;
    ret = 100;

  }
  */
  if (s < MIN_VALUE || s > MAX_VALUE || v < MIN_VALUE || v > MAX_VALUE) { // kontrola rozsahu
    if (ret > 101 || ret == 0) {
    chyba = false;
    ret = 101;
    }
    

  } 
  if (s % 2 == 0) { // kontrola liché sirky
    if (ret > 102 || ret == 0){
    chyba = false;
    ret = 102;
    }
  

  } 
  if (fence){ // kontrola, jestli se vstup načetl
    if (v <= p || p < 1){ // kontrola jestli velikost plotu je ostře menší než výška
      chyba = false;
      if (ret > 103 || ret == 0){
      ret = 103;
      }
    }
  }




/*
  if (v > p && p >= 2){ // počídalo jesli je parametr velikosti plotu číslicí
    moc += 1;

  }
 
   
  if ( fence && chyba == true && s == v && ret != 103 ) { // konečné vyhodnocení jestli je parametr číslicí
      //fprintf(stderr,"Error: Chybny vstup!\n");
      chyba = false;
      ret = 100;
  }
*/

  if (ret != 0) { // kontrola, jestli se bude tisknou nějaký chybový výstup
    if (ret == 100) {
      fprintf(stderr,"Error: Chybny vstup!\n");
    }
    if (ret == 101) {
      fprintf(stderr,"Error: Vstup mimo interval!\n");
    }
    if (ret == 102) {
      fprintf(stderr,"Error: Sirka neni liche cislo!\n");  
    }
    if (ret == 103) {
      fprintf(stderr,"Error: Neplatna velikost plotu!\n"); 
    }
    
    
    
    
    return ret; // hlášení čísla chyby
  }
  


//chyby













  //printf("%d\n",ret);

  // začátek budování domu.........................
  if (!fence && chyba){

    for(int i = 0; i <= (s / 2) ; i++) {
    //printf("%d\n",(s / 2));
      for(int j = 1; j <= s; j++) {
        if (i + j == (s / 2 ) +1 || i == (s / 2) || i + j == ((s / 2 ) +1) + i*2) { // podmínky pro generování pravé střechy, podstavy střechy a levé části střechy
          printf("X");
        } else {
          if (i + j < ((s / 2 ) +1) + i*2) {
            printf(" ");
          }
        }


      }
      printf("\n");
    }

// střecha

    for(int i = 0; i <= v -3; i++) {
      for(int j = 1; j <= s; j++ ) {
        if ( j == 1 || j == s ) 
          printf("X");
        else
          printf(" ");
        
      }
      printf("\n");
    }
    for(int i = 1 ; i <= s; i++){
      printf("X");
    }
    printf("\n");
    return 0;
  }  
  // hrubá stavba










  if (fence && s == v && chyba){
    if (p % 2) // rozdělovač na správné generování plotu
      plot = 1;
    else
      plot = 0;


    for(int i = 0; i <= (s / 2) ; i++) {
      //printf("%d\n",(s / 2));
      for(int j = 1; j <= s; j++) {
        if (i + j == (s / 2 ) +1 || i == (s / 2) || i + j == ((s / 2 ) +1) + i*2) {
         printf("X");
        }else {
         if (i + j < ((s / 2 ) +1) + i*2) {
            printf(" ");
          }
        }


      }
      printf("\n");
      
    }
// střecha


    for(int i = 0; i <= v -3; i++) {
      for(int j = 1; j <= s + p; j++ ) {
        if ( j == 1 || j == s ) { //generování zdí
          printf("X");
        
        } else if (j > s && (v - 2) - (p - 1) == i  ) { //větší než šířka a začátek generování hradla 
          if (plot % 2) {
            printf("|");
            plot += 1; // počítadlo aby se střídal plot
          } else {
            printf("-");
            plot += 1;
          }
         
        } else if (p % 2 == 0 && j > s && j % 2 == 1 && (v - 2) - (p - 1) + 1 <= i  ) { // generování plotu 
            printf("|");
          
        } else if (p % 2 == 1 && j > s && j % 2 == 0 && (v - 2) - (p - 1) + 1 <= i  ){
            printf("|");
          
        } else if (j >= 2 && j <= s-1) { // generování vnitřku domu
          if (counter % 2){
            printf("o");
            counter += 1; // počítadlo na střídání vnitřku domu
          } else {
            printf("*");
            counter += 1;
          }
        } else { // generování mezery v plotu
          if ((v - 2) - (p - 1) + 1 <= i) {
            printf(" ");
          }
        }
          
      }
      printf("\n");
    }
    if (p % 2) // rozdělovač na správné generování plotu
      plot = 1;
    else
      plot = 0;


    for(int i = 1 ; i <= s + p; i++){ // generování podlahy domu  + plotu
      if (i <= s) // podlaha
        printf("X");
      else { //plot
        if (plot % 2) {
          printf("|");
          plot += 1; // počítadlo aby se střídal plot
        } else {
          printf("-");
          plot += 1;
        }
      }     

    }
    printf("\n");
// hrubá stavba

  return 0;
  }
}




