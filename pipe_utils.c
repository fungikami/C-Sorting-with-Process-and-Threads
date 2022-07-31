/**
 * pipe_utils.c
 *
 * Implementación de diversas funciones que se harán uso durante el programa
 * de ordenaproc, para crear, cerrar y liberar arreglos de pipes.
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#include <stdlib.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1

/**
 * Función que inicializa un arreglo de pipes.
 * Parámetros:
 *      size: número de pipes a inicializar.
 * Retorno:
 *      NULL si hubo un error, en cambio, puntero a un arreglo de pipes.
 */
int **initialize_multiple_pipes(int size) {
    int i, **pipes = (int **) malloc(sizeof(int *) * size);
    if (!pipes) return NULL;

    for (i = 0; i < size; i++) {
        pipes[i] = (int *) malloc(sizeof(int) * 2);
        if (!pipes[i]) return NULL;

        if (pipe(pipes[i]) == -1) return NULL;
    }
    return pipes;
}

/**
 * Función que cierra los extremos de un arreglo de pipes.
 * Parámetros:
 *      pipes: puntero de punteros de pipes.
 *      size: número de pipes a cerrar. 
 *      not_close: índice de una pipe que no se debe cerrar.
 */
void close_multiple_pipes(int **pipes, int size, int not_close) {
    int i;
    for (i = 0; i < size; i++) {
        if (i != not_close) {
            close(pipes[i][READ_END]);
            close(pipes[i][WRITE_END]);
        }
    }
}

/**
 * Función que libera la memoria asignada a un arreglo de pipes.
 * Parámetros:
 *      pipes: puntero de punteros de pipes.
 *      size: número de pipes a liberar. 
 */
void free_multiple_pipes(int **pipes, int size) {
    int i;
    for (i = 0; i < size; i++) free(pipes[i]);
    free(pipes);
}