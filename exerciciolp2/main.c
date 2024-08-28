#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_LETRAS 26
#define ASCII_A 65
#define TAMANHO_SENHA 4
#define NUM_ARQUIVOS 10

// Função de criptografia fornecida
char* encrypt(const char* str) {
    char* str_result = (char*) malloc(sizeof(char) * (TAMANHO_SENHA + 1));
    if (!str_result) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TAMANHO_SENHA; i++) {
        char c = str[i];
        char chave = str[i];

        if (c >= 'A' && c <= 'Z') {
            int str_idx = c - ASCII_A;
            int chave_idx = chave - ASCII_A;
            str_result[i] = ((str_idx + chave_idx) % NUM_LETRAS) + ASCII_A;
        } else {
            perror("Erro: String contém caracteres inválidos.");
            free(str_result);
            exit(EXIT_FAILURE);
        }
    }
    str_result[TAMANHO_SENHA] = '\0'; // Finaliza a string
    return str_result;
}

// Função para gerar todas as combinações possíveis de senhas e comparar
void quebrar_senhas(const char* senha_criptografada, const char* arquivo_saida) {
    char tentativa[TAMANHO_SENHA + 1] = "AAAA";
    FILE* file_out = fopen(arquivo_saida, "a"); // Abrir em modo append
    if (!file_out) {
        perror("Erro ao abrir arquivo de saída");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char* senha_gerada = encrypt(tentativa);
        if (strcmp(senha_gerada, senha_criptografada) == 0) {
            fprintf(file_out, "Senha quebrada: %s\n", tentativa);
            free(senha_gerada);
            break;
        }
        free(senha_gerada);

        // Incrementa a combinação de tentativa
        for (int i = TAMANHO_SENHA - 1; i >= 0; i--) {
            if (tentativa[i] == 'Z') {
                tentativa[i] = 'A';
            } else {
                tentativa[i]++;
                break;
            }
        }

        // Simulação de trabalho (remover ou ajustar conforme necessário)
        usleep(1); // Espera 10 milissegundos para simular carga de trabalho

        // Se voltamos para "AAAA", acabamos todas as combinações
        if (strcmp(tentativa, "AAAA") == 0) {
            break;
        }
    }

    fclose(file_out);
}

// Função que será executada por cada processo ou thread
void* processa_arquivo(void* args) {
    // Casting do argumento e processamento
    char** argv = (char**)args;
    const char* arquivo_entrada = argv[0];
    const char* arquivo_saida = argv[1];

    // Aqui a função de leitura do arquivo e quebra da senha seria implementada
    FILE* file_in = fopen(arquivo_entrada, "r");
    if (!file_in) {
        perror("Erro ao abrir arquivo de entrada");
        pthread_exit(NULL);
    }

    char senha_criptografada[TAMANHO_SENHA + 1];
    fscanf(file_in, "%s", senha_criptografada); // Lê a senha criptografada
    quebrar_senhas(senha_criptografada, arquivo_saida);

    fclose(file_in);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s -p|-t\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    clock_t inicio = clock();

    if (strcmp(argv[1], "-p") == 0) {
        // Modo processos
        printf("Gerando 10 processos para processar arquivos...\n");
        for (int i = 0; i < NUM_ARQUIVOS; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                // Processo filho
                char entrada[20], saida[20];
                sprintf(entrada, "passwd_%d.txt", i);
                sprintf(saida, "dec_passwd_%d.txt", i);
                printf("Processo PID %d: Quebrando senhas de %s\n", getpid(), entrada);
                processa_arquivo((void*[]){entrada, saida});
                printf("Processo PID %d: Senhas quebradas salvas em %s\n", getpid(), saida);
                exit(0);
            } else if (pid < 0) {
                perror("Erro ao criar processo");
                exit(EXIT_FAILURE);
            }
        }

        // Processo pai espera os filhos
        while (wait(NULL) > 0);
    } else if (strcmp(argv[1], "-t") == 0) {
        // Modo threads
        printf("Criando 10 threads para processar arquivos...\n");
        pthread_t threads[NUM_ARQUIVOS];
        for (int i = 0; i < NUM_ARQUIVOS; i++) {
            char entrada[20], saida[20];
            sprintf(entrada, "passwd_%d.txt", i);
            sprintf(saida, "dec_passwd_%d.txt", i);
            printf("Thread #%d: Quebrando senhas de %s\n", i, entrada);
            pthread_create(&threads[i], NULL, processa_arquivo, (void*[]){entrada, saida});
        }

        // Espera pelas threads
        for (int i = 0; i < NUM_ARQUIVOS; i++) {
            pthread_join(threads[i], NULL);
            printf("Thread #%d: Senhas quebradas salvas em dec_passwd_%d.txt\n", i, i);
        }
    } else {
        fprintf(stderr, "Opção inválida\n");
        exit(EXIT_FAILURE);
    }

    clock_t fim = clock();
    double tempo_execucao = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Tempo de execução: %.2f segundos\n", tempo_execucao);

    return 0;
}
