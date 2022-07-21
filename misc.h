/**
 * misc.h
 * 
 * Autor: Ka Fung (18-10492)
 * Fecha: 28/07/2022 
 */

#ifndef __MISC_H__
    #define __MISC_H__
    #include <stdint.h>

    int is_dir_file(char *path);
    int is_reg_file(char *path);
    int is_txt_file(char *path);
    int verify_arguments(int argc, char **argv);
#endif