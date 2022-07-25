#! /usr/bin/bash

if [ $# -ne 3 ]
then
    echo "Uso: $0 <n_archivos> <n_numeros> <dirname>"
    exit -1
fi

# Crea directorio de pruebas
rm -rf $3

for (( k = $(( $1 - 1 )) ; k >= 0 ; k-- ))
do
    mkdir $3
    cd $3

    for (( i = 0 ; i <= k ; i++ ))
    do
        file="test$i.txt"
        > $file

        for (( j = 0 ; j < $2 ; j++ ))
        do
            randint=$(( $RANDOM - 16384 ))
            if [ $j -eq 0 ]
            then
                printf %d "$randint" >> $file
            else
                printf "\n$randint" >> $file
            fi
        done
    done
done