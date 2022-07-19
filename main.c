#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "misc.h"

#define READ_END 0
#define WRITE_END 1

#define NUM_ORDS 1
#define NUM_MEZC 2
#define NUM_FILES 31

int lector(char *raiz, int num_ord, int num_mez, char *salida);
void ordenador(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc);
void mezclador(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc, int **mezc_esc);
void escritor(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc, int *lec_esc, int **mezc_esc, char *salida);
int escribe_secuencia(int64_t **secuencias, int *tam_secuencias, char *salida);

int main(int argc, char *argv[]) {
    char *raiz = argv[1];
    int num_ord = atoi(argv[2]);
    int num_mez = atoi(argv[3]);
    char *salida = argv[4];

    printf("Directorio: %s\n", argv[1]);

    /* Invoca al lector */
    lector(raiz, num_ord, num_mez, salida);

    return 0;
}

int lector(char *raiz, int num_ord, int num_mez, char *salida) {
    /* Ordenadores y mezcladores disponibles, pipe lector-escritor */
    int ord_lec[2], mezc_ord[2], lec_esc[2];
    int **lec_ord = malloc(NUM_ORDS * sizeof(int *));   /* Pipes de lector a ordenador */
    int **ord_mezc = malloc(NUM_MEZC * sizeof(int *));  /* Pipes de ordenador a mezclador */
    int **mezc_esc = malloc(NUM_MEZC * sizeof(int *));  /* Pipes de mezclador a escritor */
    int i;

    /* Crea pipes de lector-ordenador y ordenador-mezclador */
    for (i = 0; i < NUM_ORDS; i++) {
        lec_ord[i] = malloc(2 * sizeof(int));
        pipe(lec_ord[i]);
    }

    for (i = 0; i < NUM_MEZC; i++) {
        ord_mezc[i] = malloc(2 * sizeof(int));
        pipe(ord_mezc[i]);
    }

    pipe(ord_lec);
    pipe(mezc_ord);

    /* ================ ORDENADORES ================ */
    ordenador(ord_lec, lec_ord, mezc_ord, ord_mezc);

    /* Crea pipes de mezclador-escritor */
    for (i=0 ; i<NUM_MEZC ; i++) {
        mezc_esc[i] = malloc(2 * sizeof(int));
        pipe(mezc_esc[i]);
    }

    /* ================ MEZCLADORES ================ */
    mezclador(ord_lec, lec_ord, mezc_ord, ord_mezc, mezc_esc);

    /* Crea pipes de lector-escritor */
    pipe(lec_esc);

    /* ================ ESCRITOR ================ */
    escritor(ord_lec, lec_ord, mezc_ord, ord_mezc, lec_esc, mezc_esc, salida);

    /* ============= LECTOR ============= */
    /* No usa la escritura de cola de ords */
    close(ord_lec[WRITE_END]);
    close(lec_esc[READ_END]);

    for (i=0; i<NUM_MEZC; i++) {
        close(ord_mezc[i][READ_END]);
        close(ord_mezc[i][WRITE_END]);
    }

    /* No usa la lectura de nadie */
    for (i=0; i<NUM_ORDS; i++)
        close(lec_ord[i][READ_END]);

    /* Pasarle a los ordenadores los archivos */
    /* DENTRO DE TRAVERSE_DIR */
    traverse_dir(raiz, ord_lec, lec_ord);

    /* Cierra los extremos de los pipes */
    for (i=0; i<NUM_ORDS; i++)
        close(lec_ord[i][WRITE_END]);

    for (i=0; i<NUM_ORDS; i++) {
        int n;
        read(ord_lec[READ_END], &n, sizeof(int));
        close(lec_ord[n][READ_END]);
    }
    close(ord_lec[READ_END]);

    /* Cierra los extremos de los pipes */
    for (i=0; i<NUM_MEZC; i++)
        close(ord_mezc[i][WRITE_END]);

    for (i=0; i<NUM_MEZC; i++) {
        int n;
        read(mezc_ord[READ_END], &n, sizeof(int));

        /* Cierra pipe */
        close(ord_mezc[n][READ_END]);

        /* Se encola el mezclador al escritor */
        write(lec_esc[WRITE_END], &n, sizeof(int));
    }

    close(mezc_ord[READ_END]);
    close(lec_esc[WRITE_END]);

    for (i=0; i<NUM_MEZC+NUM_ORDS+1;i++) wait(NULL);
    printf("Terminó el padre\n");
    return 0;
}

void ordenador(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc) {
    int pid, i;
    for (i = 0; i < NUM_ORDS; i++) {
        if ((pid = fork()) == 0) {
            int j;

            /* Recibe string de archivo */
            close(lec_ord[i][WRITE_END]);
            close(ord_lec[READ_END]);
            close(mezc_ord[WRITE_END]);

            /* Cierra extremos de otras pipes */
            for (j=0; j<NUM_ORDS; j++) {
                if (j!=i) {
                    close(lec_ord[j][READ_END]);
                    close(lec_ord[j][WRITE_END]);
                }
            }

            /* Cierra extremo de lectura */
            for (j=0; j<NUM_MEZC; j++) 
                close(ord_mezc[j][READ_END]);

            while (1) {
                int n, r, m, size;
                int64_t *secuencia;
                char *filename;
                write(ord_lec[WRITE_END], &i, sizeof(int));

                /* Lee el tamaño del filename */
                r = read(lec_ord[i][READ_END], &n, sizeof(int));
                if (r == 0) break;
                
                /* Guarda el filename */
                filename = malloc(n + 1);
                read(lec_ord[i][READ_END], filename, n + 1);

                /* Lectura de datos */
                printf("Ordenador %d ordenando %s\n", i, filename);
                secuencia = file_selection_sort(filename, &size);

                /* Toma un mezclador disponible */
                read(mezc_ord[READ_END], &m, sizeof(int));

                /* Encola el tamaño y la secuencia */
                write(ord_mezc[m][WRITE_END], &size, sizeof(int));
                for (j=0; j<size; j++) {
                    write(ord_mezc[m][WRITE_END], &secuencia[j], sizeof(int64_t));
                }
            }

            for (j=0; j<NUM_MEZC; j++) {
                close(ord_mezc[j][WRITE_END]);
            }
            close(mezc_ord[READ_END]);

            exit(0);
        } else if (pid > 0) {
            
        } else {
            perror("Error: fork");
            exit(1);
        }
    }
}

void mezclador(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc, int **mezc_esc) {
    int pid, i;

    for (i=0; i<NUM_MEZC; i++) {
        if ((pid = fork()) == 0) {
            int j, size = 0;

            /* Inicializa la secuencia vacía */
            int64_t *secuencia = NULL;

            /* Cierra los pipes de lector-ordenador */
            close(ord_lec[READ_END]);
            close(ord_lec[WRITE_END]);
            for (j=0; j<NUM_ORDS; j++) {
                close(lec_ord[j][READ_END]);
                close(lec_ord[j][WRITE_END]);
            }

            /* Cierra los extremos que no se utilizarán */
            close(ord_mezc[i][WRITE_END]);
            close(mezc_ord[READ_END]);
            close(mezc_esc[i][READ_END]);
            for (j=0; j<NUM_MEZC; j++) {
                if (j!=i) {
                    close(ord_mezc[j][READ_END]);
                    close(ord_mezc[j][WRITE_END]);
                    close(mezc_esc[j][READ_END]);
                    close(mezc_esc[j][WRITE_END]);
                }
            }

            while (1) {
                int n, r;
                int64_t *secuencia_ordenada;

                /* Encolar mezclador como disponible */
                write(mezc_ord[WRITE_END], &i, sizeof(int));

                /* Lee el tamaño de la secuencia */
                r = read(ord_mezc[i][READ_END], &n, sizeof(int));
                if (r == 0) break;

                /* printf("Mezclador %d mezclando %d\n", i, n); */
                /* Lee los números de la secuencia */
                secuencia_ordenada = malloc(n * sizeof(int64_t));
                for (j = 0; j < n; j++) {
                    int64_t m;
                    read(ord_mezc[i][READ_END], &m, sizeof(int64_t));
                    secuencia_ordenada[j] = m;
                }

                /* Mezcla la secuencia con la secuencia ordenada */
                secuencia = mezclar_sec(secuencia, size, secuencia_ordenada, n, &size);
            }

            close(mezc_ord[WRITE_END]);

            /* Pasa la secuencia al escritor */
            write(mezc_esc[i][WRITE_END], &size, sizeof(int));
            for (j = 0; j < size; j++) {
                write(mezc_esc[i][WRITE_END], &secuencia[j], sizeof(int64_t));
            }

            printf("Mezclador %d terminado\n", i);
            exit(0);
        } 
    }
}

void escritor(
    int *ord_lec, int **lec_ord, 
    int *mezc_ord, int **ord_mezc, 
    int *lec_esc, int **mezc_esc,
    char *salida
) {
    int pid;
    if ((pid = fork()) == 0) {
        int j, n = 0;
        /* Arreglo de arreglo de secuencias */
        int64_t **secuencias = malloc(NUM_MEZC * sizeof(int64_t *));

        /* Arreglo del tamaño de cada secuencia */
        int *tam_secuencias = malloc(NUM_MEZC * sizeof(int));

        /* Cierra los pipes de lector-ordenador */
        close(ord_lec[READ_END]);
        close(ord_lec[WRITE_END]);
        for (j=0; j<NUM_ORDS; j++) {
            close(lec_ord[j][READ_END]);
            close(lec_ord[j][WRITE_END]);
        }
        close(mezc_ord[READ_END]);
        close(mezc_ord[WRITE_END]);
        for (j=0; j<NUM_MEZC; j++) {
            close(ord_mezc[j][READ_END]);
            close(ord_mezc[j][WRITE_END]);
        }

        /* Cierra los pipes de mezclador-escritor */
        close(lec_esc[WRITE_END]);
        for (j=0; j<NUM_MEZC; j++) {
            close(mezc_esc[j][WRITE_END]);
        }

        while (1) {
            int mezc, size;

            if (n == NUM_MEZC) break;

            /* Lee el mezclador asignado */
            read(lec_esc[READ_END], &mezc, sizeof(int));
            printf("Al escritor %d le dieron mezclador %d\n", n, mezc);
            
            /* Lee el tamaño de la secuencia */
            read(mezc_esc[mezc][READ_END], &size, sizeof(int));
            tam_secuencias[mezc] = size;

            /* Lee la secuencia */
            secuencias[mezc] = malloc(size * sizeof(int64_t));
            for (j = 0; j < size; j++) {
                read(mezc_esc[mezc][READ_END], &secuencias[mezc][j], sizeof(int64_t));
            }

            /* Imprime la secuencia */
            printf("Escritor escribiendo secuencia de %d elementos\n", size);
            for (j=0; j<size; j++) {
                printf("%ld  ", secuencias[mezc][j]);
            }

            n++;
        }
        /* Escribe ordenado salida */
        escribe_secuencia(secuencias, tam_secuencias, salida);
        
        printf("A punto de morir\n");

        exit(0);
    }
}

/**
 * Función que recibe un arreglo de arreglo de secuencias y escribe en el archivo de salida
 * los elementos de las secuencias en orden.
 * 
 */
int escribe_secuencia(int64_t **secuencias, int *tam_secuencias, char *salida) {
    FILE *f = fopen(salida, "w");
    int i, *index = malloc(NUM_MEZC * sizeof(int));
    for (i=0; i<NUM_MEZC; i++) index[i] = 0;

    if (f == NULL) {
        printf("Error al abrir el archivo de salida\n");
        return -1;
    }
    
    /* Escribe en la salida, viendo el menor de todas las secuencias sucesivamente */
    while (1) {
        int i, to_break = 1;
        int64_t min = 9223372036854775807;
        int min_index = 0;

        for (i=0; i<NUM_MEZC; i++) {
            if (index[i] < tam_secuencias[i]) {
                to_break = 0;
            }
        }

        if (to_break) break;

        for (i=0; i<NUM_MEZC; i++) {
            if (index[i] < tam_secuencias[i] && (secuencias[i][index[i]] < min)) {
                min = secuencias[i][index[i]];
                min_index = i;
            }
        }
        fprintf(f, "%ld\n", min);
        index[min_index]++;
    }

    fclose(f);
    return 0;
}
