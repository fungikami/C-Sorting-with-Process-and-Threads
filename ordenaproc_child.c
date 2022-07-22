/**
 * ordenaproc_child.c
 * 
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "sequence.h"
#include "pipe_utils.h"

#define READ_END 0
#define WRITE_END 1

void ordena(int pos_ord, int num_ord, int ords, int lec_ord, int mezcs, int **ord_mezc);
void mezcla(int pos_mezc, int fd_free_mezc, int fd_ord_mezc, int fd_mezc_esc);
void escribe(int num_mezc, int fd_lec_esc, int **mezc_esc, char *salida);

/**
 * Función que crea los procesos ordenadores.
 *
 * Parámetros:
 * - num_ord: número de ordenadores.
 * - num_mezc: número de mezcladores.
 * - ords: pipe de ordenadores disponibles.
 * - mezcs: pipe de mezcladores disponibles.
 * - lec_ord: pipes de lector-ordenador (lector asigna archivos a ordenador).
 * - ord_mezc: pipes de ordenador-mezclador (ordenador pasa secuencias a mezclador). 
 */
void ordenador(
    int num_ord, int num_mezc,
    int *ords, int *mezcs,
    int **lec_ord, int **ord_mezc
) {
    int i, j, pid;
    for (i = 0; i < num_ord; i++) {
        if ((pid = fork()) == 0) {
            /* Cierra los extremos a no utilizar de lector-ordenador y ordenador-mezclador */
            close(ords[READ_END]);
            close(mezcs[WRITE_END]);
            close(lec_ord[i][WRITE_END]);
            for (j = 0; j < num_mezc; j++) close(ord_mezc[j][READ_END]);
            close_multiple_pipes(lec_ord, num_ord, i);

            ordena(i, num_ord, ords[WRITE_END], lec_ord[i][READ_END], mezcs[READ_END], ord_mezc);

            /* Cierra los extremos de los pipes utilizados */
            close(ords[WRITE_END]);
            close(mezcs[READ_END]);
            close(lec_ord[i][READ_END]);
            for (j = 0; j < num_mezc; j++) close(ord_mezc[j][WRITE_END]);
            exit(0);
        } else if (pid < 0) {
            perror("Error al crear proceso de ordenador\n");
            exit(1);
        }
    }
}

/**
 * Función que toma secuencias de un archivo dado en una pipe y las ordena.
 * Luego, pasa la secuencia ordenada a un mezclador disponible.
 * 
* Parámetros:
* - pos_ord: posición del ordenador en la lista de ordenadores.
* - num_ord: número de ordenadores.
* - fd_ords: fd de lectura de la pipe de ordenadores disponibles.
* - fd_lec_ord: fd de lectura de las pipes de lector-ordenador.
* - fd_mezcs: fd de escritura de la pipe de mezcladores disponibles.
* - ord_mezc: pipes de ordenador-mezclador.
 */
void ordena(
    int pos_ord, int num_ord, 
    int fd_ords, int fd_lec_ord, 
    int fd_mezcs, int **ord_mezc
) {
    for (;;) {
        char *filename;
        int i, aux, size, mezc;
        sequence_t *sequence;

        /* Encola el ordenador como disponible */
        if ((aux = write(fd_ords, &pos_ord, sizeof(int))) == -1) continue;

        /* Lee el tamaño del filename asignado */
        aux = read(fd_lec_ord, &size, sizeof(int));
        if (aux == -1) continue;
        if (aux == 0) break;
        
        /* Guarda el filename */
        if (!(filename = malloc(size * sizeof(char)))) continue;
        if (size != read(fd_lec_ord, filename, size * sizeof(char))) {
            free(filename);
            continue;
        }

        /* Extrae y ordena los números del archivo */
        if (!(sequence = extract_sequence(filename))) {
            free(filename);
            continue;
        }
        selection_sort(sequence);
        free(filename);

        /* Toma un mezclador disponible */
        if ((aux = read(fd_mezcs, &mezc, sizeof(int))) == -1) {
            free_sequence(sequence);
            continue;
        }

        /* Encola el tamaño y la secuencia */
        size = sequence->size;
        if ((aux = write(ord_mezc[mezc][WRITE_END], &size, sizeof(int))) == -1) {
            free_sequence(sequence);
            continue;
        }
        for (i = 0; i < size; i++) {
            aux = write(ord_mezc[mezc][WRITE_END], &sequence->data[i], sizeof(int64_t));
            if (aux == -1) continue;
        }

        /* Libera la memoria de la secuencia */
        free_sequence(sequence);
    }
}

/**
 * Función que crea los procesos mezcladores.
 *
 * Parámetros:
 * - num_ord: número de ordenadores.
 * - num_mezc: número de mezcladores.
 * - ords: pipe de ordenadores disponibles.
 * - mezcs: pipe de mezcladores disponibles.
 * - lec_ord: pipes de lector-ordenador (lector asigna archivos a ordenador).
 * - ord_mezc: pipes de ordenador-mezclador (ordenador pasa secuencias a mezclador).
 * - mezc_esc: pipes de mezclador-escritor (mezclador pasa secuencias a escritor).
 */
void mezclador(
    int num_ord, int num_mezc,
    int *ords, int *mezcs,
    int **lec_ord, int **ord_mezc, int **mezc_esc
) {
    int i, pid;
    for (i = 0; i < num_mezc; i++) {
        if ((pid = fork()) == 0) {
            /* Cierra los pipes de lector-ordenador */
            close(ords[READ_END]);
            close(ords[WRITE_END]);
            close_multiple_pipes(lec_ord, num_ord, -1);

            /* Cierra los extremos que no se utilizarán */
            close(mezcs[READ_END]);
            close(ord_mezc[i][WRITE_END]);
            close(mezc_esc[i][READ_END]);
            close_multiple_pipes(ord_mezc, num_mezc, i);
            close_multiple_pipes(mezc_esc, num_mezc, i);

            mezcla(i, mezcs[WRITE_END], ord_mezc[i][READ_END], mezc_esc[i][WRITE_END]);

            /* Cierra los extremos que se utilizaron*/
            close(mezcs[WRITE_END]);
            close(ord_mezc[i][READ_END]);
            close(mezc_esc[i][WRITE_END]);

            exit(0);
        } else if (pid < 0) {
            perror("Error al crear proceso de mezclador\n");
            exit(1);
        }
    }
}

/**
 * Función que toma secuencias ordenadas de un ordenador y las mezcla.
 * Luego, pasa la secuencia mezclada al escritor.
 * 
 * Parámetros:
 * - pos_mezc: posición del mezclador en la lista de mezcladores.
 * - fd_mezcs: fd de escritura de la pipe de mezcladores disponible.
 * - fd_ord_mezc: fd de lectura de la pipe de ordenador-mezclador.
 * - fd_mezc_esc: fd de escritura de la pipe de mezclador-escritor.
 */
void mezcla(int pos_mezc, int fd_mezcs, int fd_ord_mezc, int fd_mezc_esc) {
    int i, aux, size;
    sequence_t *sequence = create_sequence(0);

    for (;;) {
        sequence_t *ord_seq, *mez_seq;

        /* Encolar mezclador como disponible */
        if ((aux = write(fd_mezcs, &pos_mezc, sizeof(int))) == -1) continue;

        /* Lee el tamaño de la secuencia */
        aux = read(fd_ord_mezc, &size, sizeof(int));
        if (aux == -1) continue;
        if (aux == 0) break;

        /* Lee los números de la secuencia */
        if (!(ord_seq = create_sequence(size))) continue;
        for (i = 0; i < size; i++) {
            if ((aux = read(fd_ord_mezc, &ord_seq->data[i], sizeof(int64_t))) == -1) {
                free_sequence(ord_seq);
                continue;
            }
        }

        /* Mezcla la secuencia con la secuencia ordenada */
        if (!(mez_seq = merge_sequence(sequence, ord_seq))) {
            free_sequence(ord_seq);
            continue;
        }

        free_sequence(sequence);
        free_sequence(ord_seq);
        sequence = mez_seq;
    }

    /* Pasa el tamaño y la secuencia al escritor */
    size = sequence->size;
    if ((aux = write(fd_mezc_esc, &size, sizeof(int))) == -1) {
        free_sequence(sequence);
        return;
    }
    for (i = 0; i < size; i++) {
        aux = write(fd_mezc_esc, &sequence->data[i], sizeof(int64_t));
        if (aux == -1) continue;
    }

    free_sequence(sequence);
}

/**
 * Función qe crea el proceso escritor.
 *
 * Parámetros:
 * - num_ord: número de ordenadores.
 * - num_mezc: número de mezcladores.
 * - ords: pipe de ordenadores disponibles.
 * - mezcs: pipe de mezcladores disponibles.
 * - lec_ord: pipes de lector-ordenador (lector asigna archivos a ordenador).
 * - ord_mezc: pipes de ordenador-mezclador (ordenador pasa secuencias a mezclador).
 * - mezc_esc: pipes de mezclador-escritor (mezclador pasa secuencias a escritor).
 * - salida: archivo donde se escribirá el resultado.
 */
void escritor(
    int num_ord, int num_mezc,
    int *ords, int *mezcs, int *lec_esc,
    int **lec_ord, int **ord_mezc, int **mezc_esc, 
    char *salida
){
    int i, pid;
    if ((pid = fork()) == 0) {
        /* Cierra los pipes de lector-ordenador */
        close(ords[READ_END]);
        close(ords[WRITE_END]);
        close_multiple_pipes(lec_ord, num_ord, -1);

        /* Cierra los pipes de mezclador-ordenador */
        close(mezcs[READ_END]);
        close(mezcs[WRITE_END]);
        close_multiple_pipes(ord_mezc, num_mezc, -1);

        /* Cierra los extremos que no se utilizarán de mezclador-escritor */
        close(lec_esc[WRITE_END]);
        for (i = 0; i < num_mezc; i++) close(mezc_esc[i][WRITE_END]);

        escribe(num_mezc, lec_esc[READ_END], mezc_esc, salida);

        /* Cierra los extremos que se utilizaron de mezclador-escritor */
        close(lec_esc[READ_END]);
        for (i = 0; i < num_mezc; i++) close(mezc_esc[i][READ_END]);

        exit(0);
    } else if (pid < 0) {
        perror("Error al crear proceso de escritor\n");
        exit(1);
    }
}

/**
 * Función que recibe secuencias de los mezcladores  
 * y las escribe en el archivo de salida. 
 *
 * Parámetros:
 * - num_mezc: número de mezcladores.
 * - fd_lec_esc: fd de lectura de la pipe de lector-escritor.
 * - mezc_esc: pipes de mezclador-escritor (mezclador pasa secuencias a escritor).
 * - salida: archivo donde se escribirá el resultado.
 */
void escribe(int num_mezc, int fd_lec_esc, int **mezc_esc, char *salida) {
    int aux, i;

    /* Apuntador de apuntadores de secuencias */
    sequence_t **sequences = malloc(num_mezc * sizeof(sequence_t *));
    if (!sequences) return;

    for (i = 0; i < num_mezc; i++) {
        int j, mezc, size;

        /* Lee el mezclador asignado */
        if ((aux = read(fd_lec_esc, &mezc, sizeof(int))) == -1) continue;

        /* Lee el tamaño de la secuencia */
        if ((aux = read(mezc_esc[mezc][READ_END], &size, sizeof(int))) == -1) continue;
        if (!(sequences[mezc] = create_sequence(size))) continue;

        /* Lee la secuencia */
        for (j = 0; j < size; j++) {
            if ((aux = read(mezc_esc[mezc][READ_END], &sequences[mezc]->data[j], sizeof(int64_t))) == -1) 
                continue;
        }
    }

    /* Escribe ordenado salida y libera la memoria */
    write_sequence(sequences, num_mezc, salida);
    for (i = 0; i < num_mezc; i++) free_sequence(sequences[i]);
    free(sequences);
}
