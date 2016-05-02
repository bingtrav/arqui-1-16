/* Proyecto Arquitectura de Computadoras. */

// Se incluye la biclioteca de hilos
#include <pthread.h>

//PC del CPU
int pc;
    
// Cache de instrucciones que tiene cada CPU
int cache[4][17];
    
//Memoria principal
int memPrin [384];
    
//Registros de CPU
int reg[32];
    
//Estructura de datos para los contextos
int contextos[4][33];

int buscarBloque(int pc) {
    int bloque = pc/16;
    int palabra = pc%16;
    int seccion = bloque%4;
    
    
    if(cache[seccion][16] == bloque) {
        
    }
}

int main () {

    
    return 0;
}
