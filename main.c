#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "misc.h"
#include "ordenaproc.h"


int main(int argc, char *argv[]) {
    int num_ord, num_mez;
    char *raiz, *salida;

    /* Verifica los argumentos */
    if (verify_arguments(argc, argv) == -1) return -1;

    num_ord = atoi(argv[1]);
    num_mez = atoi(argv[2]);
    raiz = argv[3];
    salida = argv[4];

    printf("Directorio: %s\n", argv[3]);

    /* Invoca al lector */
    lector(raiz, num_ord, num_mez, salida);

    return 0;
}
