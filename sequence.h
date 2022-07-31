/**
 * sequence.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __SEQUENCE_H__
    #define __SEQUENCE_H__
    #include <stdint.h>

    /**
     * Estructura que representa una secuencia de enteros.
     * Posee las funciones de crear, liberar, imprimir, ordenar, mezclar
     * secuencias, como de extraer y escribir secuencias a un archivo.
     *
     * Atributos:
     *      - data: puntero a un arreglo de enteros.
     *      - size: número de elementos de la secuencia.
     */
    typedef struct {
        int64_t *data;
        int size;
    } sequence_t;

    void selection_sort(sequence_t *sequence);
    void free_sequence(sequence_t *sequence);
    void print_sequence(sequence_t *sequence);
    sequence_t *create_sequence(int size);
    sequence_t *extract_sequence(char *path);
    sequence_t *merge_sequence(sequence_t *sequence1, sequence_t *sequence2);
    int write_sequence(sequence_t **sequences, int num_seq, char *path);
#endif