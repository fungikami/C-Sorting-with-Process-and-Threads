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
#include "ordenaproc.h"

#define READ_END 0
#define WRITE_END 1

void ordenador(int pos_ord, int num_ord, int free_ord, int lec_ord, int free_mezc, int **ord_mezc);
void mezclador(int pos_mezc, int fd_free_mezc, int fd_ord_mezc, int fd_mezc_esc);
void escritor(int num_mez, int fd_lec_esc, int **mezc_esc, char *salida);
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
 * - num_mez: número de mezcladores.
 * - salida: nombre del archivo de salida.
 *
 * Retorno:
 * - 0 si todo fue bien, -1 si hubo un error.
 */
int lector(char *raiz, int num_ord, int num_mez, char *salida) {
    int pid, i, j;
    int free_ord[2], free_mezc[2], lec_esc[2];
    int **lec_ord, **ord_mezc, **mezc_esc;

    /* Crea la pipe de ordenadores disponibles y pipes de lector-ordenador */
    if (pipe(free_ord) == -1) return -1;
    if (!(lec_ord = initialize_multiple_pipes(num_ord))) return -1;

    /* Crea la pipe de mezcladores disponibles y pipes de ordenador-mezclador */
    if (pipe(free_mezc) == -1) return -1;
    if (!(ord_mezc = initialize_multiple_pipes(num_mez))) return -1;

    /* Crea ordenadores */
    for (i = 0; i < num_ord; i++) {
        if ((pid = fork()) == 0) {
            /* Cierra los extremos a no utilizar de lector-ordenador y ordenador-mezclador */
            close(free_ord[READ_END]);
            close(free_mezc[WRITE_END]);
            close(lec_ord[i][WRITE_END]);
            for (j = 0; j < num_mez; j++) close(ord_mezc[j][READ_END]);

            /* Cierra extremos de otras pipes de lector-ordenador */
            close_multiple_pipes(lec_ord, num_ord, i);

            ordenador(i, num_ord, free_ord[WRITE_END], lec_ord[i][READ_END], free_mezc[READ_END], ord_mezc);

            /* Cierra los extremos de los pipes utilizados de ordenador-mezclador */
            close(free_mezc[READ_END]);
            for (j = 0; j < num_mez; j++) close(ord_mezc[j][WRITE_END]);
            exit(0);
        } else if (pid < 0) {
            perror("Error al crear proceso de ordenador\n");
            exit(1);
        }
    }

    /* Crea pipes de mezclador-escritor */
    if (!(mezc_esc = initialize_multiple_pipes(num_mez))) return -1;

    /* Crea los mezcladores*/
    for (i = 0; i < num_mez; i++) {
        if ((pid = fork()) == 0) {
            /* Cierra los pipes de lector-ordenador */
            close(free_ord[READ_END]);
            close(free_ord[WRITE_END]);
            close_multiple_pipes(lec_ord, num_ord, -1);

            /* Cierra los extremos que no se utilizarán */
            close(free_mezc[READ_END]);
            close(ord_mezc[i][WRITE_END]);
            close(mezc_esc[i][READ_END]);
            close_multiple_pipes(ord_mezc, num_mez, i);
            close_multiple_pipes(mezc_esc, num_mez, i);

            mezclador(i, free_mezc[WRITE_END], ord_mezc[i][READ_END], mezc_esc[i][WRITE_END]);

            exit(0);
        } else if (pid < 0) {
            perror("Error al crear proceso de mezclador\n");
            exit(1);
        }
    }

    /* Crea pipes de lector-escritor y el proceso escritor*/
    if (pipe(lec_esc) == -1) return -1;

    /* Crear escritor*/
    if ((pid = fork()) == 0) {
        /* Cierra los pipes de lector-ordenador */
        close(free_ord[READ_END]);
        close(free_ord[WRITE_END]);
        close_multiple_pipes(lec_ord, num_ord, -1);

        /* Cierra los pipes de mezclador-ordenador */
        close(free_mezc[READ_END]);
        close(free_mezc[WRITE_END]);
        close_multiple_pipes(ord_mezc, num_mez, -1);

        /* Cierra los extremos que no se utilizarán de mezclador-escritor */
        close(lec_esc[WRITE_END]);
        for (j=0; j<num_mez; j++) close(mezc_esc[j][WRITE_END]);

        escritor(num_mez, lec_esc[READ_END], mezc_esc, salida);

        exit(0);
    } else if (pid < 0) {
        perror("Error al crear proceso de escritor\n");
        exit(1);
    }

    /* Cierra los extremos a no utilizar de lector-ordenador y lector-escritor*/
    close(free_ord[WRITE_END]);
    close(lec_esc[READ_END]);
    for (i=0; i<num_ord; i++) close(lec_ord[i][READ_END]);

    /* Cierra los extremos ordenador-mezclador */
    close_multiple_pipes(ord_mezc, num_mez, -1);

    /* Recorre el directorio raíz y asigna los archivos a ordenadores */
    traverse_dir(raiz, free_ord, lec_ord);

    /* Cierra los extremos de los pipes utilizados de lector-ordenador */
    for (i = 0; i < num_ord; i++) close(lec_ord[i][WRITE_END]);
    for (i = 0; i < num_ord; i++) {
        int ord;
        read(free_ord[READ_END], &ord, sizeof(int));
        close(lec_ord[ord][READ_END]);
    }
    close(free_ord[READ_END]);

    /* Lee los mezcladores desocupados y cierra el pipe ordenador-mezclador*/
    for (i = 0; i < num_mez; i++) {
        int mez;
        read(free_mezc[READ_END], &mez, sizeof(int));
        close(ord_mezc[mez][READ_END]);

        /* Encola el mezclador al escritor para mandar las secuencias */
        write(lec_esc[WRITE_END], &mez, sizeof(int));
    }
    close(free_mezc[READ_END]);
    close(lec_esc[WRITE_END]);

    for (i = 0; i < num_mez + num_ord + 1; i++) wait(NULL);

    /* Libera memoria asignada a los pipes */
    free_multiple_pipes(lec_ord, num_ord);
    free_multiple_pipes(ord_mezc, num_mez);
    free_multiple_pipes(mezc_esc, num_mez);

    return 0;
}

void ordenador(int pos_ord, int num_ord, int free_ord, int lec_ord, int free_mezc, int **ord_mezc) {
    for (;;) {
        char *filename;
        int i, aux, size, mezc;
        sequence_t *sequence;

        /* Encola el ordenador como disponible */
        if ((aux = write(free_ord, &pos_ord, sizeof(int))) == -1) continue;

        /* Lee el tamaño del filename asignado */
        aux = read(lec_ord, &size, sizeof(int));
        if (aux == -1) continue;
        if (aux == 0) break;
        
        /* Guarda el filename */
        if ((filename = malloc(size * sizeof(char))) == NULL) continue;
        if ((aux = read(lec_ord, filename, size * sizeof(char))) == -1) {
            free(filename);
            continue;
        }

        /* Extrae y ordena los números del archivo */
        if (!(sequence = extract_sequence(filename))) {
            free(filename);
            continue;
        }
        selection_sort(sequence);
        free(filename);

        /* Toma un mezclador disponible */
        aux = read(free_mezc, &mezc, sizeof(int));

        /* Encola el tamaño y la secuencia */
        size = sequence->size;
        write(ord_mezc[mezc][WRITE_END], &size, sizeof(int));
        for (i = 0; i < size; i++) {
            write(ord_mezc[mezc][WRITE_END], &sequence->data[i], sizeof(int64_t));
        }

        /* Libera la memoria de la secuencia */
        free_sequence(sequence);
    }
}

void mezclador(int pos_mezc, int fd_free_mezc, int fd_ord_mezc, int fd_mezc_esc) {
    int i, aux, size;
    sequence_t *sequence = create_sequence(0);

    for (;;) {
        sequence_t *ord_seq, *mez_seq;

        /* Encolar mezclador como disponible */
        if ((aux = write(fd_free_mezc, &pos_mezc, sizeof(int))) == -1) continue;

        /* Lee el tamaño de la secuencia */
        aux = read(fd_ord_mezc, &size, sizeof(int));
        if (aux == -1) continue;
        if (aux == 0) break;

        /* Lee los números de la secuencia */
        if (!(ord_seq = create_sequence(size))) continue;
        for (i = 0; i < size; i++) {
            int64_t m;
            if ((aux = read(fd_ord_mezc, &m, sizeof(int64_t))) == -1) {
                free_sequence(ord_seq);
                continue;
            }
            ord_seq->data[i] = m;
        }

        /* Mezcla la secuencia con la secuencia ordenada */
        if (!(mez_seq = merge_sequence(sequence, ord_seq))) {
            free_sequence(ord_seq);
            continue;
        }

        free_sequence(sequence);
        free_sequence(ord_seq);
        sequence = mez_seq;
    }

    close(fd_free_mezc);

    /* Pasa la secuencia al escritor */
    size = sequence->size;
    write(fd_mezc_esc, &size, sizeof(int));
    for (i = 0; i < size; i++) {
        write(fd_mezc_esc, &sequence->data[i], sizeof(int64_t));
    }
}

void escritor(int num_mez, int fd_lec_esc, int **mezc_esc, char *salida) {
    int aux, i, n = 0;
    /* Arreglo de las secuencias de cada mezclador */
    sequence_t **secuencias = malloc(num_mez * sizeof(sequence_t *));

    for (;;) {
        int mezc, size;
        if (n == num_mez) break;

        /* Lee el mezclador asignado */
        if ((aux = read(fd_lec_esc, &mezc, sizeof(int))) == -1) continue;

        /* Lee el tamaño de la secuencia */
        if ((aux = read(mezc_esc[mezc][READ_END], &size, sizeof(int))) == -1) continue;
        secuencias[mezc] = create_sequence(size);

        /* Lee la secuencia */
        for (i = 0; i < size; i++) {
            if ((aux = read(mezc_esc[mezc][READ_END], &secuencias[mezc]->data[i], sizeof(int64_t))) == -1) 
                continue;
        }

        n++;
    }

    /* Escribe ordenado salida */
    write_sequence(secuencias, num_mez, salida);

    /* Libera memoria */
    for (i = 0; i < num_mez; i++) free_sequence(secuencias[i]);
    free(secuencias);
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
                    m = strlen(new_path) + 1;
                    write(lec_ord[n][WRITE_END], &m, sizeof(int));
                    write(lec_ord[n][WRITE_END], new_path, m);
                    
                }
            }
        }
        free(new_path);
    }
    closedir(dir);
    return 0;
}

/**
 * Función que inicializa un arreglo de pipes.
 * Parámetros:
 *      size: número de pipes a inicializar.
 * Retorno:
 *      Arreglo de pipes inicializado.
 */
int **initialize_multiple_pipes(int size) {
    int i, **array_pipe = (int **) malloc(sizeof(int *) * size);
    for (i = 0; i < size; i++) {
        array_pipe[i] = (int *) malloc(sizeof(int) * 2);
        if (!array_pipe[i]) return NULL;

        if (pipe(array_pipe[i]) == -1) return NULL;
    }
    return array_pipe;
}

/**
 * Función que cierra los extremos de un arreglo de pipes.
 * Parámetros:
 *      array_pipe: arreglo de pipes a cerrar.
 *      size: número de pipes a cerrar. 
 *      not_close: indice de una pipe que no se debe cerrar una pipe.
 */
void close_multiple_pipes(int **array_pipe, int size, int not_close) {
    int i;
    for (i = 0; i < size; i++) {
        if (i != not_close) {
            close(array_pipe[i][READ_END]);
            close(array_pipe[i][WRITE_END]);
        }
    }
}
/**
 * Función que libera la memoria asignada a un arreglo de pipes.
 * Parámetros:
 *      array_pipe: arreglo de pipes a liberar.
 *      size: número de pipes a liberar. 
 */
void free_multiple_pipes(int **array_pipe, int size) {
    int i;
    for (i = 0; i < size; i++) free(array_pipe[i]);
    free(array_pipe);
}