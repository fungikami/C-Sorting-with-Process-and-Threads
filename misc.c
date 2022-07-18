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
int traverse_dir(char* path) {
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
                if (traverse_dir(new_path) == -1) {
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

                /* Si es un archivo txt */
                if (is_reg && is_txt_file(new_path)) {
                    printf("%s\n", new_path);
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
