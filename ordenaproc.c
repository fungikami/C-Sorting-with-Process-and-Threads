/**
 * ordenaproc.c
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include "misc.h"
#include "sequence.h"
#include "pipe_utils.h"
#include "ordenaproc.h"
#include "ordenaproc_child.h"

#define READ_END 0
#define WRITE_END 1

void free_multiple_pipes(int **array_pipe, int size);
void close_multiple_pipes(int **array_pipe, int size, int not_close);
int **initialize_multiple_pipes(int size);
int traverse_dir(char *path, int *ord_lec, int **lec_ord);

/**
 * Función que implementa el proceso Lector. 
 *
 * Parámetros:
 * - raiz: directorio raiz del árbol de directorios a procesar.
 * - num_ord: número de ordenadores.
 * - num_mezc: número de mezcladores.
 * - salida: nombre del archivo de salida.
 *
 * Retorno:
 * - 0 si todo fue bien, -1 si hubo un error.
 */
int lector(char *raiz, int num_ord, int num_mezc, char *salida) {
    int i;
    int ords[2], mezcs[2], lec_esc[2];
    int **lec_ord, **ord_mezc, **mezc_esc;

    /* Crea la pipe de ordenadores disponibles y pipes de lector-ordenador */
    if (pipe(ords) == -1) return -1;
    if (!(lec_ord = initialize_multiple_pipes(num_ord))) return -1;

    /* Crea la pipe de mezcladores disponibles y pipes de ordenador-mezclador */
    if (pipe(mezcs) == -1) return -1;
    if (!(ord_mezc = initialize_multiple_pipes(num_mezc))) return -1;

    /* Crea ordenadores */
    ordenador(num_ord, num_mezc, ords, mezcs, lec_ord, ord_mezc);

    /* Crea pipes de mezclador-escritor y los mezcladores*/
    if (!(mezc_esc = initialize_multiple_pipes(num_mezc))) return -1;
    mezclador(num_ord, num_mezc,ords, mezcs,lec_ord, ord_mezc, mezc_esc);

    /* Crea pipes de lector-escritor y el escritor*/
    if (pipe(lec_esc) == -1) return -1;
    escritor(
        num_ord, num_mezc, ords, mezcs, 
        lec_esc, lec_ord, ord_mezc, mezc_esc, 
        salida
    );

    /* Cierra los extremos a no utilizar de lector-ordenador y lector-escritor*/
    close(ords[WRITE_END]);
    close(mezcs[WRITE_END]);
    close(lec_esc[READ_END]);
    for (i = 0; i < num_ord; i++) close(lec_ord[i][READ_END]); 

    /* Cierra los pipes de ordenador-mezclador y mezclador-escritor */
    close_multiple_pipes(ord_mezc, num_mezc, -1);
    close_multiple_pipes(mezc_esc, num_mezc, -1);

    /* Recorre el directorio raíz y asigna los archivos a ordenadores */
    traverse_dir(raiz, ords, lec_ord);

    /* Cierra los extremos de los pipes utilizados de lector-ordenador */
    for (i = 0; i < num_ord; i++) {
        int ord;
        read(ords[READ_END], &ord, sizeof(int));
        close(lec_ord[ord][WRITE_END]);
    }
    close(ords[READ_END]);

    /* Lee los mezcladores desocupados y cierra el pipe ordenador-mezclador*/
    for (i = 0; i < num_mezc; i++) {
        int mez;

        /* Encola el mezclador al escritor para mandar las secuencias */
        read(mezcs[READ_END], &mez, sizeof(int));
        write(lec_esc[WRITE_END], &mez, sizeof(int));
    }
    close(mezcs[READ_END]);
    close(lec_esc[WRITE_END]);

    /* Espera a que los procesos terminen */
    for (i = 0; i < num_mezc + num_ord + 1; i++) wait(NULL);

    /* Libera memoria asignada a los pipes */
    free_multiple_pipes(lec_ord, num_ord);
    free_multiple_pipes(ord_mezc, num_mezc);
    free_multiple_pipes(mezc_esc, num_mezc);

    return 0;
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
int traverse_dir(char *path, int *ords, int **lec_ord) {
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
                if (traverse_dir(new_path, ords, lec_ord) == -1) {
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
                    int ord, size, aux;

                    /* Lee que ordenador le asignaron */
                    if ((aux = read(ords[READ_END], &ord, sizeof(int))) == -1) {
                        free(new_path);
                        continue;
                    }

                    /* Escribe en la pipe el tamaño y nombre del archivo */
                    size = strlen(new_path) + 1;
                    if (((aux = write(lec_ord[ord][WRITE_END], &size, sizeof(int))) == -1) ||
                        (size != write(lec_ord[ord][WRITE_END], new_path, size))
                    ) {
                        free(new_path);
                        continue;
                    }
                }
            }
        }
        free(new_path);
    }
    closedir(dir);
    return 0;
}