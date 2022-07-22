/**
 * ordenaproc_child.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __ORDENAPROC_CHILD_H__
    #define __ORDENAPROC_CHILD_H__

    void ordenador(int num_ord, int num_mezc, int *ords, int *mezcs, int **lec_ord, int **ord_mezc);
    void mezclador(int num_ord, int num_mezc, int *ords, int *mezcs, int **lec_ord, int **ord_mezc, int **mezc_esc);
    void escritor(int num_ord, int num_mezc, int *ords, int *mezcs, int *lec_esc, int **lec_ord, int **ord_mezc, int **mezc_esc, char *salida);
#endif