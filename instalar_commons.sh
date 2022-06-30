#!/bin/sh

DIR=/home/utnso/so-commons-library
if [ -d "$DIR" ]; then
    echo "$DIR existe."
else 
    echo "$DIR no existe."
    git clone https://github.com/sisoputnfrba/so-commons-library.git
    cd $DIR
    echo "utnso" | sudo -S make install
    sudo /sbin/ldconfig -v
fi