/**
 * ordena.c
 *
 * Implementación monolítica del programa que ordena de forma ascendente los 
 * enteros almacenados en los archivos ubicados en una jerarquía de directorios.
 * 
 * Comando:
 *      ./ordenahilo <raiz> <archivo salida>
 * donde:
 *      <raiz>: es el directorio raíz que se debe procesar.
 *      <archivo salida>: nombre del archivo que almacenará los enteros ordenados.
 *
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "misc.h"
#include "sequence.h"

int lector(char *root, char *path);
sequence_t *ordenador(char *path);
sequence_t *mezclador(sequence_t *sequence1, sequence_t *sequence2);
int escritor(sequence_t *sequence, char *path);
int traverse_dir(char *path, sequence_t **sequence);

int main(int argc, char *argv[]) {
    /* Verifica los argumentos */
    if (argc != 3) {
        printf("Uso: %s <raiz> <archivo salida>\n", argv[0]);
        return -1;
    }

    /* Verifica que el directorio exista */
    if (is_dir_file(argv[1]) == -1) {
        printf("El directorio %s no existe\n", argv[1]);
        return -1;
    }

    /* Invoca al lector */
    if (lector(argv[1], argv[2]) == -1) return -1;

    return 0;
}

/**
 * Función que implementa el lector.
 * Recorre el árbol de directorios para ordenar las secuencias de los archivos
 * txt encontrados, y lo mezcla en una sola secuencia ordenada.
 * 
 * Parámetros:
 *   - root: ruta del directorio a leer las secuencias.
 *   - path: ruta del archivo a escribir la secuencia ordenada.
 *
 * Retorno:
 *   - 0 si la ejecución fue exitosa. -1 en caso de error.
 */
int lector(char *root, char *path) {
    /* Ordena y mezcla las secuencias */
    sequence_t *sequence = create_sequence(0);
    if (!sequence) return -1;
    traverse_dir(root, &sequence);

    /* Escribe la secuencia en el archivo dado */
    if (escritor(sequence, path) == -1) {
        printf("Error al escribir la secuencia en el archivo de path\n");
        return -1;
    }

    return 0;
}

/**
 * Función que implementa el ordenador. 
 * Extrae la secuencia de un archivo y lo ordena con selection sort.
 *
 * Parámetros:
 *   - path: ruta del archivo a leer.
 *
 * Retorno:
 *   - Apuntador de la secuencia ordenada. NULL en caso de error.
 */
sequence_t *ordenador(char *path) {
    /* Extrae y ordena la secuencia del archivo */
    sequence_t *sort_seq = extract_sequence(path);
    if (!sort_seq) return NULL;
    selection_sort(sort_seq);
    return sort_seq;
}

/**
 * Función que implementa el mezclador.
 * Mezcla dos secuencias ordenadas en una secuencia ordenada.
 *
 * Parámetros:
 *   - sequence1: Apuntador a la primera secuencia.
 *   - sequence2: Apuntador a la segunda secuencia.
 *
 * Retorno:
 *   - Apuntador de la mezcla si la ejecución fue exitosa. 
 *   - NULL en caso de error.
 */
sequence_t *mezclador(sequence_t *sequence1, sequence_t *sequence2) {
    sequence_t *merge_seq;
    if (!(merge_seq = merge_sequence(sequence1, sequence2))) return NULL;
    
    free(sequence1);
    free(sequence2);
    return merge_seq;
}

/**
 * Función que implementa el escritor.
 * Escribe la secuencia ordenada en el archivo dado.
 *
 * Parámetros:
 *   - path: ruta del archivo a escribir.
 *   - sequence: Apuntador a la secuencia a escribir.
 *
 * Retorno:
 *   - 0 si la ejecución fue exitosa. -1 en caso de error.
 */
int escritor(sequence_t *sequence, char *path) {
    FILE *f = fopen(path, "w");
    int i, first = 1;

    if (!f) return -1;

    for (i = 0; i < sequence->size; i++) {
        if (first) {
            first = 0;
            fprintf(f, "%ld", sequence->data[i]);
        } else {
            fprintf(f, "\n%ld", sequence->data[i]);
        }
    }

    fclose(f);
    return 0;
}

/**
 * Función que recorre recursivamente desde un directorio dado,
 * busca los archivos txt para ordenarlos y guardarlos en una secuencia.
 * 
 * Parámetros:
 *   - path: ruta del directorio a recorrer.
 *   - sequence: Apuntador a la secuencia ordenada.
 *
 * Retorno:
 *      0 si todo fue correcto, -1 si hubo un error durante la ejecución.
 */
int traverse_dir(char *path, sequence_t **sequence) {
    DIR *dir;
    struct dirent *ent;

    /* Verifica que existe el directorio */
    if (!(dir = opendir(path))) {
        fprintf(stderr, "Error al abrir el directorio %s\n", path);
        return -1;
    } 
    
    /* Recorre el contenido del directorio */
    while ((ent = readdir(dir))) {
        char* e_name = ent->d_name;
        int is_dir, dots = (strcmp(e_name, ".") == 0 || strcmp(e_name, "..") == 0);

        /* Concatena la nueva direccion */
        char* new_path = malloc(sizeof(char) * (strlen(path) + strlen(e_name) + 2));
        if (!new_path) continue;
        strcpy(new_path, path);
        strcat(new_path, "/");
        strcat(new_path, e_name);

        /* Se evitan los archivos que terminen en '.' o '..' */
        if (dots) {
            free(new_path);
            continue;
        }

        if ((is_dir = is_dir_file(new_path)) == -1) {
            free(new_path);
            continue;
        }

        if (is_dir) {
            /* Si es un directorio, se sigue recorriendo recursivamente */
            if (traverse_dir(new_path, sequence) == -1) {
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

            /* Si es un archivo txt, ordena y mezcla la secuencia */
            if (is_reg && is_txt_file(new_path)) {
                sequence_t *sort_seq = ordenador(new_path);
                if (!sort_seq) {
                    free(new_path);
                    continue;
                }
                
                /* Actualiza la secuencia */
                *sequence = mezclador(*sequence, sort_seq);
                if (!*sequence) {
                    free(new_path);
                    continue;
                }
            }
        }
        free(new_path);
    }
    closedir(dir);
    return 0;
}