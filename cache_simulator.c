#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint32_t tag; //identifica o bloco
    int valid; //indica se a linha contém um dado válido (1) ou não (0)
    int age; //utilizado para FIFO e LRU (maior, mais velho)
} CacheLine;

typedef struct {
    CacheLine *lines; //array de linhas dentro do conjunto
} CacheSet;

typedef struct {
    int nsets, bsize, assoc;
    char replacement;
    int accesses, hits, misses;
    int compulsory_misses, capacity_misses, conflict_misses;
    CacheSet *sets; //array de conjuntos dentro da cache
} Cache;

// Inicializa a cache
Cache *cache_init(int nsets, int bsize, int assoc, char replacement) {
    Cache *cache = (Cache *)malloc(sizeof(Cache)); //alloca a cache e as variáveis inseridas
    cache->nsets = nsets;
    cache->bsize = bsize;
    cache->assoc = assoc;
    cache->replacement = replacement;
    cache->accesses = cache->hits = cache->misses = 0;
    cache->compulsory_misses = cache->capacity_misses = cache->conflict_misses = 0;

    cache->sets = (CacheSet *)malloc(nsets * sizeof(CacheSet)); //alloca os conjuntos com base no nset
    for (int i = 0; i < nsets; i++) {
        cache->sets[i].lines = (CacheLine *)malloc(assoc * sizeof(CacheLine)); //alloca a quantidade de linhas dentro de cada conjunto com base na Associatividade
        for (int j = 0; j < assoc; j++) {
            cache->sets[i].lines[j].valid = 0; //inicializa cada linha como não contendo dado válido (vão estar vazias)
            cache->sets[i].lines[j].age = 0;
        }
    }
    return cache;
}

// Função de acesso à cache
void cache_access(Cache *cache, uint32_t address) {
    cache->accesses++; //cada acesso a um endereço na cache é registrado

    // Cálculo do índice e tag
    uint32_t index = (address / cache->bsize) % cache->nsets; //divide o endereço pelo tamanho do bloco para remover o offset, depois usa módulo (%) para extrair o índice do conjunto.
    uint32_t tag = (address / cache->bsize) / cache->nsets;  //remove o offset e divide pelo número de conjuntos para separar os bits superiores.

    CacheSet *set = &cache->sets[index]; //ponteiro para a posição [index] do array de CacheSets que está dentro da cache. Linha importante do código!

    // Verifica se o bloco está na cache (hit)
    for (int i = 0; i < cache->assoc; i++) { //percorre as linhas do conjunto com base na Associatividade
        if (set->lines[i].valid && set->lines[i].tag == tag) { //se encontrar uma linha válida com a mesma tag, é hit
            cache->hits++;      

            // Atualiza LRU
            if (cache->replacement == 'L') {
                set->lines[i].age = cache->accesses; //atualiza o age
            }
            return; //retorna imediatamente se for um hit
        }
    }

    // Miss
    cache->misses++; //se não retornar, vai cair aqui, contabilizando o miss

    // Verifica se há espaço vazio (miss compulsório)
    int empty_index = -1;
    for (int i = 0; i < cache->assoc; i++) {
        if (!set->lines[i].valid) { //Verifica se ./há alguma linha vazia para alocar o novo bloco.
            empty_index = i;
            break;
        }
    }

    // Classifica o tipo de miss
    if (empty_index != -1) {
        cache->compulsory_misses++; //Miss compulsório: bloco nunca foi carregado antes (há espaço livre).
    } else if (cache->assoc == 1) {
        cache->conflict_misses++; //Miss de conflito: ocorre na cache mapeada diretamente (assoc == 1).
    } else {
        cache->capacity_misses++; //Miss de capacidade: ocorre quando todas as linhas estão ocupadas e um bloco precisa ser substituído.
    }

    // Substituição de bloco
    int replace_index = empty_index;
    if (replace_index == -1) {
        if (cache->replacement == 'R') {
            replace_index = rand() % cache->assoc;
        } else if (cache->replacement == 'F') {
            int oldest = 0;
            for (int i = 1; i < cache->assoc; i++) {
                if (set->lines[i].age < set->lines[oldest].age) {
                    oldest = i;
                }
            }
            replace_index = oldest;
        
        } else if (cache->replacement == 'L') {
            int lru = 0;
            for (int i = 1; i < cache->assoc; i++) {
                if (set->lines[i].age < set->lines[lru].age) {
                    lru = i;
                }
            }
            replace_index = lru;
            // Atualiza a idade do bloco acessado
            set->lines[lru].age = cache->accesses; // Usando um contador global ou número de acesso

        }
    }

    // Atualiza a cache
    set->lines[replace_index].valid = 1;
    set->lines[replace_index].tag = tag; //atualiza a linha escolhida com os novos dados
    set->lines[replace_index].age = cache->accesses;
}

// Relatório final
void cache_report(Cache *cache, int flag_saida) {
    double hit_rate = (double)cache->hits / cache->accesses;
    double miss_rate = 1.0 - hit_rate;
    double compulsory_miss_rate = (double)cache->compulsory_misses / cache->misses;
    double capacity_miss_rate = (double)cache->capacity_misses / cache->misses;
    double conflict_miss_rate = (double)cache->conflict_misses / cache->misses;

    if (flag_saida == 0) {
        printf("Total de acessos: %d\n", cache->accesses);
        printf("Taxa de hits: %.2f\n", hit_rate);
        printf("Taxa de miss: %.2f\n", miss_rate);
        printf("Miss compulsorio: %.2f\n", compulsory_miss_rate);
        printf("Miss de capacidade: %.2f\n", capacity_miss_rate);
        printf("Miss de conflito: %.2f\n", conflict_miss_rate);
    } else {
        printf("%d %.2f %.2f %.2f %.2f %.2f\n",
               cache->accesses, hit_rate, miss_rate,
               compulsory_miss_rate, capacity_miss_rate, conflict_miss_rate);
    }
}

// Processamento do arquivo binário
void process_file(Cache *cache, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    uint32_t address;
    while (fread(&address, sizeof(uint32_t), 1, file) == 1) { //Retorna 1 a cada 4 bytes lidos. Continua lendo enquanto continuar retornando 1.
        address = __builtin_bswap32(address); // Converte Big Endian para Little Endian

        cache_access(cache, address);
    }

    fclose(file);
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Uso: cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> <arquivo_entrada>\n");
        return EXIT_FAILURE;
    }

    int nsets = atoi(argv[1]);
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char replacement = argv[4][0];
    int flag_saida = atoi(argv[5]);
    const char *filename = argv[6];

    srand(time(NULL));

    Cache *cache = cache_init(nsets, bsize, assoc, replacement);
    process_file(cache, filename);
    cache_report(cache, flag_saida);

    free(cache);
    return 0;
}
