/**
 * sequence.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __SEQUENCE_H__
    #define __SEQUENCE_H__
    #include <stdint.h>

    typedef struct {
        int64_t *data;
        int size;
    } sequence_t;

    sequence_t *create_sequence(int size);
    sequence_t *file_selection_sort(char *path);
    sequence_t *merge_sequence(sequence_t *sequence1, sequence_t *sequence2);
    void free_sequence(sequence_t *sequence);
    int write_sequence(sequence_t **sequences, int num_seq, char *path);
#endif