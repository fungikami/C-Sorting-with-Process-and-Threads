/**
 * main.c
 *
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "misc.h"

int main(int argc, char **argv) {
    int num_ordenadores, num_mezcladores;
    char *raiz;

    /* Verificar argumentos */
    if (verify_arguments(argc, argv) == -1) return -1;
    
    num_ordenadores = atoi(argv[1]);
    num_mezcladores = atoi(argv[2]);
    raiz = argv[3];

    traverse_dir(raiz);

    return 0;
}

int lector(int num_ordenadores, int num_mezcladores, char *raiz, char *archivo_salida) {
    /* */
    return 0;
}