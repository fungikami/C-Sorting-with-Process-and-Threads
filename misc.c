/**
 * misc.c
 * Implementación de diversas funciones que se harán uso durante el programa.
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */
 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/stat.h>
#include "misc.h"

/**
 * Función que determina si un archivo es un directorio.
 * Parámetros:
 *      path: ruta del archivo.
 * Retorno:
 *      No nulo es una archivo de directorio, NULL en caso contrario.
 */
int is_dir_file(char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        fprintf(stderr, "No se pudo aplicar stat sobre el archivo %s\n", path);
        return -1;
    }
    return S_ISDIR(st.st_mode);
}

/**
 * Función que determina si un archivo es regular.
 * Parámetros:
 *      path: ruta del archivo.
 * Retorno:
 *      No nulo si es un archivo regular, NULL en caso contrario.
 */
int is_reg_file(char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        fprintf(stderr, "No se pudo aplicar stat sobre el archivo %s\n", path);
        return -1;
    }
    return S_ISREG(st.st_mode);
}  

/**
 * Función que determina si un archivo es un archivo de texto.
 * Parámetros:
 *      path: ruta del archivo.
 * Retorno:
 *      0 si no es un archivo de texto, distinto de 0 en caso contrario.
 */
int is_txt_file(char *path) {
    char *ext = strrchr(path, '.');
    if (!ext) return 0;
    return strcmp(ext, ".txt") == 0;
}

/**
 * Función que verifica los argumentos de entrada.
 * Parámetros:
 *      argc: número de argumentos.
 *      argv: argumentos.
 * Retorno:
 *      0 si los argumentos son correctos, -1 en caso contrario.
 */
int verify_arguments(int argc, char **argv) {
    int num_ordenadores, num_mezcladores;
    char *raiz;
    
    /* Verificar argumentos que sean 5 argumentos */
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <num Ordenadores> <num Mezcladores> <raiz> <archivo salida>\n", argv[0]);
        return -1;
    }

    /* Verificar que num Ordenadores y num Mezcladores sean enteros */
    num_ordenadores = atoi(argv[1]);
    num_mezcladores = atoi(argv[2]);
    if (num_ordenadores <= 0 || num_mezcladores <= 0) {
        fprintf(stderr, "Los argumentos <num Ordenadores> <num Mezcladores> deben ser enteros positivos\n");
        return -1;
    }

    /* Verificar que la ruta de la raiz sea un directorio */
    raiz = argv[3];
    if (is_dir_file(raiz) == -1) {
        fprintf(stderr, "El argumento <raiz> debe ser un directorio\n");
        return -1;
    }

    return 0;
}