/**
 * main.c
 *
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "misc.h"

#define READ_END 0
#define WRITE_END 1
int ord_lec[2];

int lector(int num_ordenadores, int num_mezcladores, char *raiz, char *archivo_salida);

int main(int argc, char **argv) {
    int num_ordenadores, num_mezcladores;
    char *raiz, *archivo_salida;

    /* Verificar argumentos */
    if (verify_arguments(argc, argv) == -1) return -1;

    num_ordenadores = atoi(argv[1]);
    num_mezcladores = atoi(argv[2]);
    raiz = argv[3];
    archivo_salida = argv[4];

    /* Llama al lector */
    lector(num_ordenadores, num_mezcladores, raiz, archivo_salida);

    /* traverse_dir(raiz); */

    return 0;
}

int lector(int num_ordenadores, int num_mezcladores, char *raiz, char *archivo_salida) {
    int i, lec_ord[2];

    /* Creación de un pipe nominal entre lector y ordenadores */
    if (pipe(lec_ord) == -1 || pipe(ord_lec) == -1) {
        fprintf(stderr, "Error al crear el pipe entre lector y ordenadores\n");
        return -1;
    }

    /* Creación de ordenadores */
    for (i = 0; i < num_ordenadores; i++) {
        int pid;

        if ((pid = fork()) == 0) {
            /* Proceso hijo */
            close(lec_ord[WRITE_END]);
            close(ord_lec[READ_END]);

            /* Ejecución de ordenadores */
            for (;;) {
                int file_to_order, r, w;

                /* Lee el archivo que le asignó el Lector */
                r = read(lec_ord[READ_END], &file_to_order, sizeof(int));
                if (!r) break;

                /* Ordenar archivo */
                printf("To-do: Ordenar archivo %d\n", file_to_order);
                sleep(4);
                /* if (order_file(file_to_order) == -1) {
                    fprintf(stderr, "Error al ordenar archivo\n");
                    return -1;
                } */

                /* Avisa al lector que operador i es */
                w = write(ord_lec[WRITE_END], &i, sizeof(int));
                if (w == -1) {
                    fprintf(stderr, "Error al escribir en el pipe\n");
                    return -1;
                }
                /* kill(getppid(), SIGUSR1); */

                /* Pasa secuencia ordenada al mezclador */
                printf("To-do: Pasar secuencia ordenada al mezclador\n");

            }

            /* Cierra los extremos de los pipes */
            close(lec_ord[READ_END]);
            close(ord_lec[WRITE_END]);

            printf("Terminó el ordenador %d\n", i);
            exit(0);
        } else if (pid < 0) {
            /* Error */
            fprintf(stderr, "Error al crear los ordenadores \n");
            return -1;
        }
    }

    /* Como lector, lee el archivo y lo pasa a los ordenadores */
    close(lec_ord[READ_END]);
    close(ord_lec[WRITE_END]);

    /* Recorre el directorio para encontrar los archivos txt y pasarlos a ordenadores */
    for (i = 0; i < 7; i++) {
        /* Abre el archivo txt */
        printf("To-do: Abrir archivo %d\n", i);
        write(lec_ord[WRITE_END], &i, sizeof(int));
        sleep(1);
    }

    close(lec_ord[WRITE_END]);
    close(ord_lec[READ_END]);

    /* Espera a los hijos */
    for (i = 0; i < num_ordenadores; i++) wait(NULL);

    printf("To-do: Mezclar archivos\n");

    return 0;
}
