#!/usr/bin/env bash

ip_a_cambiar=$1
DIR=/home/utnso/tp-2022-1c-Champagne-SO/pruebas
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/prueba_base/cpu.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/prueba_base/memoria.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/prueba_base/kernel.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_TLB/cpu.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_TLB/memoria.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_TLB/kernel.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_memoria/cpu.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_memoria/memoria.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_memoria/kernel.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_integrales/cpu.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_integrales/memoria.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_integrales/kernel.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_planificacion/cpu.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_planificacion/memoria.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_planificacion/kernel.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_suspension/cpu.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_suspension/memoria.conf
sed -i "s/IP_LOCAL=127.0.0.1/IP_LOCAL=$ip_a_cambiar/g" $DIR/pruebas_suspension/kernel.conf