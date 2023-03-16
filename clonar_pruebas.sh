#!/bin/sh

DIR=/home/utnso/kiss-pruebas
if [ -d "$DIR" ]; then
    echo "$DIR existe."
    cp $DIR/* ./Consola/bin
    echo "Pruebas copiadas"
else 
    echo "$DIR no existe."
    git clone https://github.com/sisoputnfrba/kiss-pruebas.git
    cp $DIR/* ./Consola/bin
    echo "Pruebas copiadas"
    
fi
