#!/usr/bin/env bash

ip_a_cambiar=$1
DIR=/home/utnso/tp-2022-1c-Champagne-SO/Consola
sed -i "s/IP_KERNEL=127.0.0.1/IP_KERNEL=$ip_a_cambiar/g" $DIR/consola.conf