/**
 * sequence.c
 *
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#include <stdio.h>
#include <stdlib.h>
#include "sequence.h"

/**
 * Función que crea una nueva secuencia.
 * Parámetros:
 * 		- size: tamaño de la secuencia.
 * Retorno:
 * 		- NULL en caso de error, en cambio, apuntador de la secuencia.
 */
sequence_t *create_sequence(int size) {
    sequence_t *sequence = (sequence_t *)malloc(sizeof(sequence_t));
    if (sequence == NULL) return NULL;

    sequence->size = size;
    sequence->data = (int64_t *)malloc(sizeof(int64_t) * size);
    if (sequence->data == NULL) return NULL;
    return sequence;
}

/**
 * Función que extrae los números de un archivo y los guarda en una secuencia.
 * Parámetros:
 * 		- filename: nombre del archivo.
 * Retorno:
 * 		- NULL en caso de error, en cambio, apuntador de la secuencia.
 */
sequence_t *extract_sequence(char *filename) {
    sequence_t *sequence;
    int64_t aux, *array = NULL;
    int size = 0;

    FILE *file = fopen(filename, "r");
    if (file == NULL) return NULL;

    /* Se lee el archivo y se guardan los enteros en un arreglo */
    while (fscanf(file, "%ld", &aux) != EOF) {
        array = realloc(array, sizeof(int64_t) * (size + 1));
        /* if (array == NULL) return NULL; */
        array[size] = aux;
        size++;
    }
    fclose(file);

    /* Se crea la secuencia */
    sequence = malloc(sizeof(sequence_t));
    if (sequence == NULL) return NULL;
    sequence->size = size;
    sequence->data = array;
    return sequence;
}

/**
 * Función que ordena los enteros de una secuencia con Selection Sort.
 * Parámetros:
 *      - sequence: apuntador de la secuencia.
 */
void selection_sort(sequence_t *sequence) {
    int64_t aux, *array = sequence->data;
    int i, j, size = sequence->size;
        
    /* Se ordena la secuencia con Selection Sort */
    for (i = 0; i < size - 1; i++) {
        for (j = i + 1; j < size; j++) {
            if (array[i] > array[j]) {
                aux = array[i];
                array[i] = array[j];
                array[j] = aux;
            }
        }
    }
}

/**
 * Función que mezcla dos secuencias ordenadas y guarda el resultado en una
 * nueva secuencia ordenada.
 * Parámetros:
 *      sequence1: secuencia 1 a mezclar.
 *      sequence2: secuencia 2 a mezclar.
 * Retorno:
 *      NULL en caso de error, en cambio, la secuencia resultante.
 */
sequence_t *merge_sequence(sequence_t *sequence1, sequence_t *sequence2) {
    int i, j, k;
    int size1 = sequence1->size, size2 = sequence2->size;
    int size_mezclada = size1 + size2;
    int64_t *secuencia1 = sequence1->data, *secuencia2 = sequence2->data;

    /* Crea la nueva secuencia */
    sequence_t *sequence = create_sequence(size_mezclada);
    if (sequence == NULL) return NULL;

    /* Mezcla las secuencias */
    for (i = 0, j = 0, k = 0; i < size_mezclada; i++) {

        /* Si ambas secuencias no están vacías, revisa cuál de los dos tiene el menor */
        if (j < size1 && k < size2) {
            if (secuencia1[j] < secuencia2[k]) {
                sequence->data[i] = secuencia1[j];
                j++;
            } else {
                sequence->data[i] = secuencia2[k];
                k++;
            }
        } else if (j < size1) {
            /* Si la secuencia 1 no está vacía, se copia el elemento */
            sequence->data[i] = secuencia1[j];
            j++;
        } else {
            /* Si la secuencia 2 no está vacía, se copia el elemento */
            sequence->data[i] = secuencia2[k];
            k++;
        }
    }

    return sequence;
}

/**
 * Función que recibe un arreglo de secuencias y escribe en un archivo los
 * enteros de cada una de ellas de forma ordenada.
 * Parámetros:
 *      sequences: arreglo de secuencias.
 *      num_seq: tamaño del arreglo (número de secuencias).
 *      path: ruta del archivo donde se escribirán los enteros.
 * Retorno:
 *      -1 en caso de error, 0 en caso de éxito.
 */
int write_sequence(sequence_t **sequences, int num_seq, char *path) {
    FILE *f;
    int i, *index, index_total = 0, first = 1;

    /* Crea un arreglo para mantener los índices de cada secuencia */
    if (!(index = malloc(num_seq * sizeof(int)))) return -1;
    for (i = 0; i < num_seq; i++) index[i] = 0;
    
    /* Busca el total de números a escribir */
    for (i = 0; i < num_seq; i++) index_total += sequences[i]->size;
    
    /* Escribe en la salida, viendo el menor de los enteros de las secuencias */
    if ((f = fopen(path, "w")) == NULL) return -1;
    for (;;) {
        int i, min_index = 0;
        int64_t min = 9223372036854775807;

        /* Verifica si todas las secuencias ya fueron escritas */
        if (index_total == 0) break;

        /* Busca el menor entero entre las secuencias */
        for (i = 0; i < num_seq; i++) {
            if (index[i] < sequences[i]->size && sequences[i]->data[index[i]] < min) {
                min = sequences[i]->data[index[i]];
                min_index = i;
            }
        }

        /* Escribe el menor de los enteros de las secuencias */
        if (first) {
            first = 0;
            fprintf(f, "%ld", min);
        } else {
            fprintf(f, "\n%ld", min);
        }

        index[min_index]++;
        index_total--;
    }

    free(index);
    fclose(f);
    return 0;
}

/**
 * Función que libera la memoria de una secuencia.
 * Parámetros:
 * 		- sequence: apuntador a la secuencia. 
 */
void free_sequence(sequence_t *sequence) {
    if (sequence == NULL) return;
    free(sequence->data);
    free(sequence);
}