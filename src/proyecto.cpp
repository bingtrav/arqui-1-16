/* Proyecto Arquitectura de Computadoras. */
// http://tuxthink.blogspot.com/2013/01/ussing-barriers-in-pthreads.html
// Se incluye la biclioteca de hilos
#include <pthread.h>

//PC del CPU
int pc;

//Variable que contiene el quantum
int quantum;

// Barrera de control
pthread_barrier_t barrera;
//pthread_barrier_init(&barrera,NULL,3);
    
// Cache de instrucciones que tiene cada CPU
int cache[4][17];
    
//Memoria principal
int memPrin [384];
    
//Registros de CPU
int reg[32];
    
//Estructura de datos para los contextos
int contextos[4][33];

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

void falloCache (int direccion, int seccion) {
    for(int a = 0; a < 16; a++) {
        cache [seccion][a] = memPrin[direccion];
        direccion++;
        // Aqui va la barrera
        //pthread_barrier_wait(&barrera);
    }
}

int main () {

    
    return 0;
}
