#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "misc.h"
#include "ordenaproc.h"

#define READ_END 0
#define WRITE_END 1

/**
 * Función que implementa el proceso Lector. Se engarga de:
 * - Crear los procesos/hilos restantes.
 * - Inicializar los mecanismos de comunicación necesarios.
 * - Encontrar los archivos con extensión txt.
 * - Por cada archivo no procesador: asigna a un ordenador.
 * - Indicarle a los Mezcladores y al Escritor que deben realizar el último paso.
 * - Destruir los procesos/hilos creados. Terminar.
 *
 * Parámetros:
 * - raiz: directorio raiz del árbol de directorios a procesar.
 * - num_ord: número de ordenadores.
 * - num_mez: número de mezcladores.
 * - salida: nombre del archivo de salida.
 *
 * Retorno:
 * - 0 si todo fue bien, -1 si hubo un error.
 */
int lector(char *raiz, int num_ord, int num_mez, char *salida) {
    /* Ordenadores y mezcladores disponibles, pipe lector-escritor */
    int i, free_ord[2], free_mezc[2], lec_esc[2];
    int **lec_ord = malloc(num_ord * sizeof(int *));   /* Pipes de lector a ordenador */
    int **ord_mezc = malloc(num_mez * sizeof(int *));  /* Pipes de ordenador a mezclador */
    int **mezc_esc = malloc(num_mez * sizeof(int *));  /* Pipes de mezclador a escritor */

    /* Crea pipes de lector-ordenador */
    if (pipe(free_ord) == -1) return -1;
    for (i = 0; i < num_ord; i++) {
        lec_ord[i] = malloc(2 * sizeof(int));
        if (!lec_ord[i] || pipe(lec_ord[i]) == -1) return -1;
    }

    /* Crea pipes de ordenador-mezclador */
    if (pipe(free_mezc) == -1) return -1;
    for (i = 0; i < num_mez; i++) {
        ord_mezc[i] = malloc(2 * sizeof(int));
        if (!ord_mezc[i] || pipe(ord_mezc[i]) == -1) return -1;
    }

    /* Crea ordenadores */
    ordenador(num_ord, num_mez, free_ord, lec_ord, free_mezc, ord_mezc);

    /* Crea pipes de mezclador-escritor y procesos mezcladores */
    for (i=0 ; i<num_mez ; i++) {
        mezc_esc[i] = malloc(2 * sizeof(int));
        if (!mezc_esc[i] || pipe(mezc_esc[i]) == -1) return -1;
    }
    mezclador(num_ord, num_mez, free_ord, lec_ord, free_mezc, ord_mezc, mezc_esc);

    /* Crea pipes de lector-escritor y el proceso escritor*/
    if (pipe(lec_esc) == -1) return -1;
    escritor(num_ord, num_mez, free_ord, lec_ord, free_mezc, ord_mezc, lec_esc, mezc_esc, salida);

    /* Cierra los extremos a no utilizar de lector-ordenador y lector-escritor*/
    close(free_ord[WRITE_END]);
    for (i=0; i<num_ord; i++) close(lec_ord[i][READ_END]);
    close(lec_esc[READ_END]);

    /* Cierra los extremos ordenador-mezclador */
    for (i=0; i<num_mez; i++) {
        close(ord_mezc[i][READ_END]);
        close(ord_mezc[i][WRITE_END]);
    }

    /* Recorre el directorio raíz y asigna los archivos a ordenadores */
    traverse_dir(raiz, free_ord, lec_ord);

    /* Cierra los extremos de los pipes utilizados de lector-ordenador */
    for (i = 0; i < num_ord; i++) close(lec_ord[i][WRITE_END]);
    for (i = 0; i < num_ord; i++) {
        int ord;
        read(free_ord[READ_END], &ord, sizeof(int));
        close(lec_ord[ord][READ_END]);
    }
    close(free_ord[READ_END]);

    /* Lee los mezcladores desocupados y cierra el pipe ordenador-mezclador*/
    for (i = 0; i < num_mez; i++) {
        int mez;
        read(free_mezc[READ_END], &mez, sizeof(int));
        close(ord_mezc[mez][READ_END]);

        /* Encola el mezclador al escritor para mandar las secuencias */
        write(lec_esc[WRITE_END], &mez, sizeof(int));
    }
    close(free_mezc[READ_END]);
    close(lec_esc[WRITE_END]);

    for (i = 0; i < num_mez + num_ord + 1; i++) wait(NULL);

    /* Libera memoria */
    for (i = 0; i < num_ord; i++) free(lec_ord[i]);
    for (i = 0; i < num_mez; i++) {
        free(ord_mezc[i]);
        free(mezc_esc[i]);
    }
    free(lec_ord);
    free(ord_mezc);
    free(mezc_esc);

    printf("Terminó el lector\n");
    return 0;
}

void ordenador(
    int num_ord, int num_mez,
    int *free_ord, int **lec_ord, 
    int *free_mezc, int **ord_mezc
) {
    int pid, i;
    for (i = 0; i < num_ord; i++) {
        if ((pid = fork()) == 0) {
            int j;

            /* Cierra los extremos a no utilizar de lector-ordenador y ordenador-mezclador */
            close(lec_ord[i][WRITE_END]);
            close(free_ord[READ_END]);
            close(free_mezc[WRITE_END]);
            for (j = 0; j < num_mez; j++) close(ord_mezc[j][READ_END]);

            /* Cierra extremos de otras pipes de lector-ordenador */
            for (j=0; j<num_ord; j++) {
                if (j!=i) {
                    close(lec_ord[j][READ_END]);
                    close(lec_ord[j][WRITE_END]);
                }
            }
            
            for (;;) {
                char *filename;
                int n, r, w, mezc, size;
                int64_t *secuencia;

                /* Encola el ordenador como disponible */
                w = write(free_ord[WRITE_END], &i, sizeof(int));

                /* Lee el tamaño del filename asignado */
                r = read(lec_ord[i][READ_END], &n, sizeof(int));
                if (r == 0) break;
                
                /* Guarda el filename */
                filename = malloc((n + 1) * sizeof(char));
                r = read(lec_ord[i][READ_END], filename, n + 1);

                /* Ordena los números del archivo */
                printf("Ordenador %d ordenando %s\n", i, filename);
                secuencia = file_selection_sort(filename, &size);
                free(filename);

                /* Toma un mezclador disponible */
                r = read(free_mezc[READ_END], &mezc, sizeof(int));

                /* Encola el tamaño y la secuencia */
                write(ord_mezc[mezc][WRITE_END], &size, sizeof(int));
                for (j=0; j<size; j++) {
                    write(ord_mezc[mezc][WRITE_END], &secuencia[j], sizeof(int64_t));
                }
            }

            /* Cierra los extremos de los pipes utilizados de ordenador-mezclador */
            for (j=0; j<num_mez; j++) close(ord_mezc[j][WRITE_END]);
            close(free_mezc[READ_END]);
            exit(0);

        } else if (pid < 0) {
            perror("Error al crear proceso de ordenador\n");
            exit(1);
        }
    }
}

void mezclador(
    int num_ord, int num_mez,
    int *free_ord, int **lec_ord, 
    int *free_mezc, int **ord_mezc, int **mezc_esc
) {
    int pid, i;

    for (i=0; i<num_mez; i++) {
        if ((pid = fork()) == 0) {
            int j, size = 0;

            /* Inicializa la secuencia vacía */
            int64_t *secuencia = NULL;

            /* Cierra los pipes de lector-ordenador */
            close(free_ord[READ_END]);
            close(free_ord[WRITE_END]);
            for (j=0; j<num_ord; j++) {
                close(lec_ord[j][READ_END]);
                close(lec_ord[j][WRITE_END]);
            }

            /* Cierra los extremos que no se utilizarán */
            close(free_mezc[READ_END]);
            close(ord_mezc[i][WRITE_END]);
            close(mezc_esc[i][READ_END]);
            for (j=0; j<num_mez; j++) {
                if (j!=i) {
                    close(ord_mezc[j][READ_END]);
                    close(ord_mezc[j][WRITE_END]);
                    close(mezc_esc[j][READ_END]);
                    close(mezc_esc[j][WRITE_END]);
                }
            }

            for (;;) {
                int n, r;
                int64_t *secuencia_ordenada, *secuencia_mezclada;

                /* Encolar mezclador como disponible */
                write(free_mezc[WRITE_END], &i, sizeof(int));

                /* Lee el tamaño de la secuencia */
                r = read(ord_mezc[i][READ_END], &n, sizeof(int));
                if (r == 0) break;

                printf("Mezclador %d mezclando %d\n", i, n);

                /* Lee los números de la secuencia */
                secuencia_ordenada = malloc(n * sizeof(int64_t));
                for (j = 0; j < n; j++) {
                    int64_t m;
                    read(ord_mezc[i][READ_END], &m, sizeof(int64_t));
                    secuencia_ordenada[j] = m;
                }

                /* Mezcla la secuencia con la secuencia ordenada */
                secuencia_mezclada = merge_sequence(secuencia, size, secuencia_ordenada, n, &size);
                free(secuencia);
                free(secuencia_ordenada);
                secuencia = secuencia_mezclada;
            }

            close(free_mezc[WRITE_END]);

            /* Pasa la secuencia al escritor */
            write(mezc_esc[i][WRITE_END], &size, sizeof(int));
            for (j = 0; j < size; j++) {
                write(mezc_esc[i][WRITE_END], &secuencia[j], sizeof(int64_t));
            }

            printf("Mezclador %d terminado\n", i);
            exit(0);
        } else if (pid < 0) {
            perror("Error al crear proceso de mezclador\n");
            exit(1);
        }
    }
}

void escritor(
    int num_ord, int num_mez,
    int *free_ord, int **lec_ord, 
    int *free_mezc, int **ord_mezc, 
    int *lec_esc, int **mezc_esc,
    char *salida
) {
    int pid;
    if ((pid = fork()) == 0) {
        int j, n = 0;

        /* Arreglo de arreglo de secuencias y tamaños */
        int64_t **secuencias = malloc(num_mez * sizeof(int64_t *));
        int *tam_secuencias = malloc(num_mez * sizeof(int));

        /* Cierra los pipes de lector-ordenador */
        close(free_ord[READ_END]);
        close(free_ord[WRITE_END]);
        for (j=0; j<num_ord; j++) {
            close(lec_ord[j][READ_END]);
            close(lec_ord[j][WRITE_END]);
        }

        /* Cierra los pipes de mezclador-ordenador */
        close(free_mezc[READ_END]);
        close(free_mezc[WRITE_END]);
        for (j=0; j<num_mez; j++) {
            close(ord_mezc[j][READ_END]);
            close(ord_mezc[j][WRITE_END]);
        }

        /* Cierra los extremos que no se utilizarán de mezclador-escritor */
        close(lec_esc[WRITE_END]);
        for (j=0; j<num_mez; j++) close(mezc_esc[j][WRITE_END]);

        for (;;) {
            int mezc, size;
            if (n == num_mez) break;

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
        write_sequence(num_mez, secuencias, tam_secuencias, salida);

        /* Libera memoria */
        for (j=0; j<num_mez; j++) free(secuencias[j]);
        free(secuencias);
        free(tam_secuencias);
        printf("A punto de morir\n");
        exit(0);
    } else if (pid < 0) {
        perror("Error al crear proceso de escritor\n");
        exit(1);
    }
}
