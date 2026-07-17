#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TYPE long long 
#define MAX_NUMBERS 1000000

int sieve[MAX_NUMBERS];

// vytvoření pole prvočísel
// vrátí pole zaplněné prvočísly
void create_sieve() {
    sieve[0] = sieve[1] = 1;
    for (int i = 2; i < MAX_NUMBERS; i++) {
        if (sieve[i] == 0) {
            for (int j = i + i; j < MAX_NUMBERS; j += i) {
                sieve[j] = 1;
            }
        }
    }
}

int next_prime(int from) {
    while (from < MAX_NUMBERS && sieve[from] == 1) {
        from++;
    }
    return from == MAX_NUMBERS ? -1 : from;
}

void print_primes(TYPE n) {
    int first_print = 1;
    int prime = 2;
    while (n >= prime && prime != -1) {
        int exponent = 0;
        while (n % prime == 0) {
            exponent++;
            n /= prime;
        }
        
        if (exponent > 0) {
            if (!first_print) {
                printf(" x ");
            }
            if (exponent == 1) {
                printf("%d", prime);
            }
            else if (exponent > 1) {
                printf("%d^%d", prime, exponent);
            }
            first_print = 0;
        }
        prime = next_prime(prime + 1);
        // printf("\n(%d %d)\n", n, prime);
    }
}

/* The main program */
int main(void)
{
    // TODO - insert your code here
    TYPE number = 0; // načtené číslo

    create_sieve();
    // for (int i = 0; i < 100; i++) {
    //     if (sieve[i] == 0) {
    //         printf("%d\n", i);
    //     }
    // }

    //int max_prime_numbers = MAX_NUMBERS;
    //int field_primenumber[max_prime_numbers];
    //printf("%zu\n", sizeof(TYPE));

    // while (scanf("%ld", &number) == 1)
    int scanf_status;
    while ((scanf_status = scanf("%lld", &number)) == 1)
    { // načítej dokud je na vstupu číslo a nebo se číslo nerovná nule
        if (number == 0)
        {
            return 0;
        }

        if (number < 0)
        {
            fprintf(stderr, "Error: Chybny vstup!\n");
            return 100;
        }

        printf("Prvociselny rozklad cisla %lld je:\n", number);

        if (number == 1)
        { // když číslo je jednička, tak ji vypiš
            printf("1\n");
        }
        else { // když číslo je cokoliv jiného, než jednička
            print_primes(number);
            printf("\n");
        }
    }
    if (scanf_status != EOF) {
        fprintf(stderr, "Error: Chybny vstup!\n");
        return 100;
    }

    return 0;
}
