/**
 * ordenahilo.c
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include "misc.h"
#include "sequence.h"

void *ordenador(void *arg);
void *mezclador(void *arg);
void *escritor(void *arg);
void lector(char *raiz, int num_ord, int num_mezc, char *salida);
int traverse_dir(char *path);

/* */
typedef struct Thread {
    pthread_t id;
    pthread_cond_t cond;
    int status;


} thread_t;

int main(int argc, char *argv[]) {
    lector(argv[1], atoi(argv[2]), atoi(argv[3]), argv[4]);
    return 0;
}


void lector(char *raiz, int num_ord, int num_mezc, char *salida) {
    thread_t **ord_threads = malloc(num_ord * sizeof(thread_t *));
    thread_t **mezc_threads = malloc(num_mezc * sizeof(thread_t *));
    thread_t *esc_thread = malloc(sizeof(thread_t));

    /* Inicializa los ordenadores */
    for (int i = 0; i < num_ord; i++) {
        ord_threads[i] = malloc(sizeof(thread_t));
        if (pthread_create(&ord_threads[i]->id, NULL, ordenador, NULL)) {
            printf("Error al crear el ordenador %d\n", i);
            exit(1);
        }
        pthread_cond_t 
    }
}

void *ordenador(void *arg) {
    
    pthread_exit(NULL);
}

void *mezclador(void *arg) {
}

void *escritor(void *arg) {
    printf("Hilo escritor\n");
    sem_wait(&start_write_seq);

    int num_mezc = *(int *) arg[0];
    char *salida = (char *) arg[1];

    write_sequence(seq_to_write, num_mezc, salida);
    return arg;
}


/**
 * Función que recorre recursivamente desde un directorio dado,
 * busca los archivos txt para ordenarlos y guardarlos en una secuencia.
 * 
 * Parámetros:
 *   - path: ruta del directorio a recorrer.
 *   - sequence: Apuntador a la secuencia ordenada.
 * Retorno:
 *      0 si todo fue correcto, -1 si hubo un error durante la ejecución.
 */
int traverse_dir(char *path) {
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
            if (traverse_dir(new_path) == -1) {
                free(new_path);
                continue;
            }
            free(new_path);
        } else {
            /* Si es un archivo regular, se revisa si es txt */
            int is_reg = is_reg_file(new_path);
            if (is_reg == -1) {
                free(new_path);
                continue;
            }

            /* Si es un archivo txt, ordena y mezcla la secuencia */
            if (is_reg && is_txt_file(new_path)) {
                
            } else {
                free(new_path);
            }
        }
    }
    closedir(dir);
    return 0;
}