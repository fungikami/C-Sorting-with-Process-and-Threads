/**
 * misc.c
 * Misceláneo
 * Implementación de diversas funciones que se harán uso durante el programa.
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */
 
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "misc.h"

#define READ_END 0
#define WRITE_END 1

/**
 * Función que determina si un archivo es un directorio.
 * 
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
 * 
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

int is_txt_file(char *path) {
    char *ext = strrchr(path, '.');
    if (ext == NULL) return 0;
    return strcmp(ext, ".txt") == 0;
}

/**
 * Función que recorre recursivamente desde un directorio dado y ejecuta una función indicada.
 * 
 * Parámetros:
 *      fun: función a ejecutar por cada archivo.
 *      args: argumentos de la función.
 *      action: indica si la función a ejecutar es para un directorio y/o un archivo
 *          (0 si es para regulares, 1 si es para directorios, 2 si es para ambos casos)
 * Retorno:
 *      0 si todo fue correcto, -1 si hubo un error durante la ejecución.
 */
int traverse_dir(char *path, int *ord_lec, int **lec_ord) {
    DIR* dir;
    struct dirent* ent;

    /* Verifica que existe el directorio */
    if (!(dir = opendir(path))) {
        fprintf(stderr, "Error al abrir el directorio %s\n", path);
        return -1;
    } 
    
    /* Recorre el contenido del directorio */
    while ((ent = readdir(dir))) {
        char* e_name = ent->d_name;
        int dots = strcmp(e_name, ".") == 0 || strcmp(e_name, "..") == 0;

        /* Concatena la nueva direccion */
        char* new_path = malloc(sizeof(char) * (strlen(path) + strlen(e_name) + 2));
        strcpy(new_path, path);
        strcat(new_path, "/");
        strcat(new_path, e_name);

        /* Se revisa el contenido del archivo, evitando aquellos que terminen en '.' o '..' */
        if (!dots) {
            int is_dir = is_dir_file(new_path);
            if (is_dir == -1) {
                free(new_path);
                continue;
            }

            if (is_dir) {
                /* Si es un directorio, se sigue recorriendo recursivamente */
                if (traverse_dir(new_path, ord_lec, lec_ord) == -1) {
                    free(new_path);
                    continue;
                }
            } else {
                /* Si es un archivo regular, se revisa si es txt */
                int is_reg = is_reg_file(new_path);
                if (is_reg == -1) {
                    free(new_path);
                    continue;
                }

                /* Si es un archivo txt, se le asigna a un ordenador */
                if (is_reg && is_txt_file(new_path)) {
                    int n, m;

                    /* Lee que ordenador le asignaron */
                    read(ord_lec[READ_END], &n, sizeof(int));

                    /* Escribe en la pipe el tamaño y nombre del archivo */
                    m = strlen(new_path);
                    write(lec_ord[n][WRITE_END], &m, sizeof(int));
                    write(lec_ord[n][WRITE_END], new_path, m + 1);
                    
                }
            }
        }
        free(new_path);
    }
    closedir(dir);
    return 0;
}

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

/**
 * Función que ordena los enteros de un archivo con selection sort.
 * Parámetros:
 *      path: ruta del archivo.
 * Retorno:
 *      Array de enteros ordenados.
 */
int64_t *file_selection_sort(char *path, int *n) {
    int64_t *array = NULL, aux;
    int i, j;
    int size = 0;
    
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "No se pudo abrir el archivo %s\n", path);
        return NULL;
    }

    /* Se lee el archivo y se guardan los enteros en un array */
    while (fscanf(file, "%ld", &aux) != EOF) {
        array = realloc(array, sizeof(int64_t) * (size + 1));
        array[size] = aux;
        size++;
    }
    fclose(file);
    
    /* Se ordena el array con Selection Sort */
    for (i = 0; i < size - 1; i++) {
        for (j = i + 1; j < size; j++) {
            if (array[i] > array[j]) {
                aux = array[i];
                array[i] = array[j];
                array[j] = aux;
            }
        }
    }

    *n = size;

    return array;
}

/**
 * Función que mezcla dos secuencias ya ordenadas.
 */
int64_t *merge_sequence(int64_t *secuencia1, int size1, int64_t *secuencia2, int size2, int *size) {
    int i, j, k;
    int64_t *secuencia_mezclada;
    int size_mezclada = size1 + size2;
    secuencia_mezclada = malloc(size_mezclada * sizeof(int64_t));
    
    for (i = 0, j = 0, k = 0; i < size_mezclada; i++) {
        if (j < size1 && k < size2) {
            if (secuencia1[j] < secuencia2[k]) {
                secuencia_mezclada[i] = secuencia1[j];
                j++;
            } else {
                secuencia_mezclada[i] = secuencia2[k];
                k++;
            }
        } else if (j < size1) {
            secuencia_mezclada[i] = secuencia1[j];
            j++;
        } else {
            secuencia_mezclada[i] = secuencia2[k];
            k++;
        }
    }
    *size = size_mezclada;

    return secuencia_mezclada;
}

/**
 * Función que recibe un arreglo de arreglo de secuencias y escribe en el archivo de salida
 * los elementos de las secuencias en orden.
 * 
 */
int write_sequence(int num_secuencias, int64_t **secuencias, int *tam_secuencias, char *salida) {
    FILE *f = fopen(salida, "w");
    int i, *index = malloc(num_secuencias * sizeof(int));
    for (i = 0; i < num_secuencias; i++) index[i] = 0;

    if (f == NULL) {
        printf("Error al abrir el archivo de salida\n");
        return -1;
    }
    
    /* Escribe en la salida, viendo el menor de todas las secuencias sucesivamente */
    for (;;) {
        int i, to_break = 1;
        int64_t min = 9223372036854775807;
        int min_index = 0;

        for (i = 0; i < num_secuencias; i++) {
            if (index[i] < tam_secuencias[i]) {
                to_break = 0;
            }
        }

        if (to_break) break;

        for (i = 0; i < num_secuencias; i++) {
            if (index[i] < tam_secuencias[i] && (secuencias[i][index[i]] < min)) {
                min = secuencias[i][index[i]];
                min_index = i;
            }
        }
        fprintf(f, "%ld\n", min);
        index[min_index]++;
    }
    free(index);
    fclose(f);
    return 0;
}