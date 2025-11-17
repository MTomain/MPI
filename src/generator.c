#include "../include/generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int generate(int N, int M){
    FILE *population;
    int person;

    srand(time(0));

    population = fopen(name, "w");

    if (population == NULL) {
        printf("Erro: Não foi possível criar o arquivo %s\n", name);
        return 1;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
           int prob = rand() % 100;
           
           if (prob < weight_dead) {
                person = dead;
            } 
            else if (prob < weight_sick) { 
                person = sick;
            } 
            else if (prob < weight_empty) { 
                person = empty;
            } 
            else { 
                person = healthy;
            }
           fprintf(population, "%i",person);
           
           if(j<M-1){
            fprintf(population," ");
           }
        }
        fprintf(population, "\n");
    }

    fclose(population);

    printf("Arquivo \"%s\" gerado com sucesso!\n", name);

    return 0;
}