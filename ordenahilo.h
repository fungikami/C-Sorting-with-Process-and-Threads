/**
 * misc.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __ORDENAPROC_H__
    #define __ORDENAPROC_H__

    int lector(char *raiz, int num_ord, int num_mez, char *salida);
    void ordenador(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc);
    void mezclador(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc, int **mezc_esc);
    void escritor(int *ord_lec, int **lec_ord, int *mezc_ord, int **ord_mezc, int *lec_esc, int **mezc_esc, char *salida);
#endif