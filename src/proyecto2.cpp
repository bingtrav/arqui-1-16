/* Proyecto Arquitectura de Computadoras. */
// http://tuxthink.blogspot.com/2013/01/ussing-barriers-in-pthreads.html
// http://www.bogotobogo.com/cplusplus/multithreading_pthread.php
// Se incluye la biclioteca de hilos
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define numHilos 3

/*struct datosCpu () {
    int reg[32];
    int cache[4][17];
    int contextos[4][33];
};*/

//PC del CPU
int pc;
//int pc[3];

//Variable que contiene el quantum
int quantum;
//int quantum[3];

// Barrera de control
pthread_barrier_t barrera;
//

// Cache de instrucciones que tiene cada CPU
int cache[4][17];
//int cache[3][4][17];

//Memoria principal
int memPrin[384];
//int memPrin[3][384];


//Registros de CPU
int reg[32];
//int reg[3][32];

//Estructura de datos para los contextos
int contextos[4][33];
//int contextos[3][4][33];

void falloCache (int direccion, int seccion) {
    for(int a = 0; a < 16; a++) {
        cache [seccion][a] = memPrin[direccion];
        direccion++;
         pthread_barrier_wait(&barrera);
    }
}

int* buscarBloque(int pc) {
    int bloque = pc/16;

    int palabra = pc%16;
    int seccion = bloque%4;
    int plb[4];
    int* plbPtr = plb;

    if(cache[seccion][16] != bloque) {
        int dirFisica = bloque * 16;
        falloCache(dirFisica, seccion);
    }

    for(int a = 0; a < 4; a++) {
        plb[a] = cache[seccion][palabra];
        palabra++;
    }
    return plbPtr;
}

/*
void falloCache (int direccion, int seccion, int id) {
    for(int a = 0; a < 16; a++) {
        cache [id][seccion][a] = memPrin[id][direccion];
        direccion++;
         pthread_barrier_wait(&barrera);
    }
}

int* buscarBloque(int pc, int id) {
    int bloque = pc[id]/16;

    int palabra = pc[id]%16;
    int seccion = bloque%4;
    int plb[4];
    int* plbPtr = plb;

    if(cache[id][seccion][16] != bloque) {
        int dirFisica = bloque * 16;
        falloCache(dirFisica, seccion, id);
    }

    for(int a = 0; a < 4; a++) {
        plb[a] = cache[id][seccion][palabra];
        palabra++;
    }
    return plbPtr;
}*/

void procesarPalabra(int* palabra) {
    switch ( palabra[0] )
      {
         case 8:{
            //DADDI; RX, RY, #n; Rx <-- (Ry) + n
            break;
         }
         case 32:{
            // DADD; RX, RY, RZ; Rx <-- (Ry) + (Rz)
            break;
         }
         case 34:{
            // DSUB; RX, RY, RZ; Rx <-- (Ry) - (Rz)
            break;
         }
         case 12:{
            // DMUL; RX, RY, RZ; Rx <-- (Ry) * (Rz)
            break;
         }
         case 14:{
            // DDIV; RX, RY, RZ; Rx <-- (Ry) / (Rz)
            break;
         }
         case 4:{
            // BEQZ; RX, ETIQ; Si Rx = 0 SALTA
            break;
         }
         case 5:{
            //BNEZ; RX, ETIQ; Si Rx <> 0 SALTA
            break;
         }
         case 3:{
            // JAL; n; R31<--PC, PC<-- PC+n
            break;
         }
         case 2:{
            // JR; RX; PC <-- (Rx)
            break;
         }
         case 63:{
            // FIN Detiene el programa
            break;
         }
      }
      pthread_barrier_wait(&barrera);
}

void *CPU(void *param)
{
    int id = (int)param;
    int plb[4];

    //ver como se crean las estructuras internas de cada procesador


    if(id == 0){
        //cargar los hilos a cada procesador a la memoria principal y a los contentextos
        pthread_barrier_wait(&barrera);
    }
    //cargar pc con dato del hilo, se busca en el contexto

    //cargar palabra en CPU para procesar.
    // plb[4] = buscarBloque(pc);

    // procesar palabra

    //repetir


}

int main (int argc, char** argv) {
    pthread_t hilo1, hilo2, hilo3;
    int ret;
    pthread_barrier_init(&barrera,NULL,3);
    ret =  pthread_create(&hilo1, NULL, &CPU, (void*)0);
    ret =  pthread_create(&hilo2, NULL, &CPU, (void*)1);
    ret =  pthread_create(&hilo3, NULL, &CPU, (void*)2);
    pthread_join(hilo1, 0);
    pthread_join(hilo2, 0);
    pthread_join(hilo3, 0);
    pthread_exit(NULL);
    return 0;
}
