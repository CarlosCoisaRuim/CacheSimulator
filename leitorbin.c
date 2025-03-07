#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

    void leitor(char *arquivoEntrada);



    int main(int argc, char *argv[]) {
        if (argc != 7) {
            printf("Numero de argumentos incorreto. Utilize:\n");
            printf("./cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada\n");
            exit(EXIT_FAILURE);
        }
    
        int nsets = atoi(argv[1]);
        int bsize = atoi(argv[2]);
        int assoc = atoi(argv[3]);
        char *subst = argv[4];
        int flagOut = atoi(argv[5]);
        char *arquivoEntrada = argv[6];
    
        printf("nsets = %d\n", nsets);
        printf("bsize = %d\n", bsize);
        printf("assoc = %d\n", assoc);
        printf("subst = %s\n", subst);
        printf("flagOut = %d\n", flagOut);
        printf("arquivo = %s\n", arquivoEntrada);
    
        srand(time(NULL));
    
        
        leitor(arquivoEntrada);
        
    
    }

    void leitor(char *arquivoEntrada) {
        FILE *file = fopen(arquivoEntrada, "rb");
        if (!file) {
            perror("Erro ao abrir arquivo");
            exit(EXIT_FAILURE);
        }
    
        uint32_t address;
        while (fread(&address, sizeof(uint32_t), 1, file) == 1) {
            address = __builtin_bswap32(address); // Conversor de ordem dos bits
    
            printf("%d\n", address);
        }
    
        fclose(file);
    }