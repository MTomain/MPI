/* #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/generator.h"

int N_global, M_global; 
int cont_iteracoes = 0; 
int morreu = 0;

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

//para uso com o generator
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

void free_matrix(int **matrix) {
    if (matrix) {
        if (matrix[0]) {
            free(matrix[0]); 
        }
        free(matrix);
    }
}

//leitura de arquivo externo (testar com os de CUDA)
int **read_input_file(const char *filename, int *N_out, int *M_out) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Não foi possível abrir '%s'.\n", filename);
        return NULL;
    }

    if (fscanf(fp, "%d %d", N_out, M_out) != 2) {
        fprintf(stderr, "Quantidade matricial incorreta (NxM)\n");
        fclose(fp);
        return NULL;
    }
    
    int N = *N_out;
    int M = *M_out;
    
    int **matrix = alloc_matrix(N, M);
    if (matrix == NULL) {
        fprintf(stderr, "Falha na alocacao da matriz.\n");
        fclose(fp);
        return NULL;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            //mais de um valor junto
            if (fscanf(fp, "%d", &matrix[i][j]) != 1) {
                fprintf(stderr, "Falha ao ler [%d][%d] da matriz.\n", i, j);
                free_matrix(matrix);
                fclose(fp);
                return NULL;
            }
        }
    }

    fclose(fp);
    return matrix;
}

//escreve resultados em saida.txt
void write_output(const char *filename, int total_mortos, int total_sobreviventes, int iteracoes_finais, int **matrix, int N, int M) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Não foi possivel abrir o arquivo de saida '%s'\n", filename);
    }
    
    fprintf(fp, "Iteracoes_finais: %d\n", iteracoes_finais);
    fprintf(fp, "Mortos: %d\n", total_mortos);
    fprintf(fp, "Sobreviventes: %d\n", total_sobreviventes);

    fclose(fp);
}

void print_matrix(int **matrix, int N, int M){
    for (int i =0; i<N; i++){
        for (int j=0;j<M;j++){
            //3i é para tabelamento
            printf("%3i ",matrix[i][j]);
        }
        printf("\n");
    }
}

int iteration(int **matrix, int i, int j, int N, int M) {
    int atual = matrix[i][j];
    
    if (atual == 0) {
        return 0; // Vazio permanece vazio.
    }
    
    if (atual == -2) {
        // Morto na iteração anterior some (0) agora.
        morreu++;
        return 0;
    }
    
    if (atual == -1) {
        // Contaminado - chance de cura (0.1), continuar (0.3), ou morrer (0.6).
        int prob = rand() % 1000; 
        
        if (prob >= 0 && prob <= 100) { // 10%
            return 1; // Curado 
        } else if (prob < 400) { // 30% (de 100 a 399)
            return -1; //continua podi
        } else { // 60% (de 400 a 999)
            return -2; // Morreu 
        }
    }
    
    if (atual == 1) {
        // Saudável vai ser contaminado?
        // Cima (i-1)
        if (i > 0 && (matrix[i-1][j] == -1 || matrix[i-1][j] == -2)) {
            return -1;
        }
        // Baixo (i+1)
        if (i < N-1 && (matrix[i+1][j] == -1 || matrix[i+1][j] == -2)) {
            return -1; 
        }
        // Esquerda (j-1)
        if (j > 0 && (matrix[i][j-1] == -1 || matrix[i][j-1] == -2)) {
            return -1; // ih ala otário tomou de trás
        }
        // Direita (j+1)
        if (j < M-1 && (matrix[i][j+1] == -1 || matrix[i][j+1] == -2)) {
            return -1; 
        }
        // Se não foi contaminado, permanece saudável.
        return 1;
    }
    return atual; 
}

//Checa a condição de parada: população zero ou estável (todos salvos).
//Retorna 1 para continuar, 0 para parar.
int check_stop_condition(int **matrix, int N, int M) {
    int count_infected = 0;
    int count_healthy = 0;
    int count_dead_pending = 0; //quem ta pra sumir (-2)
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            if (matrix[i][j] == -1) {
                count_infected++;
            } else if (matrix[i][j] == 1) {
                count_healthy++;
            } else if (matrix[i][j] == -2) {
                count_dead_pending++;
            }
        }
    }

    int total_populacao = count_infected + count_healthy + count_dead_pending;
    
    //Todos salvos (1 ou 0)
    if (count_infected == 0 && count_dead_pending == 0 && count_healthy > 0) {
        printf("\n SIMULACAO ENCERRADA: Todos foram salvos! (Iteracoes: %d)\n", cont_iteracoes);
        return 0; 
    }
    
    //Geral morreu (0)
    if (total_populacao == 0) {
        printf("\n SIMULACAO ENCERRADA: Populacao acabou! (Mortos). (Iteracoes: %d)\n", cont_iteracoes);
        return 0; 
    }

    //Limite de iterações atingido
    if (cont_iteracoes >= N * M) {
         printf("\n SIMULACAO ENCERRADA: Limite de %d iteracoes atingido.\n", N * M);
         return 0;
    }
    
    return 1; //continua
}

//executa tudo
void run_simulation(int **initial_matrix, int N, int M, const char *output_filename) {
    int **current_matrix = initial_matrix;
    int **next_matrix = alloc_matrix(N, M);
    if (next_matrix == NULL) {
        fprintf(stderr, "Falha na alocacao da matriz secundaria.\n");
        return;
    }
    
    cont_iteracoes = 0;
    morreu = 0;

    //printf("\n--- Matriz Inicial (Iteracao 0) ---\n");
    //print_matrix(current_matrix, N, M);
    //printf("-----------------------------------\n");

    while (check_stop_condition(current_matrix, N, M)) {
        cont_iteracoes++;
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                next_matrix[i][j] = iteration(current_matrix, i, j, N, M);
            }
        }

        int **temp = current_matrix;
        current_matrix = next_matrix;
        next_matrix = temp;
        
        //printf("\n--- Matriz Apos Iteracao %d ---\n", cont_iteracoes);
        //print_matrix(current_matrix, N, M);
        //printf("-----------------------------------\n");
    }

    int total_sobreviventes = 0; // -1 e 1 achei esquisito isso pq no arquivo ele fala q só para qnd ou geral morre ou geral se cura mas dps ele fala q pode os dois tmb

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            if (current_matrix[i][j] == 1 || current_matrix[i][j] == -1) {
                total_sobreviventes++;
            }
        }
    }
    
    write_output(output_filename, morreu, total_sobreviventes, cont_iteracoes, current_matrix, N, M);

    free_matrix(next_matrix); 
}


int main(int argc, char *argv[]){
    if (argc != 3) {
        fprintf(stderr, "%s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        return 1;
    }
    srand(time(NULL)); 
    
    int N, M;    
    int **population_matrix = read_input_file(argv[1], &N, &M);
    if (population_matrix == NULL) {
        return 1;
    }

    clock_t start = clock();
    run_simulation(population_matrix, N, M, argv[2]);
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Tempo de execução Sequencial: %.4f segundos\n", time_spent);
    free_matrix(population_matrix); 

    return 0;
} */ 