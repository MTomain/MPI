#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../include/generator.h"

int cont_iteracoes = 0; 
int total_mortos_globais = 0;
int N_global, M_global; 

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
void write_output(const char *filename, int total_mortos, int total_sobreviventes) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, " Não foi possivel abrir o arquivo de saída '%s'\n", filename);
        return;
    }
    
    fprintf(fp, "Mortos: %d\n", total_mortos);
    fprintf(fp, "Sobreviventes: %d\n", total_sobreviventes);

    fclose(fp);
}

int iteration(int **matrix, int i, int j, int N_local_total, int M_global) {
    int atual = matrix[i][j];
    
    if (atual == 0) return 0;
    
    if (atual == -2) {
        return -3; 
    }

    if(atual == -3){
        return 0;
    }
    
    if (atual == -1) {
        int prob = rand() % 10000; 
        if (prob >= 0 && prob < 1000) return 1; 
        if (prob >=1000 && prob < 4000) return -1;
        return -2; 
    }
    
    if (atual == 1) {
        // Cima (i-1)
        if (i > 0 && (matrix[i-1][j] == -1 || matrix[i-1][j] == -2 || matrix[i-1][j] == -3)) return -1;
        // Baixo (i+1)
        if (i < N_local_total - 1 && (matrix[i+1][j] == -1 || matrix[i+1][j] == -2 || matrix[i+1][j] == -3)) return -1;
        // Esquerda (j-1)
        if (j > 0 && (matrix[i][j-1] == -1 || matrix[i][j-1] == -2 || matrix[i][j-1] == -3)) return -1;
        // Direita (j+1)
        if (j < M_global - 1 && (matrix[i][j+1] == -1 || matrix[i][j+1] == -2 || matrix[i][j+1] == -3)) return -1;
        
        return 1;
    }
    return atual; 
}

int check_stop_condition(int N, int M, int total_active_pop) {
    if (cont_iteracoes >= N*M) {
        return 0;
    }
    if (total_active_pop == 0) {
        return 0; 
    }
    return 1;
}

//calcular distribuição desigual 
void calculate_displacements(int N, int M, int size, int **counts_out, int **displs_out) {
    int *counts = (int*)malloc(size * sizeof(int));
    int *displs = (int*)malloc(size * sizeof(int));
    int remainder = N % size;
    int sum = 0;
    
    for (int i = 0; i < size; i++) {
        int rows_per_proc = N / size;
        if (i < remainder) {
            rows_per_proc++;
        }
        
        counts[i] = rows_per_proc * M; 
        displs[i] = sum;
        sum += counts[i];
    }
    *counts_out = counts;
    *displs_out = displs;
}

int **run_mpi_simulation(char *input_file, char *output_file) {
    int rank, size;
    double start_time, end_time;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status status;
    
    int **global_matrix_ptr = NULL; 
    int *global_matrix_data = NULL; 
    
    int *sendcounts = NULL; 
    int *displs = NULL;     
    
    //Leitura e Broadcast de N e M 
    if (rank == 0) {
        global_matrix_ptr = read_input_file(input_file, &N_global, &M_global);
        if (global_matrix_ptr == NULL) MPI_Abort(MPI_COMM_WORLD, 1);
        global_matrix_data = global_matrix_ptr[0];
        calculate_displacements(N_global, M_global, size, &sendcounts, &displs);
    }
    
    // Todos precisam saber as dimensões
    MPI_Bcast(&N_global, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    MPI_Bcast(&M_global, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Recebe o count de elementos que este rank receberá
    int local_count_total = 0;
    // O Rank 0 envia os sendcounts. Os demais ranks recebem seu count.
    MPI_Scatter(sendcounts, 1, MPI_INT, &local_count_total, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    
    int N_local_rows = local_count_total / M_global; 
    
    //Alocação Local (Incluindo Ghost Cells)
    int has_up_ghost = (rank > 0);
    int has_down_ghost = (rank < size - 1);
    int N_local_total = N_local_rows + has_up_ghost + has_down_ghost; 
    
    int **current_local_matrix = alloc_matrix(N_local_total, M_global);
    int **next_local_matrix = alloc_matrix(N_local_total, M_global);
    if (current_local_matrix == NULL || next_local_matrix == NULL) MPI_Abort(MPI_COMM_WORLD, 1);

    //Distribuição dos Dados Iniciais (Scatterv)
    int local_recv_start_index = has_up_ghost * M_global; 

    MPI_Scatterv(global_matrix_data, sendcounts, displs, MPI_INT, 
                 &current_local_matrix[0][0] + local_recv_start_index, local_count_total, MPI_INT, 
                 0, MPI_COMM_WORLD);

    int global_total_active_pop = 1; 
    int mortes_da_iteracao_soma = 0; // Variavel temporaria no Rank 0
    
    start_time = MPI_Wtime();
    
    while (check_stop_condition(N_global, M_global, global_total_active_pop)) {
        cont_iteracoes++;
        
        //Comunicação de Fronteira (Troca de Ghost Cells)
        int up_neighbor = (rank == 0) ? MPI_PROC_NULL : rank - 1;
        int down_neighbor = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;
        
        #define TAG_UP 1 
        #define TAG_DOWN 2

        // Troca com o vizinho superior (envia 1ª real para cima, recebe ghost 0)
        MPI_Sendrecv(&current_local_matrix[has_up_ghost][0], M_global, MPI_INT, up_neighbor, TAG_UP,
                     &current_local_matrix[0][0], M_global, MPI_INT, up_neighbor, TAG_DOWN, 
                     MPI_COMM_WORLD, &status);
        
        // Troca com o vizinho inferior (envia última real, recebe ghost final)
        MPI_Sendrecv(&current_local_matrix[N_local_total - has_down_ghost - 1][0], M_global, MPI_INT, down_neighbor, TAG_DOWN,
                     &current_local_matrix[N_local_total - 1][0], M_global, MPI_INT, down_neighbor, TAG_UP,
                     MPI_COMM_WORLD, &status);
        
        //Cálculo Local e Contagem
        int local_active_pop = 0;
        int local_mortes = 0; // Reiniciada a 0 a cada iteração 
        
        for (int i = has_up_ghost; i < N_local_total - has_down_ghost; i++) {
            for (int j = 0; j < M_global; j++) {
                int next_state = iteration(current_local_matrix, i, j, N_local_total, M_global);
                next_local_matrix[i][j] = next_state;

                // Contagem de mortes: se a célula era -2 na matriz antiga e virou 0 na nova
                if (current_local_matrix[i][j] == -3 && next_state == 0) {
                    local_mortes++; // Incrementa a contagem LOCAL
                }
                
                if (next_state != 0) {
                    local_active_pop++; // Contagem de 1, -1, -2
                }
            }
        }

        //Troca de Matrizes Locais (Swap)
        int **temp = current_local_matrix;
        current_local_matrix = next_local_matrix;
        next_local_matrix = temp;

        // Sincronização (Allreduce e Reduce)
        
        // População Ativa (check_stop_condition)
        MPI_Allreduce(&local_active_pop, &global_total_active_pop, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        
        // MPI_Reduce: Soma as mortes locais. O Rank 0 recebe o valor somado.
        MPI_Reduce(&local_mortes, &mortes_da_iteracao_soma, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        
        // Rank 0 acumula o total global de mortes
        if (rank == 0) {
            total_mortos_globais += mortes_da_iteracao_soma;
        }
    }
    
    end_time = MPI_Wtime();
    
    int total_sobreviventes_finais = 0;
    
    // Soma os sobreviventes finais (1 e -1)
    int local_sobreviventes_finais = 0;
    for (int i = has_up_ghost; i < N_local_total - has_down_ghost; i++) {
        for (int j = 0; j < M_global; j++) {
            if (current_local_matrix[i][j] == 1 || current_local_matrix[i][j] == -1) {
                local_sobreviventes_finais++;
            }
        }
    }
    // MPI_Reduce: Soma a contagem final de sobreviventes
    MPI_Reduce(&local_sobreviventes_finais, &total_sobreviventes_finais, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        if (global_total_active_pop == 0) {
            printf(" A doenca exterminou a população. Total de %d mortes registradas.\n", total_mortos_globais);
        } else if (total_sobreviventes_finais == global_total_active_pop) {
             printf("Alguns foram salvos! %d pessoas sobreviveram.\n", total_sobreviventes_finais);
        } else {
             printf("Simulação encerrada. %d sobreviventes ativos.\n", total_sobreviventes_finais);
        }

        write_output(output_file, total_mortos_globais, total_sobreviventes_finais);

        double time_spent = end_time - start_time;
        printf("Tempo de execução MPI (%d procs): %.4f segundos\n", size, time_spent);

        free(sendcounts);
        free(displs);
        free_matrix(global_matrix_ptr); 
    } 
    
    free_matrix(current_local_matrix); 
    return next_local_matrix; 
}

int main(int argc, char *argv[]){
    //oh pra rodar com 8 slots precisa colocar a tag --oversubscribe
    // mpiexec -np 8 --oversubscribe bin/main_mpi realdata.dat saida_1024.txt
    //pelo menos na minha máquina teve, acho q pq de quantidade de cluster
    if (argc != 3) {
        fprintf(stderr, "Insira: mpirun -np <num_procs> %s \n", argv[0]);
        return 1;
    }
    
    MPI_Init(&argc, &argv); 
    
    int rank_dummy; 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_dummy); 
    
    srand(time(NULL) + rank_dummy); 
    
    int **auxiliary_matrix = NULL;

    auxiliary_matrix = run_mpi_simulation(argv[1], argv[2]);
    
    if (auxiliary_matrix != NULL) {
        free_matrix(auxiliary_matrix); 
    }
    
    MPI_Finalize();
    
    return 0;
}