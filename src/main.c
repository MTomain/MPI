#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/generator.h"

int** alloc_matrix(int lines, int cols) {
    int **matrix = (int**)malloc(lines * sizeof(int*));
    if (matrix == NULL) return NULL;
    
    int *dados = (int*)malloc(lines * cols * sizeof(int));
    if (dados == NULL) {
        free(matrix);
        return NULL;
    }
    
    for (int i = 0; i < lines; i++) {
        matrix[i] = &(dados[i * cols]);
    }
    return matrix;
}

int **fill_matrix(int **matrix, int N, int M){
    int i = 0;
    int j = 0;
    char buffer[1024]; // Buffer para a linha inteira
    char *token;       // Ponteiro para o "pedaço" (número)
    FILE *fp;

    fp = fopen(name, "r");
    if (fp == NULL) {
        perror("Error opening file");
    }


    while (i < N && fgets(buffer, sizeof(buffer), fp)) {
        if (buffer[0] == '\n') {
            continue;
        }
        j = 0; 
        token = strtok(buffer, " \t\n");
        while (token != NULL && j < M) {
            matrix[i][j] = atoi(token);
            j++;
            token = strtok(NULL, " \t\n");
        }
        i++;
    }

    fclose(fp);

    return matrix;
}

void print_matrix(int **matrix, int N, int M){
    for (int i =0; i<N; i++){
        for (int j=0;j<M;j++){
            printf("%i ",matrix[i][j]);
        }
        printf("\n");
    }
}

void free_matrix(int **matrix) {
    if (matrix) {
        if (matrix[0]) {
            free(matrix[0]); 
        }
        free(matrix);
    }
}

void iteration(int **matrix, int i, int j){
    // verificar a contaminação a cada iteração da matrix


    if(matrix[i][j] == -2){
        //se a pessoa já estava morta da iteração anterior, deverá sumir
        matrix[i][j] = 0;

    }else if (matrix[i][j] == -1){
        // se a pessoa estiver ontaminada, há chance de cura;
        srand(time(0));
        int prob = rand() % 9999;

        if (prob >=0 && prob<=999){
            matrix[i][j] = 1 // curada
        }else if(prob>=4000){
            matrix[i][j]=-2 // morreu
        }else{
            continue; //continua contaminada
        }
    } else if (matrix[i][j] == 1){
        // se a pessoa está saudavel, pode ser contaminada
        
    }

}

int main(){
    int N, M;
    printf("N x M dimensions:");
    scanf("%i %i",&N, &M);

    int **population_matrix = alloc_matrix(N, M); // gera matriz
    generate(N,M); // gera a população incial

    fill_matrix(population_matrix, N,M);

    // loop de iterações NxM
/*     for (int i =0, i<N; i++){
        for (int j=0;j<M;j++){
            round();
        }
    } */

    return 0;
}