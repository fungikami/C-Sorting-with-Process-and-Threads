/**
 * ordenahilo.c
 *
 * Implementación de una aplicación que ordena de forma ascendente los enteros
 * almacenados en los archivos ubicados en una jerarquía de directorios, con la
 * creación de hilos cooperantes de un Lector: 
 *      - Ordenadores: Ordenan las secuencias de cada archivo.
 *      - Mezcladores: Mezcla las secuencias ordenadas con otra secuencia ordenada.
 *      - Escritor: Escribe la secuencias mezcladas en un archivo de salida.
 *
 * Comando:
 *      ./ordenahilo <num Ordenadores> <num Mezcladores> <raiz> <archivo salida>
 * donde:
 *      <num Ordenadores>: número de hilos que ordenarán los enteros.
 *      <num Mezcladores>: número de hilos que mezclarán secuencias ordenadas.
 *      <raiz>: es el directorio raíz que se debe procesar.
 *      <archivo salida>: nombre del archivo que almacenará los enteros ordenados.
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

char *output_file, *file_to_sort;
sequence_t *seq_to_merge, **seq_to_write;
sem_t sem_take_file, sem_put_file, sem_ords, sem_sorted_seq;
sem_t sem_take_seq, sem_put_seq, sem_mezc, sem_write;

void *ordenador(void *arg);
void *mezclador(void *arg);
void *escritor(void *arg);
int lector(int num_ord, int num_mezc, char *raiz, char *salida);
int traverse_dir(char *path);

int main(int argc, char *argv[]) {
    /* Verificar argumentos */
    if (verify_arguments(argc, argv) == -1) return -1;

    /* Invoca al lector */
    if (lector(atoi(argv[1]), atoi(argv[2]), argv[3], argv[4]) == -1) return -1;
    
    return 0;
}

/**
 * Función que implementa el lector. Se encarga de:
 * - Crear los hilos restantes (ordenadores, mezcladores y escritor).
 * - Crear los semáforos y mecanismos de comunicación.
 * - Asignar los archivos a un ordenador desocupado
 * - Indicar a los mezcladores y escritores que realicen la mezcla y escritura. 
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
int lector(int num_ord, int num_mezc, char *raiz, char *salida) {
    int i;

    /* Hilos de Ordenadores, Mezcladores y Escritor */
    pthread_t *ord_ids = malloc(num_ord * sizeof(pthread_t));
    pthread_t *mezc_ids = malloc(num_mezc * sizeof(pthread_t));
    pthread_t esc_id;
    int *pmezc = malloc(num_mezc * sizeof(int));
    void *ord_res, *mezc_res, *esc_res;

    if (!ord_ids || !mezc_ids || !pmezc) {
        fprintf(stderr, "Error al reservar memoria para los ids de los hilos.\n");
        return -1;
    }

    /* Inicializa las variables globales */
    output_file = salida;
    seq_to_write = malloc(num_mezc * sizeof(sequence_t *));
    if (!seq_to_write) return -1;

    /* Inicializa los semáforos de lector - ordenador */
    if (sem_init(&sem_take_file, 0, 0) == -1 || 
        sem_init(&sem_put_file, 0, 1) == -1 ||
        sem_init(&sem_ords, 0, 1) == -1
    ) {
        fprintf(stderr, "Error al inicializar los semáforos para lector - ordenador.\n");
        return -1;
    }    
    
    /* Inicializa los semáforos de ordenador - mezclador */
    if (sem_init(&sem_sorted_seq, 0, 1) == -1 ||
        sem_init(&sem_mezc, 0, 1) == -1 ||
        sem_init(&sem_take_seq, 0, 0) == -1 ||
        sem_init(&sem_put_seq, 0, 1) == -1
    ) {
        fprintf(stderr, "Error al inicializar los semáforos para ordenador - mezclador.\n");
        return -1;
    }

    /* Inicializa los semáforos de lector - escritor */
    if (sem_init(&sem_write, 0, 0) == -1) {
        fprintf(stderr, "Error al inicializar los semáforos para lector - escritor.\n");
        return -1;
    }

    /* Creación de ordenadores */
    for (i = 0; i < num_ord; i++) {
        if (pthread_create(&ord_ids[i], NULL, ordenador, NULL)) {
            fprintf(stderr, "Error al crear el hilo ordenador %d.\n", i);
            return -1;
        }
    }

    /* Creación de mezcladores */
    for (i = 0; i < num_mezc; i++) {
        pmezc[i] = i;
        if (pthread_create(&mezc_ids[i], NULL, mezclador, &pmezc[i])) {
            fprintf(stderr, "Error al crear el hilo mezclador %d.\n", i);
            return -1;
        }
    }

    /* Creación del escritor */
    if (pthread_create(&esc_id, NULL, escritor, &num_mezc)) {
        fprintf(stderr, "Error al crear el hilo escritor.\n");
        return -1;
    }

    /* Recorre el árbol de directorio y asigna el archivo a un ordenador */
    traverse_dir(raiz);

    /* Espera que tomen el último archivo, para indicar que no hay más */
    if (sem_wait(&sem_put_file) == -1) return -1;
    file_to_sort = NULL;
    if (sem_post(&sem_take_file) == -1) return -1;

    /* Esperar que los ordenadores asignen sus secuencias a mezcladores */
    for (i = 0; i < num_ord; i++) {
        if (pthread_join(ord_ids[i], &ord_res) != 0) {
            fprintf(stderr, "Error al esperar al hilo ordenador %d.\n", i);
            return -1;
        }
    }

    /* Indica al mezclador que no hay más secuencias ordenadas por mezclar, 
       empieza a pasar las secuencias mezcladas al escritor */
    if (sem_wait(&sem_put_seq) == -1) return -1;
    seq_to_merge = NULL;
    if (sem_post(&sem_take_seq) == -1) return -1;

    /* Esperar que todos los mezcladores terminen de pasar su secuencia */
    for (i = 0; i < num_mezc; i++) {
        if (pthread_join(mezc_ids[i], &mezc_res) != 0) {
            fprintf(stderr, "Error al esperar al hilo mezclador %d.\n", i);
            return -1;
        }
    }

    /* Notifica al escritor que ya puede comenzar a escribir */
    if (sem_post(&sem_write) == -1) return -1;

    /* Espera a que el escritor termine de escribir */
    if (pthread_join(esc_id, &esc_res) != 0) {
        fprintf(stderr, "Error al esperar al hilo escritor.\n");
        return -1;
    }

    /* Cerrar los semáforos */
    sem_destroy(&sem_take_file);     
    sem_destroy(&sem_put_file);      
    sem_destroy(&sem_ords);          
    sem_destroy(&sem_sorted_seq);    
    sem_destroy(&sem_mezc);          
    sem_destroy(&sem_take_seq);      
    sem_destroy(&sem_put_seq);      
    sem_destroy(&sem_write);

    /* Liberar la memoria asignada a los ids de los hilos */
    free(ord_ids);
    free(mezc_ids);
    free(pmezc);
    return 0;
}

/**
 * Función que implementa el ordenador. Se encarga de:
 * - Leer el archivo a ordenar.
 * - Ordenar la secuencia con el algoritmo de selección.
 * - Asigna la secuencia a un mezclador desocupado.
 * - Espera otro archivo a ordenar o terminar.
 *
 * Parámetros:
 * - void *arg: no se utiliza los argumentos.
 */
void *ordenador(void *arg) {
    char *to_sort;
    while (1) {
        sequence_t *seq;

        /* Región crítica: sólo un ordenador puede tomar el archivo a ordenar */
        sem_wait(&sem_ords); 
        
            /* Espera hasta que el lector pase un archivo a ordenar */
            sem_wait(&sem_take_file);

                /* Si ya no hay archivos por ordenar, termina */
                if (!file_to_sort) {
                    sem_post(&sem_take_file);
                    sem_post(&sem_ords);
                    break;
                }

                /* Toma el archivo a ordenar */
                to_sort = file_to_sort;
            
            /* Indica al lector que ya tomó el archivo a ordenar */
            sem_post(&sem_put_file);

        sem_post(&sem_ords);

        /* Extrae la secuencia y libera el nombre del archivo */
        if (!(seq = extract_sequence(to_sort))) {
            free(to_sort);
            continue;
        }
        free(to_sort);

        /* Ordena la secuencia con Selection Sort */
        selection_sort(seq);
        
        /* Región crítica: sólo un ordenador puede pasar la secuencia a un mezclador */
        sem_wait(&sem_sorted_seq);

            /* Espera un mezclador desocupado para asignar su secuencia */
            sem_wait(&sem_put_seq);

                seq_to_merge = seq;

            /* Indica al mezclador desocupado que ya puede tomar la secuencia */
            sem_post(&sem_take_seq);

        sem_post(&sem_sorted_seq);
    }
    
    pthread_exit(NULL);
}

/**
 * Función que implementa el mezclador. Se encarga de:
 * - Inicializa una secuencia vacía, que está ordenada.
 * - Espera que se le asigne una secuencia a mezclar.
 * - Mezcla la secuencia con la que ya tiene ordenada.
 * - Espera hasta que el Lector le indique para enviar su secuencia a escritor.
 *
 * Parámetros:
 * - arg: índice del mezclador en el arreglo de ids de hilos de mezclador.
 */
void *mezclador(void *arg) {
    int i = *(int *) arg;

    /* Inicializa la secuencia a mezclar */
    sequence_t *sequence = create_sequence(0);

    while(1) {
        sequence_t *to_merge, *new_seq;

        /* Región crítica: sólo un mezclador puede tomar la secuencia a mezclar */
        sem_wait(&sem_mezc);

            /* Espera hasta que el ordenador pase una secuencia a mezclar */
            sem_wait(&sem_take_seq);

                /* Si el lector le indica que debe pasar su secuencia al Escritor */
                if (!seq_to_merge) {
                    sem_post(&sem_take_seq);
                    sem_post(&sem_mezc);
                    break;
                }
                
                /* Recibe la secuencia ordenada a mezclar */
                to_merge = seq_to_merge; 

            /* Indica que ya tomó la secuencia a mezclar */          
            sem_post(&sem_put_seq);
            
        sem_post(&sem_mezc);
        
        /* Mezcla la secuencia y libera la memoria asignada a las secuencias */
        if (!(new_seq = merge_sequence(sequence, to_merge))) {
            free(to_merge);
            continue;
        }
        free_sequence(sequence);
        free_sequence(to_merge);
        sequence = new_seq;
    }

    /* Pasa la secuencia al escritor */
    seq_to_write[i] = sequence;
    pthread_exit(NULL);
}

/**
 * Función que implementa el escritor. Se encarga de:
 * - Espera hasta que el Lector le indique para comenzar a escribir.
 * - Recibe una secuencia por cada Mesclador.
 * - Escribe la secuencia final en el archivo de salida, seleccionando 
 *   el menor entero entre las secuencias.
 *
 * Parámetros:
 * - arg: número de mezcladores.
 */
void *escritor(void *arg) {
    int i, num_mezc = *(int *) arg;

    /* Espera hasta que el lector le indique que va a comenzar a escribir */
    sem_wait(&sem_write);
    write_sequence(seq_to_write, num_mezc, output_file);

    /* Libera la memoria asignada a las secuencias mezcladas */
    for (i = 0; i < num_mezc; i++) free_sequence(seq_to_write[i]);
    free(seq_to_write);

    pthread_exit(NULL);
}


/**
 * Función que recorre recursivamente desde un directorio dado,
 * busca los archivos txt para ordenarlos y guardarlos en una secuencia.
 * 
 * Parámetros:
 *   - path: ruta del directorio a recorrer.
 *
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
                /* Esperar que pueda poner otro archivo */
                sem_wait(&sem_put_file);

                file_to_sort = new_path;
                
                /* Permitir que un ordenador pueda tomar el archivo */
                sem_post(&sem_take_file);
            } else {
                free(new_path);
            }
        }
    }
    closedir(dir);
    return 0;
}