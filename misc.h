/**
 * misc.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __MISC_H__
    #define __MISC_H__
    #include <stdint.h>

    int is_dir_file(char *path);
    int is_reg_file(char *path);
    int is_txt_file(char *path);
    int verify_arguments(int argc, char **argv);
    int traverse_dir(char *path, int *ord_lec, int **lec_ord);
    int write_sequence(int num_secuencias, int64_t **secuencias, int *tam_secuencias, char *salida);
    int64_t *file_selection_sort(char *path, int *n);
    int64_t *merge_sequence(int64_t *secuencia1, int size1, int64_t *secuencia2, int size2, int *size);
#endif