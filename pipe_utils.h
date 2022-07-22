/**
 * pipe_utils.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __PIPE_UTILS_H__
    #define __PIPE_UTILS_H__

    int **initialize_multiple_pipes(int size);
    void close_multiple_pipes(int **array_pipe, int size, int not_close);
    void free_multiple_pipes(int **array_pipe, int size);
#endif