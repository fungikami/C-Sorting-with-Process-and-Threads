#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "misc.h"
#include "ordenaproc.h"

int main(int argc, char *argv[]) {
    int num_ord, num_mezc;
    char *raiz, *salida;

    /* Verifica los argumentos */
    if (verify_arguments(argc, argv) == -1) return -1;

    num_ord = atoi(argv[1]);
    num_mezc = atoi(argv[2]);
    raiz = argv[3];
    salida = argv[4];

    /* Invoca al lector */
    lector(raiz, num_ord, num_mezc, salida);

    return 0;
}
