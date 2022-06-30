#!/bin/sh

DIR=/home/utnso/tp-2022-1c-Champagne-SO
if [ -d "$DIR" ]; then
    echo "$DIR existe."
else 
    echo "$DIR no existe."
    git clone https://github.com/sisoputnfrba/tp-2022-1c-Champagne-SO.git
    cd $DIR/Consola && make
    cd $DIR/kernel && make
    cd $DIR/cpu && make
    cd $DIR/memoria && make
    cd $DIR/shared && echo "utnso" | sudo -S make install
fi