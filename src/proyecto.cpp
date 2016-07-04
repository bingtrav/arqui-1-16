/* Proyecto Arquitectura de Computadoras. */
// Compilar con pthreads: g++ -pthread  proyecto.cpp -o proyecto
// mutex try lock
// http://en.cppreference.com/w/cpp/thread/mutex/try_lock

#include <pthread.h>                    // Se incluye la biclioteca de hilos
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <dirent.h>                                                            
#include <string.h>
#include <vector>                  // Manejo de zonas criticas.

using namespace std;

// Constante con el número de hilos = número de CPU's
#define numHilos 3

// Número de hilos que se manejarán en lso 3 CPU's
int subHilos;

// Variable que contiene los ciclos de los CPU's
int ciclo;

//PC del CPU
int pc1;
int pc2;
int pc3;

bool cpu1 = true;
bool cpu2 = true;
bool cpu3 = true;

//Contexto actual del CPU y el estado que tendrá luego del cambio de contexto.
int cntxActual1, estado1;
int cntxActual2, estado2;
int cntxActual3, estado3;

//Variable que contiene el quantum
int quantum;

// Contadores para quantum.
int quantum1; //quantum del CPU1
int quantum2; //quantum del CPU2
int quantum3; //quantum del CPU3

// Barrera de control
pthread_barrier_t barrier;

// Cache de instrucciones que tiene cada CPU
int cache1[4][17];
int cache2[4][17];
int cache3[4][17];

// Cache de datos que tiene cada CPU
/*
    Cache de Datos
        ----------------------------------------------------------------
        |  datos  | numero de bloque | estado 0 = "C", 1 = "M", 2 = "I"|     
        ----------------------------------------------------------------
        | 0 ... 3 |         4        |                  5              |    
        ----------------------------------------------------------------
*/

int cacheDatos1[4][6];
int cacheDatos2[4][6];
int cacheDatos3[4][6];

// Mutex para el uso de caches
pthread_mutex_t  mCacheDatos1;
pthread_mutex_t  mCacheDatos2;
pthread_mutex_t  mCacheDatos3;

// booleano que simula semaforo para el ingreso a cache true = cache libre, false = cache ocupado
bool cacheDisponible[3];

// Directorio que maneja informacion y estado de los Bloques que tiene cada CPU
/*
    Cache de Datos
        ----------------------------------------------------------------
        |  estado 0 = "C", 1 = "M", 2 = "U"  | CPUS   | 
        ----------------------------------------------------------------
        |                 0                  |  1..3  |    
        ----------------------------------------------------------------
*/
int directorio1[8][4];
int directorio2[8][4];
int directorio3[8][4];

// Mutex para el uso de directorios
pthread_mutex_t  mDirectorio1;
pthread_mutex_t  mDirectorio2;
pthread_mutex_t  mDirectorio3;

// booleano que simula semaforo para el ingreso a Directorio true = directoro libre, false = directorio ocupado
bool DirDisponible[3];

/*
    Memoria principal
        --------------------------------
        |  0  ... 127 | Mem Compartida |
        | 128 ... 383 | Mem Compartida |
        --------------------------------
*/
int memPrin1[384];
int memPrin2[384];
int memPrin3[384];

/*
    Memoria Compartida de Datos
        -----------------------------
        | 0 ... 32 | Mem Compartida |
        -----------------------------
*/
int memDatos1[32];
int memDatos2[32];
int memDatos3[32];


//Registros de CPU
int reg1[32];
int reg2[32];
int reg3[32];

// Registros RL de cada CPU
int RL1;
int RL2;
int RL3;

pthread_mutex_t  mRL1;
pthread_mutex_t  mRL2;
pthread_mutex_t  mRL3;

//Estructura de datos para los contextos
/*
    Estructura:
        -------------------------------------------------------------------------------
        |0=pc|1-32=registros|33= EP/F |34= Ciclo-Inicio|35= Ciclo-Final|36= id-subHilo|
        -------------------------------------------------------------------------------
        | PC |R1=0,...,R32=4|  true   |  inicio-ciclo  |   fin-ciclo   |  id-subHilo  |
        -------------------------------------------------------------------------------
*/
int contextos1[4][37];
int contextos2[4][37];
int contextos3[4][37];

//Llena las memorias de datos con unos como valor determinado
void llenaMemDatos() {
    for(int a = 0; a < 32; a++) {
        memDatos1[a] = 1;
        memDatos2[a] = 1;
        memDatos3[a] = 1;
    }
}

// Método que imprime en pantalla el estado de la memoria principal de cada CPU
void imprimirMemP() {
    cout << "Mem 1: ";
    cout << endl;
    for(int i= 128; i < 383; i++){
        cout << memPrin1[i] << " - ";
    }
    cout << endl;
    cout << "Mem 2: ";
    cout << endl;
    for(int i= 128; i < 383; i++){
        cout << memPrin2[i] << " - ";
    }
    cout << endl;
    cout << "Mem 3: ";
    cout << endl;
    for(int i= 128; i < 383; i++){
        cout << memPrin3[i] << " - ";
    }
        
}

// Método que imprime en pantalla el estado de los contextos de cada CPU
void imprimircontextos(){
    cout << endl;
    cout << "Contexto 1: ";
    cout << endl;
    for(int i = 0; i < 4; i++){
        cout << endl;
        for(int c = 0; c < 37; c++){
            cout << contextos1[i][c] << " - "; 
        }
    }
    
    cout << endl;
    cout << "Contexto 2: ";
    cout << endl;
    for(int i = 0; i < 4; i++){
        cout << endl;
        for(int c = 0; c < 37; c++){
            cout << contextos2[i][c] << " - "; 
        }
    }
    
    cout << endl;
    cout << "Contexto 3: ";
    cout << endl;
    for(int i = 0; i < 4; i++){
        cout << endl;
        for(int c = 0; c < 37; c++){
            cout << contextos3[i][c] << " - "; 
        }
    }
}

// Método que imprime en pantalla el estado de los registros de cada CPU
void imprimirInfoHilo() {
    cout << endl << "Cpu1: ";
    cout << endl;
    for(int i= 0; i < 4; i++){
        if(contextos1[i][34] != 0) {
            cout << endl << "Registros del Hilo: " << contextos1[i][36];
            cout << endl;
            for(int j = 1; j < 33; j++) {
                cout << contextos1[i][j]<< " - ";
            }
            cout << endl << "Cantidad de ciclos del hilo: " << contextos1[i][35] - contextos1[i][34];
            cout << endl << "Ciclo inicial del hilo: " << contextos1[i][34];
            cout << endl << "Ciclo final del Hilo: " << contextos1[i][35];
            cout << endl;
        }
    }
    
    cout << endl << "Cpu2: ";
    cout << endl;
    for(int i= 0; i < 4; i++){
        if(contextos3[i][34] != 0) {
            cout << endl << "Registros del Hilo: " << contextos2[i][36];
            cout << endl;
            for(int j = 1; j < 33; j++) {
                cout << contextos2[i][j]<< " - ";
            }
            cout << endl << "Cantidad de ciclos del hilo: " << contextos2[i][35] - contextos2[i][34];
            cout << endl << "Ciclo inicial del hilo: " << contextos2[i][34];
            cout << endl << "Ciclo final del Hilo: " << contextos2[i][35];
            cout << endl;
        }
    }
    
    cout << endl << "Cpu3: ";
    cout << endl;
    for(int i= 0; i < 4; i++){
        if(contextos3[i][34] != 0) {
            cout << endl << "Registros del Hilo: " << contextos3[i][36];
            cout << endl;
            for(int j = 1; j < 33; j++) {
                cout << contextos3[i][j]<< " - ";
            }
            cout << endl << "Cantidad de ciclos del hilo: " << contextos3[i][35] - contextos3[i][34];
            cout << endl << "Ciclo inicial del hilo: " << contextos3[i][34];
            cout << endl << "Ciclo final del Hilo: " << contextos3[i][35];
            cout << endl;
        }
    }
        
}

/* 
    Este metodo se encarga de realizar el cambio de contexto segun el CPU que lo solicite. 
    En caso de que el contecto actual este inactivo, o sea, contextos[id_hilo][33] == 0, 
    solo hace cambios de ciclo sin cambiar registros.
*/
void cambioContexto(int id_hilo) {
    switch(id_hilo) {
        case 1: // Cambio de contexto del CPU1
            for(int i=1; i<33; i++) { // Llena la tabla de contextos con los registros del hilo en ejecución
                contextos1[cntxActual1][i] = reg1[i-1];
            }
            contextos1[cntxActual1][0] = pc1;
            contextos1[cntxActual1][33] = estado1;
            quantum1 = quantum;
            if(estado1==0){ // Si el hilo ya terminó, pone en la tabla de contextos el ciclo donde este terminó.
                contextos1[cntxActual1][35] = ciclo;
                estado1=1;
                
            }
            for(int i=0; i<4; i++) { 
                if((contextos1[i][33] == 1) && (i != cntxActual1)){ // pasa por todos los contextos para revisar cual no ha termindado, verifica que no sea igual al contexto actual si hay mas de un hilo corriendo  
                    pc1 = contextos1[i][0];
                    cntxActual1 = i;
                    if(contextos1[i][34] == 0) { // Si el hilo que se va a traer a registros nunca se ha ejecutado, le marca el ciclo donde inicia
                        contextos1[i][34] = ciclo;
                    }
                    for(int x=1; x<33; x++) {
                        reg1[x-1] = contextos1[i][x]; // Se trae registros los datos del suiguiente hilo
                    }
                    i = 4;
                }
            }
            break;
        case 2:
            for(int i=1; i<33; i++) { // Llena la tabla de contextos con los registros del hilo en ejecución
                contextos2[cntxActual2][i] = reg2[i-1];
            }
            contextos2[cntxActual2][0] = pc2;
            contextos2[cntxActual2][33] = estado2;
            quantum2 = quantum;
            if(estado2==0){ // Si el hilo ya terminó, pone en la tabla de contextos el ciclo donde este terminó.
                contextos2[cntxActual2][35] = ciclo;
            }
            for(int i=0; i<4; i++) { 
                if((contextos2[i][33] == 1) && (i != cntxActual2)){ // pasa por todos los contextos para revisar cual no ha termindado, verifica que no sea igual al contexto actual si hay mas de un hilo corriendo  
                    pc2 = contextos2[i][0];
                    cntxActual2 = i;
                    if(contextos2[i][34] == 0) { // Si el hilo que se va a traer a registros nunca se ha ejecutado, le marca el ciclo donde inicia
                        contextos2[i][34] = ciclo;
                    }
                    for(int x=1; x<33; x++) {
                        reg2[x-1] = contextos2[i][x]; // Se trae registros los datos del suiguiente hilo
                    }
                    i = 4;
                }
            }
            break;
            
        case 3:
            for(int i=1; i<33; i++) { // Llena la tabla de contextos con los registros del hilo en ejecución
                contextos3[cntxActual3][i] = reg3[i-1];
            }
            contextos3[cntxActual3][0] = pc3;
            contextos3[cntxActual3][33] = estado3;
            quantum3 = quantum;
            if(estado3==0){ // Si el hilo ya terminó, pone en la tabla de contextos el ciclo donde este terminó.
                contextos3[cntxActual3][35] = ciclo;
            }
            for(int i=0; i<4; i++) { 
                if((contextos3[i][33] == 1) && (i != cntxActual3)){ // pasa por todos los contextos para revisar cual no ha termindado, verifica que no sea igual al contexto actual si hay mas de un hilo corriendo  
                    pc3 = contextos3[i][0];
                    cntxActual3 = i;
                    if(contextos3[i][34] == 0) { // Si el hilo que se va a traer a registros nunca se ha ejecutado, le marca el ciclo donde inicia
                        contextos3[i][34] = ciclo;
                    }
                    for(int x=1; x<33; x++) {
                        reg3[x-1] = contextos3[i][x]; // Se trae registros los datos del suiguiente hilo
                    }
                    i = 4;
                }
            }
            break;
    }
}

/* Cuenta cuantas instrucciones ha hecho para cambio de contexto*/
void cuentaIns(int id_hilo) {
    switch(id_hilo) {
        case 1:
            if(estado1==0 || quantum1==0) {
                cambioContexto(id_hilo); // si el quantum de CPU1 llega a 0 el estado del hilo es 0 (ya finalizado) hace cambio de contexto
            } else {
                quantum1--; // De lo contrario simplemente resta el quantum
            }
            break;
        case 2:
            if(estado2==0 || quantum2==0) {
                cambioContexto(id_hilo); // si el quantum de CPU2 llega a 0 el estado del hilo es 0 (ya finalizado) hace cambio de contexto
            } else {
                quantum2--; // De lo contrario simplemente resta el quantu
            }
            break;
        case 3:
            if(estado3==0 || quantum3==0) {
                cambioContexto(id_hilo); // si el quantum de CPU3 llega a 0 el estado del hilo es 0 (ya finalizado) hace cambio de contexto
            } else {
                quantum3--; // De lo contrario simplemente resta el quantu
            } 
            break;
    }
}

// Método que trae al caché del respectivo CPU los bloques que den un fallo de caché
void falloCache(int id_bloque, int direccion, int seccion, int id_hilo) {
    switch (id_hilo)
      {
         case 1:{
            for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                cache1[seccion][a] = memPrin1[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
            }
            cache1[seccion][16] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
            cuentaIns(id_hilo); // Baja el cuantum y si es igual a 0 cambia de contexto 
            break;
         }
         case 2:{
            for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                cache2[seccion][a] = memPrin2[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal  
                direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
            }
            cache2[seccion][16] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache
            cuentaIns(id_hilo); // Baja el cuantum y si es igual a 0 cambia de contexto
            break;
         }
         case 3:{
            for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                cache3[seccion][a] = memPrin3[direccion];
                direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
            }
            cache3[seccion][16] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache
            cuentaIns(id_hilo); // Baja el cuantum y si es igual a 0 cambia de contexto
            break;
         }
    }
}

// Método que trae al caché del respectivo CPU los bloques que den un fallo de caché para LW, SC, SW, LL
void falloCacheDatos(int id_bloque, int direccion, int seccion, int id_hilo) {
    switch (id_hilo)
      {
         case 1:{
            
            if(id_bloque < 8){
                for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                    cacheDatos1[seccion][a] = memDatos1[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                    direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                    //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
                cacheDatos1[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                cacheDatos1[seccion][5] = 0; // se guarda 0 porque esta compartido
            
                for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
            }else{
                if(id_bloque < 16) {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                      cacheDatos1[seccion][a] = memDatos2[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                      direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                      //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                    }
                    cacheDatos1[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                    cacheDatos1[seccion][5] = 0; // se guarda 0 porque esta compartido
                }else{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos1[seccion][a] = memDatos3[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                        //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                    }
                    cacheDatos1[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                    cacheDatos1[seccion][5] = 0; // se guarda 0 porque esta compartido
                }
                for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
            }
            
            break;
         }
         case 2:{
            if(id_bloque > 7 && id_bloque < 16){
                for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                    cacheDatos2[seccion][a] = memDatos2[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                    direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                    //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
                cacheDatos2[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                cacheDatos2[seccion][5] = 0; // se guarda 0 porque esta compartido
            
                for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
            }else{
                if(id_bloque < 8) {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                      cacheDatos2[seccion][a] = memDatos1[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                      direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                      //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                    }
                    cacheDatos2[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                    cacheDatos2[seccion][5] = 0; // se guarda 0 porque esta compartido
                }else{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos2[seccion][a] = memDatos3[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                        //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                    }
                    cacheDatos2[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                    cacheDatos2[seccion][5] = 0; // se guarda 0 porque esta compartido
                }
                for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
            }
            break;
         }
         case 3:{
            if(id_bloque > 15){
                for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                    cacheDatos3[seccion][a] = memDatos3[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                    direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                    //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
                cacheDatos3[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                cacheDatos3[seccion][5] = 0; // se guarda 0 porque esta compartido
            
                for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
            }else{
                if(id_bloque < 7) {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                      cacheDatos3[seccion][a] = memDatos1[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                      direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                      //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                    }
                    cacheDatos3[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                    cacheDatos3[seccion][5] = 0; // se guarda 0 porque esta compartido
                }else{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos3[seccion][a] = memDatos2[direccion]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                        //pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                    }
                    cacheDatos3[seccion][4] = id_bloque; // Se guarda el Numero de bolque que se acaba de subir a la cache en la seccion de Numero de bloque de la cache 
                    cacheDatos3[seccion][5] = 0; // se guarda 0 porque esta compartido
                }
                for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                }
            }
            break;
         }
    }
}

// Método que guarda el dato de la caché del respectivo CPU a memoria para LW, SC, SW, LL
void guardaCacheDatosMem(int id_bloque, int direccion, int seccion, int id_cache) {
    switch (id_cache)
      {
         case 1:{
            if(id_bloque < 8){
                for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                    memDatos1[direccion] = cacheDatos1[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                    direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                 }
                 for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                 }
            }else{
                if(id_bloque < 16) {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        memDatos2[direccion] = cacheDatos1[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                     }
                     for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }    
                } else {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        memDatos3[direccion] = cacheDatos1[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                     }
                     for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                }
            }
           
            break;
         }
         case 2:{
            if(id_bloque < 8){
                for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                    memDatos1[direccion] = cacheDatos2[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                    direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                 }
                 for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                 }
            }else{
                if(id_bloque < 16) {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        memDatos2[direccion] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                     }
                     for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }    
                } else {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        memDatos3[direccion] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                     }
                     for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                }
            }
            break;
         }
         case 3:{
            if(id_bloque < 8){
                for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                    memDatos1[direccion] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                    direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                 }
                 for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                    pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                 }
            }else{
                if(id_bloque < 16) {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        memDatos2[direccion] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                     }
                     for(int a = 0; a < 32; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }    
                } else {
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        memDatos3[direccion] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                        direccion++; //Aunmento de la direccion Al espacio de memoria siguiente
                     }
                     for(int a = 0; a < 16; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                }
            }
            break;
         }
    }
}

// Método que guarda el dato de la caché del respectivo CPU a memoria para LW, SC, SW, LL
void guardaCacheDatosCache(int id_bloque, int seccion, int id_cacheDestino, int id_cacheRemota) {
    switch (id_cacheDestino)
      {
         case 1:{
            switch (id_cacheRemota)
             {
                 case 2:{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos1[seccion][a] = cacheDatos2[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                     }
                     for(int a = 0; a < 20; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                    break;
                 }
                 case 3:{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos1[seccion][a] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                     }
                     for(int a = 0; a < 20; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                    break;
                 }
            }    
            break;
         }
         case 2:{
            switch (id_cacheRemota)
             {
                 case 1:{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos2[seccion][a] = cacheDatos1[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                     }
                     for(int a = 0; a < 20; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                    break;
                 }
                 case 3:{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos2[seccion][a] = cacheDatos3[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                     }
                     for(int a = 0; a < 20; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                    break;
                 }
            }
            break;
         }
         case 3:{
            switch (id_cacheRemota)
             {
                 case 1:{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos3[seccion][a] = cacheDatos1[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                     }
                     for(int a = 0; a < 20; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                    break;
                 }
                 case 2:{
                    for(int a = 0; a < 4; a++) { // Simulación de los 16 ciclos del fallo de caché
                        cacheDatos3[seccion][a] = cacheDatos2[seccion][a]; //Guarda en la sección especificada de la cache del CPU1 los valores que estan en el bloque de la direccion de la memoria principal   
                     }
                     for(int a = 0; a < 20; a++) { // Simulación de los 16 ciclos del fallo de caché
                        pthread_barrier_wait(&barrier); //Barrera de control para simular los 16 ciclos del fallo de cache mientras los otros CPU's siguen trabajando
                     }
                    break;
                 }
                 
            }
            break;
         }
    }
}

vector<int> buscarBloque(int id_hilo) {
    int bloque; //Bloque que se va a buscar PC/16
    int palabra; //número de palabra del bloque PC%16
    int seccion; //seccion en cache a la que pertenece el bloque Bolque%4 
    vector<int> plb (4); // vector que contiene la palabra
                
    switch (id_hilo)
      {
         case 1:{ 
            bloque = pc1/16;            //bloque CPU1 
            palabra = pc1%16;           //palabra CPU1
            seccion = bloque%4;         //seccion CPU1
            // Mientras el bloque no este en la seccion de la cache que le corresponde se mete a fallo de cache 
            while(cache1[seccion][16] != bloque) {
                int dirFisica = bloque * 16; // Calcula la dirección física en memoria
                falloCache(bloque, dirFisica, seccion, id_hilo);  // Guarda el bloque en la cache
                if(quantum1 == quantum) { //si se acaba el quantum hay que volver a calcular bolque, palabra y seccion.
                    bloque = pc1/16;
                    palabra = pc1%16;
                    seccion = bloque%4;
                }
            }
            // Recupera la palabra de la caché
            for(int a = 0; a < 4; a++) {
                plb[a] = cache1[seccion][palabra];
                palabra++;
            }
            break;
         }
         case 2:{
            bloque = pc2/16;            //bloque CPU2
            palabra = pc2%16;           //palabra CPU2
            seccion = bloque%4;         //seccion CPU2
            // Mientras el bloque no este en la seccion de la cache que le corresponde se mete a fallo de cache 
            while(cache2[seccion][16] != bloque) {
                int dirFisica = bloque * 16; // Calcula la dirección física en memoria
                falloCache(bloque, dirFisica, seccion, id_hilo);  // Guarda el bloque en la cache
                if(quantum2 == quantum) {  //si se acaba el quantum hay que volver a calcular bolque, palabra y seccion.
                    bloque = pc2/16;
                    palabra = pc2%16;
                    seccion = bloque%4;
                }
            }
            // Recupera la palabra de la caché
            for(int a = 0; a < 4; a++) {
                plb[a] = cache2[seccion][palabra];
                palabra++;
            }
            break;
         }
         case 3:{
            bloque = pc3/16;            //bloque CPU3
            palabra = pc3%16;           //palabra CPU3
            seccion = bloque%4;         //seccion CPU3
            // Mientras el bloque no este en la seccion de la cache que le corresponde se mete a fallo de cache 
            while(cache3[seccion][16] != bloque) {
                int dirFisica = bloque * 16; // Calcula la dirección física en memoria
                falloCache(bloque, dirFisica, seccion, id_hilo); // Guarda el bloque en la cache
                if(quantum3 == quantum) { //si se acaba el quantum hay que volver a calcular bolque, palabra y seccion.
                    bloque = pc3/16;
                    palabra = pc3%16;
                    seccion = bloque%4;
                }
            }
            // Recupera la palabra de la caché
            for(int a = 0; a < 4; a++) {
                plb[a] = cache3[seccion][palabra];
                palabra++;
            }
            break;
         }
    }
    
    return plb;
}



// Metodo que se encarga de reservar caches
bool reservarCache(int cache) {
    bool resultado= false;
    if(cache == 1) {
        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
            resultado = true;
        }
    }
    if(cache == 2) {
        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
            resultado = true;
        }
    }
    if(cache == 3) {
        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
            resultado = true;
        }
    }
    return resultado;
}

// Metodo que se encarga de liberar los recursos que se hayan adquirido
vector<int> liberarRecursos(vector<int> recursos) {
    if(recursos[1]== 1) {
        pthread_mutex_unlock(&mCacheDatos1);
        recursos[1]= 0;
    }
    if(recursos[2]== 1) {
        pthread_mutex_unlock(&mCacheDatos2);
        recursos[2]= 0;
    }
    if(recursos[3]== 1) {
        pthread_mutex_unlock(&mCacheDatos3);
        recursos[3]= 0;
    }
    if(recursos[4]== 1) {
        pthread_mutex_unlock(&mDirectorio1);
        recursos[4]= 0;
    }
    if(recursos[5]== 1) {
        pthread_mutex_unlock(&mDirectorio2);
        recursos[5]= 0;
    }
    if(recursos[6]== 1) {
        pthread_mutex_unlock(&mDirectorio3);
        recursos[6]= 0;
    }
    return recursos;
}

//Metodo que dice si el dato esta en cache
bool estaCache(int cache, int seccion, int bloque){
    bool esta = false;
    switch (cache) {
        case 1:{                            
            if(cacheDatos1[seccion][4] == bloque){
                esta = true;
            }
            break;
        }
        case 2:{                            
            if(cacheDatos2[seccion][4] == bloque){
                esta = true;
            }
            break;
        }
        case 3:{                            
            if(cacheDatos3[seccion][4] == bloque){
                esta = true;
            }
            break;
        }
    }
    return esta;
}

//Metodo que dice si el dato esta compartido en cache
bool estaCompartido(int cache, int seccion){
    bool esta = false;
    switch (cache) {
        case 1:{                            
            if(cacheDatos1[seccion][5] == 0){
                esta = true;
            }
            break;
        }
        case 2:{                            
            if(cacheDatos2[seccion][5] == 0){
                esta = true;
            }
            break;
        }
        case 3:{                            
            if(cacheDatos3[seccion][5] == 0){
                esta = true;
            }
            break;
        }
    }
    return esta;
}

//Metodo que dice si el dato esta modificado en cache
bool estaModificado(int cache, int seccion){
    bool esta = false;
    switch (cache) {
        case 1:{                           
            if(cacheDatos1[seccion][5] == 1){
                esta = true;
            }
            break;
        }
        case 2:{                            
            if(cacheDatos2[seccion][5] == 1){
                esta = true;
            }
            break;
        }
        case 3:{                          
            if(cacheDatos3[seccion][5] == 1){
                esta = true;
            }
            break;
        }
    }
    return esta;
}

//Metodo que dice si el dato esta invalido en cache
bool estaInvalido(int cache, int seccion){
    bool esta = false;
    switch (cache) {
        case 1:{                            
            if(cacheDatos1[seccion][5] == 2){
                esta = true;
            }
            break;
        }
        case 2:{                        
            if(cacheDatos2[seccion][5] == 2){
                esta = true;
            }
            break;
        }
        case 3:{                       
            if(cacheDatos3[seccion][5] == 2){
                esta = true;
            }
            break;
        }
    }
    return esta;
}



//Metodo que dice si el dato esta compartido en directorio
bool estaCompartidodir(int directorio, int bloque){
    bool esta = false;
    switch (directorio) {
        case 4:{                           
            if(directorio1[bloque][0] == 0){
                esta = true;
            }
            break;
        }
        case 5:{                            
            if(directorio2[bloque][0] == 0){
                esta = true;
            }
            break;
        }
        case 6:{                          
            if(directorio3[bloque][0] == 0){
                esta = true;
            }
            break;
        }
    }
    return esta;
}

//Metodo que dice si el dato esta modificado en directorio
bool estaModificadodir(int directorio, int bloque){
    bool esta = false;
    switch (directorio) {
        case 4:{                           
            if(directorio1[bloque][0] == 1){
                esta = true;
            }
            break;
        }
        case 5:{                            
            if(directorio2[bloque][0] == 1){
                esta = true;
            }
            break;
        }
        case 6:{                          
            if(directorio3[bloque][0] == 1){
                esta = true;
            }
            break;
        }
    }
    return esta;
}

//Metodo que dice si el dato esta uncached en directorio
bool estaUncacheddir(int directorio, int bloque){
    bool esta = false;
    switch (directorio) {
        case 4:{                            
            if(directorio1[bloque][0] == 2){
                esta = true;
            }
            break;
        }
        case 5:{                        
            if(directorio2[bloque][0] == 2){
                esta = true;
            }
            break;
        }
        case 6:{                       
            if(directorio3[bloque][0] == 2){
                esta = true;
            }
            break;
        }
    }
    return esta;
}

//metodo que guarda dato en cache
void guardarDato(int id_hilo, int seccion, int palabraBloque, int registro){
    switch (id_hilo) {
        case 1:{                    
            cacheDatos1[seccion][palabraBloque] = reg1[registro];
            break;
        }
        case 2:{                            
            cacheDatos2[seccion][palabraBloque] = reg2[registro];
            break;
        }
        case 3:{                            
            cacheDatos3[seccion][palabraBloque] = reg3[registro];
            break;
        }
    }
    
}

//metodo que lee dato de cache
void leePalabra(int cache, int seccion, int palabraBloque, int registro){
    switch (cache) {
        case 1:{                    
            reg1[registro] = cacheDatos1[seccion][palabraBloque];
            break;
        }
        case 2:{                            
            reg2[registro] = cacheDatos2[seccion][palabraBloque];
            break;
        }
        case 3:{                            
            reg3[registro] = cacheDatos3[seccion][palabraBloque];
            break;
        }
    }
    
}




//metodo que dice que directorio esta el bloque
int directorioBloque(int bloque){
    int resultado = 0;
    if(bloque < 8) {                                    //si el bloque esta en dir1
        resultado = 4;                                  //Retorna 4 porque es la posicion en mlock
    }else{
        if(bloque < 16) {                               //si el bloque esta en dir2
            resultado = 5;                              //Retorna 5 porque es la posicion en mlock
        }else{                                          //si el bloque esta en dir3
            resultado = 6;                              //Retorna 6 porque es la posicion en mlock
        }    
    }
    return resultado;
}

// Metodo que se encarga de reservar directorios
bool reservarDirectorio(int bloque) {
    bool resultado= false;
    if(bloque < 8) {                                    //si el bloque esta en dir1
        if(pthread_mutex_trylock(&mDirectorio1) == 0){
            resultado = true;
        }
    }else{
        if(bloque < 16) {                               //si el bloque esta en dir2
            if(pthread_mutex_trylock(&mDirectorio2) == 0){
                resultado = true;
            }
        }else{                                          //si el bloque esta en dir3
                if(pthread_mutex_trylock(&mDirectorio3) == 0){
                    resultado = true;
                }
            }    
        
    }
    
    
    return resultado;
}

/*metodo que retorna en que caches esta el bloque
    0 = ninguna cache
    1 = si esta en cacheaux1
    2 = si esta en cacheaux2
    3 = si esta en ambas
*/
int cachesBloque(int directorio, int bloque, int id_hilo){
    
    int resultado = 0;
    switch (directorio) {
        case 4:{                                                                        //Directorio 1       
            switch (id_hilo) {
                case 1:{                                                                //CPU1    
                    if(directorio1[bloque][2] == 1 && directorio1[bloque][3] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio1[bloque][2] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;  
                        }else{
                            if(directorio1[bloque][3] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
                case 2:{                                                                //CPU2
                    if(directorio1[bloque][1] == 1 && directorio1[bloque][3] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio1[bloque][1] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio1[bloque][3] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
                case 3:{                                                                //CPU3
                    if(directorio1[bloque][1] == 1 && directorio1[bloque][2] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio1[bloque][1] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio1[bloque][2] == 1){                    //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
            }
            break;
        }
        case 5:{                                                                        //Directorio 2
            switch (id_hilo) {
                case 1:{                                                                    //CPU1
                    if(directorio2[bloque][2] == 1 && directorio2[bloque][3] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{  
                        if(directorio2[bloque][2] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio2[bloque][3] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
                case 2:{                                                                //CPU2
                    if(directorio2[bloque][1] == 1 && directorio2[bloque][3] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio2[bloque][1] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio2[bloque][3] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
                case 3:{                                                                //CPU3
                    if(directorio2[bloque][1] == 1 && directorio2[bloque][2] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio2[bloque][1] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio2[bloque][2] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
            }
            break;
        }
        case 6:{                                                                        //Directorio 3
            switch (id_hilo) {
                case 1:{                                                                    //CPU1
                    if(directorio3[bloque][2] == 1 && directorio3[bloque][3] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio3[bloque][2] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio3[bloque][3] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }   
                case 2:{                                                                //CPU2
                    if(directorio3[bloque][1] == 1 && directorio3[bloque][3] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio3[bloque][1] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio3[bloque][3] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
                case 3:{                                                                //CPU3
                    if(directorio3[bloque][1] == 1 && directorio3[bloque][2] == 1){     //El bloque esta en ambas caches
                        resultado = 3;
                    }else{
                        if(directorio3[bloque][1] == 1){                                //El bloque esta en cacheaux1
                            resultado = 1;
                        }else{
                            if(directorio3[bloque][2] == 1){                            //El bloque esta en cacheaux2
                                resultado = 2;
                            }   
                        }   
                    }
                    break;
                }
            }    
        }
            break;
    }
}
    

//metodo que invalida el bloque de la seccion de la cache 
void invalidarCache(int cache, int seccion){
    
    switch (cache) {
        case 1:{                    
            cacheDatos1[seccion][5] = 2;
            break;
        }
        case 2:{                            
            cacheDatos2[seccion][5] = 2;
            break;
        }
        case 3:{                            
            cacheDatos3[seccion][5] = 2;
            break;
        }
    }
}

//metodo que actualiza estado del bloque en directorios y deja como unico dato el que acaba de modificar
void actualizaDirSW(int directorio, int cache, int bloque){
    switch (directorio) {
        case 4:{                    
            directorio1[bloque][0] = 1;             // pone estado como modificado
            for(int i = 1; i < 4; i++){
                if(i == cache){                     // pone cache como unica que la tiene
                    directorio1[bloque][i] = 1;
                }else{                              // pone caches en 0
                    directorio1[bloque][i] = 0;
                }
            }
            break;
        }
        case 5:{                     
            directorio2[bloque][0] = 1;             // pone estado como modificado
            for(int i = 1; i < 4; i++){
                if(i == cache){                     // pone cache como unica que la tiene
                    directorio2[bloque][i] = 1;
                }else{                              // pone caches en 0
                    directorio2[bloque][i] = 0;
                }
            }
            break;
        }
        case 6:{                       
            directorio3[bloque][0] = 1;             // pone estado como modificado
            for(int i = 1; i < 4; i++){
                if(i == cache){                     // pone cache como unica que la tiene
                    directorio3[bloque][i] = 1;
                }else{                              // pone caches en 0
                    directorio3[bloque][i] = 0;
                }
            }
            break;
        }
    }
    
}

//metodo que actualiza estado del bloque en directorios y deja como unico dato el que acaba de modificar
void directorioCompartido(int directorio, int bloque, int cache){
    switch (directorio) {
        case 4:{                    
            directorio1[bloque][0] = 0;             // pone estado como compartido
            directorio1[bloque][cache] = 1;         // Agrega cache 1 como compartida
            break;
        }
        case 5:{                     
            directorio2[bloque][0] = 0;             // pone estado como compartido
            directorio2[bloque][cache] = 1;         // Agrega cache 2 como compartida
            break;
        }
        case 6:{                       
            directorio3[bloque][0] = 0;             // pone estado como compartido
            directorio3[bloque][cache] = 1;         // Agrega cache 3 como compartida
            break;
        }
    }
    
}



void storeWord(int id_hilo, vector<int> palabra) {
    // Vector para los recursos
    
    /*  mlocks[1] - mlocks[2] - mlocks[3] = caches 1, 2 y 3 respectivamente
        mlocks[4] - mlocks[5] - mlocks[6] = directorios 1, 2 y 3 respectivamente
    */
    vector<int> mLocks (7);
    bool siga = true;
    int dir = reg1[palabra[1]] + palabra[3];    //direccion donde esta el bloque
    int bloque = dir/16;                        //numero de bloque que se quiere guardar
    int palabraBloque = dir%16;                 //Numero de palabra que se quiere guardar
    int seccion = bloque%4;                     //seccion de la cache donde el bloque cae
    dir = dir / 4;                          //Division que se realiza para que la direccion concuerde con nuestra memoria
    int directorio = directorioBloque(bloque);  //Directorio donde esta el bloque que se quiere modificar
    int cachePrin = id_hilo;                //cache donde se va a guardar el dato
    int cacheAux1, cacheAux2;               //caches donde puede estar el dato compartido o modificado
    
    switch (id_hilo) {
        case 1:{                            //caso que CPU1 sea el que quiere guardar
            cacheAux1 = 2;
            cacheAux2 = 3;
            break;
        }
        case 2:{                            //caso que CPU2 sea el que quiere guardar
            cacheAux1 = 1;
            cacheAux2 = 3;
            break;
        }
        case 3:{                            //caso que CPU3 sea el que quiere guardar
            cacheAux1 = 1;
            cacheAux2 = 2;
            break;
        }
    }
    while(siga){                                                                                //mientras siga = true entra en el ciclo
        if(reservarCache(cachePrin)){                                                           //reserva cacheprincipal
            mLocks[cachePrin] = 1;                                                              //actualiza vector para liberacion de recursos
            if(estaCache(cachePrin,seccion,bloque) && estaModificado(cachePrin,seccion)){       //pregunta si bloque esta en cache y si esta modificado
                guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);                          //Guarda el dato en el bloque que esta en la seccion de la cache principal
                mLocks = liberarRecursos(mLocks);                                               //libera recursos
                siga=false;                                                                     //para que se salga del ciclo
            }else{
                if(estaCache(cachePrin,seccion,bloque) && estaCompartido(cachePrin,seccion)){   //Pregunta si el bloque esta en cacje y si esta compartido
                    if(reservarDirectorio(bloque)){                                             //reserva el directorio donde se ubica el bloque que se quiere buscar
                        mLocks[directorio] = 1;                                                  //actualiza vector para liberacion de recursos
                        /*  
                            0 = ninguna cache
                            1 = si esta en cacheaux1
                            2 = si esta en cacheaux2
                            3 = si esta en ambas
                        */
                        int cachesinvalidar = cachesBloque(directorio,bloque,id_hilo);        //Pregunta cuales caches hay que invalidar
                        switch (cachesinvalidar) {      
                            case 1:{                                                            //si el bloque solo esta en cacheaux1
                                if(reservarCache(cacheAux1)){                                   //reserva cacheaux1
                                    mLocks[cacheAux1] = 1;                                      //actualiza vector para liberacion de recursos
                                    invalidarCache(cacheAux1,seccion);                                  //Invalida cacheaux1
                                    actualizaDirSW(directorio,cachePrin,bloque);                //Pone el bloque como modificado y indica que esta en la cache principal
                                    guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);      //Guarda el dato en el bloque que esta en la seccion de la cache principal  
                                    mLocks = liberarRecursos(mLocks);                           //libera recursos
                                    siga=false;                                                 
                                }else{                                                          //entra si cacheaux1 no pudo ser reservado
                                    mLocks = liberarRecursos(mLocks);                           //libera recursos
                                }
                                break;
                            }
                            case 2:{                                                            //mismos comentarios pero con cacheaux2                    
                                if(reservarCache(cacheAux2)){                                   
                                    mLocks[cacheAux2] = 1;                                      
                                    invalidarCache(cacheAux2,seccion);                                  
                                    actualizaDirSW(directorio,cachePrin,bloque);                
                                    guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);      
                                    mLocks = liberarRecursos(mLocks);                           
                                    siga=false;                                                 
                                }else{                                                          
                                    mLocks = liberarRecursos(mLocks);                           
                                }
                                break;
                            }
                            case 3:{                                                            //mismos comentarios pero con ambas caches
                                if(reservarCache(cacheAux1) && reservarCache(cacheAux2)){      
                                    mLocks[cacheAux1] = 1;                                     
                                    mLocks[cacheAux2] = 1;                                     
                                    invalidarCache(cacheAux1,seccion);                                 
                                    invalidarCache(cacheAux2,seccion);                                 
                                    actualizaDirSW(directorio,cachePrin,bloque);               
                                    guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);      
                                    mLocks = liberarRecursos(mLocks);                          
                                    siga=false;
                                }else{
                                    mLocks = liberarRecursos(mLocks);
                                }
                                break;
                            }
                        }
                    }else{                                                                      //Entra si directorio donde esta el bloque no se puede reservar
                        mLocks = liberarRecursos(mLocks);
                    }
                }else{                                                                          //si el bloque no esta en cache o esta invalidado
                    if(reservarDirectorio(bloque)){                                             //reserva el directorio donde esta el bloque que se quiere modificar
                        mLocks[directorio] = 1;
                        if(estaUncacheddir(directorio,bloque)){                                 //el bloque este uncached
                            falloCacheDatos(bloque, dir, seccion, id_hilo);                     //trae el bloque de cache
                            actualizaDirSW(directorio,cachePrin,bloque);                        //Pone el bloque como modificado y indica que esta en la cache principal
                            guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);              //guarda dato
                            mLocks = liberarRecursos(mLocks);                                   //libera recursos
                            siga = false;
                        }else{
                            if(estaCompartidodir(directorio,bloque)){                           //el bloque este compartido en directorio
                                /*  
                                    0 = ninguna cache
                                    1 = si esta en cacheaux1
                                    2 = si esta en cacheaux2
                                    3 = si esta en ambas
                                */
                                int cachesinvalidar = cachesBloque(directorio,bloque,id_hilo);    //pregunta las caches donde esta el bloque compartido 
                                switch (cachesinvalidar) {                                          //mismos comentarios que para cuando esta compartido en la cache
                                    case 1:{                            
                                        if(reservarCache(cacheAux1)){
                                            mLocks[cacheAux1] = 1;  
                                            falloCacheDatos(bloque, dir, seccion, id_hilo);         //trae el dato de la memoria de datos
                                            invalidarCache(cacheAux1,seccion);
                                            actualizaDirSW(directorio,cachePrin,bloque);
                                            guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);
                                            mLocks = liberarRecursos(mLocks);
                                            siga=false;
                                        }else{
                                            mLocks = liberarRecursos(mLocks);
                                        }
                                        break;
                                    }
                                    case 2:{                        
                                        if(reservarCache(cacheAux2)){
                                            mLocks[cacheAux2] = 1;
                                            falloCacheDatos(bloque, dir, seccion, id_hilo);         //trae el dato de la memoria de datos
                                            invalidarCache(cacheAux2,seccion);
                                            actualizaDirSW(directorio,cachePrin,bloque);
                                            guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);
                                            mLocks = liberarRecursos(mLocks);
                                            siga=false;
                                        }else{
                                            mLocks = liberarRecursos(mLocks);
                                        }
                                        break;
                                    }
                                    case 3:{                       
                                        if(reservarCache(cacheAux1) && reservarCache(cacheAux2)){
                                            mLocks[cacheAux1] = 1;
                                            mLocks[cacheAux2] = 1;
                                            falloCacheDatos(bloque, dir, seccion, id_hilo);         //trae el dato de la memoria de datos
                                            invalidarCache(cacheAux1,seccion);
                                            invalidarCache(cacheAux2,seccion);
                                            actualizaDirSW(directorio,cachePrin,bloque);
                                            guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);
                                            mLocks = liberarRecursos(mLocks);
                                            siga=false;
                                        }else{
                                            mLocks = liberarRecursos(mLocks);
                                        }
                                        break;
                                    }
                                } 
                            }else{  //Este modificado 
                                int cachesinvalidar = cachesBloque(directorio,bloque,id_hilo);
                                switch (cachesinvalidar) {                                                  //comentarios iguales a los de compartido en la cache
                                    case 1:{                            
                                        if(reservarCache(cacheAux1)){
                                            mLocks[cacheAux1] = 1;
                                            guardaCacheDatosMem(bloque, dir, seccion, cacheAux1);           // guarda el dato en memoria antes de invalidarlo
                                            guardaCacheDatosCache(bloque, seccion, cachePrin, cacheAux1);   //trae el dato de la cacheaux1 a la cache principal
                                            invalidarCache(cacheAux1,seccion);
                                            actualizaDirSW(directorio,cachePrin,bloque);
                                            guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);
                                            mLocks = liberarRecursos(mLocks);
                                            siga=false;
                                        }else{
                                            mLocks = liberarRecursos(mLocks);
                                        }
                                        break;
                                    }
                                    case 2:{                        
                                        if(reservarCache(cacheAux2)){
                                            mLocks[cacheAux2] = 1;
                                            guardaCacheDatosMem(bloque, dir, seccion, cacheAux2);           // guarda el dato en memoria antes de invalidarlo
                                            guardaCacheDatosCache(bloque, seccion, cachePrin, cacheAux2);   //trae el dato de la cacheaux2 a la cache principal
                                            invalidarCache(cacheAux2,seccion);
                                            actualizaDirSW(directorio,cachePrin,bloque);
                                            guardarDato(id_hilo,seccion,palabraBloque,palabra[2]);
                                            mLocks = liberarRecursos(mLocks);
                                            siga=false;
                                        }else{
                                            mLocks = liberarRecursos(mLocks);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    
                    }else{
                       mLocks = liberarRecursos(mLocks);
                    }
                }
            }
        }
    }
}

// Metodo para realizar el LW con todos sus casos
void loadWord(int id_hilo, vector<int> palabra) {
    // Vector para los recursos
    
    /*  mlocks[1] - mlocks[2] - mlocks[3] = caches 1, 2 y 3 respectivamente
        mlocks[4] - mlocks[5] - mlocks[6] = directorios 1, 2 y 3 respectivamente
    */
    vector<int> mLocks (7);
    bool siga = true;
    int dir = reg1[palabra[1]] + palabra[3];    //direccion donde esta el bloque
    int bloque = dir/16;                        //numero de bloque que se quiere guardar
    int palabraBloque = dir%16;                 //Numero de palabra que se quiere guardar
    int seccion = bloque%4;                     //seccion de la cache donde el bloque cae
    dir = dir / 4;                          //Division que se realiza para que la direccion concuerde con nuestra memoria
    int directorio = directorioBloque(bloque);  //Directorio donde esta el bloque que se quiere modificar
    int cachePrin = id_hilo;                //cache donde se va a guardar el dato
    int cacheAux1, cacheAux2;               //caches donde puede estar el dato compartido o modificado
    
    switch (id_hilo) {
        case 1:{                            //caso que CPU1 sea el que quiere guardar
            cacheAux1 = 2;
            cacheAux2 = 3;
            break;
        }
        case 2:{                            //caso que CPU2 sea el que quiere guardar
            cacheAux1 = 1;
            cacheAux2 = 3;
            break;
        }
        case 3:{                            //caso que CPU3 sea el que quiere guardar
            cacheAux1 = 1;
            cacheAux2 = 2;
            break;
        }
    }
    
    while(siga){
        if(reservarCache(cachePrin)){
            mLocks[cachePrin] = 1; 
            if(estaCache(cachePrin,seccion,bloque) && !estaInvalido(cachePrin,seccion)){
                leePalabra(cachePrin, seccion, palabraBloque, palabra[2]);
                siga = false;
                mLocks = liberarRecursos(mLocks);
            }else{
                if(reservarDirectorio(bloque)){
                    mLocks[directorio] = 1;
                    if(!estaModificadodir(directorio, bloque)){
                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                        directorioCompartido(directorio,bloque,cachePrin);
                        leePalabra(cachePrin, seccion, palabraBloque, palabra[2]);
                        siga = false;
                        mLocks = liberarRecursos(mLocks);
                    }else{ 
                        int cachesmodificadas = cachesBloque(directorio,bloque,cachePrin);
                        switch (cachesmodificadas) {                                                  
                            case 1:{                            
                                if(reservarCache(cacheAux1)){
                                    mLocks[cacheAux1] = 1;
                                    guardaCacheDatosMem(bloque, dir, seccion, cacheAux1);           // guarda el dato en memoria antes de invalidarlo
                                    guardaCacheDatosCache(bloque, seccion, cachePrin, cacheAux1);   // trae el dato de la cacheaux1 a la cache principal
                                    directorioCompartido(directorio,bloque,cachePrin);
                                    leePalabra(cachePrin, seccion, palabraBloque, palabra[2]);
                                    siga = false;
                                    mLocks = liberarRecursos(mLocks);
                                }else{
                                    mLocks = liberarRecursos(mLocks);
                                }
                                break;
                            }
                            case 2:{                        
                                if(reservarCache(cacheAux2)){
                                    mLocks[cacheAux2] = 1;
                                    guardaCacheDatosMem(bloque, dir, seccion, cacheAux2);           // guarda el dato en memoria antes de invalidarlo
                                    guardaCacheDatosCache(bloque, seccion, cachePrin, cacheAux2);   // trae el dato de la cacheaux2 a la cache principal
                                    directorioCompartido(directorio,bloque,cachePrin);
                                    leePalabra(cachePrin, seccion, palabraBloque, palabra[2]);
                                    siga = false;
                                    mLocks = liberarRecursos(mLocks);
                                }else{
                                    mLocks = liberarRecursos(mLocks);
                                }
                                break;
                            }
                        }
                        
                    }
                }else{
                    mLocks = liberarRecursos(mLocks);
                }
            }
        }
    }
}
    
    
void loadLink(int id_hilo, vector<int> palabra) {
    loadWord(id_hilo,palabra);
    switch (id_hilo) {
        case 1:{
            RL1 = reg1[palabra[1]] + palabra[3];
            break;
         }
        case 2:{
            RL2 = reg2[palabra[1]] + palabra[3];
            break;
        }
        case 3:{
            RL3 = reg3[palabra[1]] + palabra[3];
            break;
        }
    }
}

void storeConditional(int id_hilo, vector<int> palabra) {
    bool siga = true;
    switch (id_hilo) {
        case 1:{
            while(siga){
                if(pthread_mutex_trylock(&mRL1) == 0){
                    if(RL1 == reg1[palabra[1]] + palabra[3]){
                        if(RL2 == reg1[palabra[1]] + palabra[3] && RL3 == reg1[palabra[1]] + palabra[3]){
                            if(pthread_mutex_trylock(&mRL2) == 0 ){
                                if(pthread_mutex_trylock(&mRL3) == 0) {
                                        RL1 = -1;
                                        RL2 = -1;
                                        RL3 = -1;
                                        storeWord(id_hilo,palabra);
                                        siga = false;
                                        pthread_mutex_unlock(&mRL1);
                                        pthread_mutex_unlock(&mRL2);
                                        pthread_mutex_unlock(&mRL3);
                                }else{
                                    pthread_mutex_unlock(&mRL1);
                                    pthread_mutex_unlock(&mRL2);
                                }
                            }else{
                                pthread_mutex_unlock(&mRL1);
                            }
                        }else{
                            if(RL2 == reg1[palabra[1]] + palabra[3]){
                                if(pthread_mutex_trylock(&mRL2) == 0) {
                                        RL1 = -1;
                                        RL2 = -1;
                                        storeWord(id_hilo,palabra);
                                        siga = false;
                                        pthread_mutex_unlock(&mRL1);
                                        pthread_mutex_unlock(&mRL2);
                                }else{
                                    pthread_mutex_unlock(&mRL1);
                                }
                            }else{
                                if(RL3 == reg1[palabra[1]] + palabra[3]){
                                    if(pthread_mutex_trylock(&mRL3) == 0) {
                                            RL1 = -1;
                                            RL3 = -1;
                                            storeWord(id_hilo,palabra);
                                            siga = false;
                                            pthread_mutex_unlock(&mRL1);
                                            pthread_mutex_unlock(&mRL3);
                                    }else{
                                        pthread_mutex_unlock(&mRL1);
                                    }
                                }else{
                                    RL1 = -1;
                                    storeWord(id_hilo,palabra);
                                    siga = false;
                                    pthread_mutex_unlock(&mRL1);
                                }   
                            }    
                        }
                    }else{
                        reg1[palabra[2]] = 0;
                        siga = false;
                    }    
                }
            }
            
            break;
        }
        case 2:{
            while(siga){
                if(pthread_mutex_trylock(&mRL2) == 0){
                    if(RL2 == reg2[palabra[1]] + palabra[3]){
                        if(RL1 == reg2[palabra[1]] + palabra[3] && RL3 == reg2[palabra[1]] + palabra[3]){
                            if(pthread_mutex_trylock(&mRL1) == 0 ){
                                if(pthread_mutex_trylock(&mRL3) == 0) {
                                        RL2 = -1;
                                        RL1 = -1;
                                        RL3 = -1;
                                        storeWord(id_hilo,palabra);
                                        siga = false;
                                        pthread_mutex_unlock(&mRL2);
                                        pthread_mutex_unlock(&mRL1);
                                        pthread_mutex_unlock(&mRL3);
                                }else{
                                    pthread_mutex_unlock(&mRL2);
                                    pthread_mutex_unlock(&mRL1);
                                }
                            }else{
                                pthread_mutex_unlock(&mRL2);
                            }
                        }else{
                            if(RL1 == reg2[palabra[1]] + palabra[3]){
                                if(pthread_mutex_trylock(&mRL1) == 0) {
                                        RL2 = -1;
                                        RL1 = -1;
                                        storeWord(id_hilo,palabra);
                                        siga = false;
                                        pthread_mutex_unlock(&mRL2);
                                        pthread_mutex_unlock(&mRL1);
                                }else{
                                    pthread_mutex_unlock(&mRL2);
                                }
                            }else{
                                if(RL3 == reg2[palabra[1]] + palabra[3]){
                                    if(pthread_mutex_trylock(&mRL3) == 0) {
                                            RL1 = -1;
                                            RL3 = -1;
                                            storeWord(id_hilo,palabra);
                                            siga = false;
                                            pthread_mutex_unlock(&mRL1);
                                            pthread_mutex_unlock(&mRL3);
                                    }else{
                                        pthread_mutex_unlock(&mRL2);
                                    }
                                }else{
                                    RL2 = -1;
                                    storeWord(id_hilo,palabra);
                                    siga = false;
                                    pthread_mutex_unlock(&mRL2);
                                }   
                            }    
                        }
                    }else{
                        reg1[palabra[2]] = 0;
                        siga = false;
                    }    
                }
            }
            break;
        }
        case 3:{
            while(siga){
                if(pthread_mutex_trylock(&mRL1) == 0){
                    if(RL3 == reg3[palabra[1]] + palabra[3]){
                        if(RL1 == reg3[palabra[1]] + palabra[3] && RL2 == reg3[palabra[1]] + palabra[3]){
                            if(pthread_mutex_trylock(&mRL1) == 0 ){
                                if(pthread_mutex_trylock(&mRL2) == 0) {
                                        RL1 = -1;
                                        RL2 = -1;
                                        RL3 = -1;
                                        storeWord(id_hilo,palabra);
                                        siga = false;
                                        pthread_mutex_unlock(&mRL1);
                                        pthread_mutex_unlock(&mRL2);
                                        pthread_mutex_unlock(&mRL3);
                                }else{
                                    pthread_mutex_unlock(&mRL1);
                                    pthread_mutex_unlock(&mRL3);
                                }
                            }else{
                                pthread_mutex_unlock(&mRL3);
                            }
                        }else{
                            if(RL2 == reg3[palabra[1]] + palabra[3]){
                                if(pthread_mutex_trylock(&mRL2) == 0) {
                                        RL2 = -1;
                                        RL3 = -1;
                                        storeWord(id_hilo,palabra);
                                        siga = false;
                                        pthread_mutex_unlock(&mRL2);
                                        pthread_mutex_unlock(&mRL3);
                                }else{
                                    pthread_mutex_unlock(&mRL3);
                                }
                            }else{
                                if(RL1 == reg3[palabra[1]] + palabra[3]){
                                    if(pthread_mutex_trylock(&mRL1) == 0) {
                                            RL1 = -1;
                                            RL3 = -1;
                                            storeWord(id_hilo,palabra);
                                            siga = false;
                                            pthread_mutex_unlock(&mRL1);
                                            pthread_mutex_unlock(&mRL3);
                                    }else{
                                        pthread_mutex_unlock(&mRL3);
                                    }
                                }else{
                                    RL3 = -1;
                                    storeWord(id_hilo,palabra);
                                    siga = false;
                                    pthread_mutex_unlock(&mRL3);
                                }   
                            }    
                        }
                    }else{
                        reg3[palabra[2]] = 0;
                        siga = false;
                    }    
                }
            }
            break;
        }
    }
}
    
/*    
// Metodo para realizar el SW con todos sus casos
void storeWordlargo(int id_hilo, vector<int> palabra) {
    bool siga = true;
    // Vector para los recursos
    
    /*  mlocks[1] - mlocks[2] - mlocks[3] = caches 1, 2 y 3 respectivamente
        mlocks[4] - mlocks[5] - mlocks[6] = directorios 1, 2 y 3 respectivamente
    
    vector<int> mLocks (7);
    
    dir = reg1[palabra[1]] + palabra[3]; 
    bloque = dir/16;
    palabraBloque = dir%16;
    seccion = bloque%4; 
    dir = dir / 4;
    switch (id_hilo) {
        // estado 0 = "C", 1 = "M", 2 = "I"
        case 1:{
            while(siga){
                if(pthread_mutex_trylock(&mCacheDatos1) == 0) { 
                       mlocks[1] = 1;
                       if((cacheDatos1[seccion][4] == bloque) && (cacheDatos1[seccion][5] == 1)){ //cuando el bloque esta en cache y estado "M"
                           
                           cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                           mlocks = liberarRecursos(mlocks);
                           siga = false;
                       }
                }else{
                    if((cacheDatos1[seccion][4] == bloque) && (cacheDatos1[seccion][5] == 0)){ //cuando el bloque esta en cache y estado "C"
                        if(bloque < 8){ //si el bloque est en directorio1
                            
                            if(pthread_mutex_trylock(&mDirectorio1) == 0){
                                mlocks[4] = 1;
                                
                                if(directorio1[bloque][2] = 1 && directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                        mlocks[2] = 1;
                                        mlocks[3] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio1[bloque][2] = 0;
                                        cacheDatos3[seccion][5] = 2;
                                        directorio1[bloque][3] = 0;
                                       
                                        directorio1[bloque][0] = 1;
                                        directorio1[bloque][1] = 1;
                                        cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                            mlocks[3] = 1;
                                            cacheDatos3[seccion][5] = 2;
                                            directorio1[bloque][3] = 0;
                                            directorio1[bloque][0] = 1;
                                            directorio1[bloque][1] = 1;
                                            cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio1[bloque][2] = 0;
                                            directorio1[bloque][0] = 1;
                                            directorio1[bloque][1] = 1;
                                            cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }else{
                            if(bloque < 16){ //si el bloque está en el directorio2
                                if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                mlocks[4] = 1;
                                
                                if(directorio2[bloque][2] = 1 && directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                        mlocks[2] = 1;
                                        mlocks[3] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio2[bloque][2] = 0;
                                        cacheDatos3[seccion][5] = 2;
                                        directorio2[bloque][3] = 0;
                                       
                                        directorio2[bloque][0] = 1;
                                        directorio2[bloque][1] = 1;
                                        cacheDatos2[seccion][palabraBloque] = reg1[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                            mlocks[3] = 1;
                                            cacheDatos3[seccion][5] = 2;
                                            directorio2[bloque][3] = 0;
                                            directorio2[bloque][0] = 1;
                                            directorio2[bloque][1] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg1[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio2[bloque][2] = 0;
                                            directorio2[bloque][0] = 1;
                                            directorio2[bloque][1] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg1[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }else{ // si esta en directorio 3
                            if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                mlocks[6] = 1;
                                
                                if(directorio3[bloque][2] = 1 && directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                        mlocks[2] = 1;
                                        mlocks[3] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio3[bloque][2] = 0;
                                        cacheDatos3[seccion][5] = 2;
                                        directorio3[bloque][3] = 0;
                                       
                                        directorio3[bloque][0] = 1;
                                        directorio3[bloque][1] = 1;
                                        cacheDatos3[seccion][palabraBloque] = reg1[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                            mlocks[3] = 1;
                                            cacheDatos3[seccion][5] = 2;
                                            directorio3[bloque][3] = 0;
                                            directorio3[bloque][0] = 1;
                                            directorio3[bloque][1] = 1;
                                            cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio3[bloque][2] = 0;
                                            directorio3[bloque][0] = 1;
                                            directorio3[bloque][1] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg1[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }    
                }else{ // si el bloque no esta en la cache o si esta invalido
                    if(bloque < 8){ //si el bloque est en directorio1
                            if(pthread_mutex_trylock(&mDirectorio1) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio1[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio1[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio1[bloque][1] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio1[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio1[bloque][2] = 1 && directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                mlocks[2] = 1;
                                                mlocks[3] = 1;
                                                cacheDatos2[seccion][5] = 2;
                                                directorio1[bloque][2] = 0;
                                                cacheDatos3[seccion][5] = 2;
                                                directorio1[bloque][3] = 0;
                                               
                                                directorio1[bloque][0] = 1;
                                                directorio1[bloque][1] = 1;
                                                cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio1[bloque][3] = 0;
                                                    directorio1[bloque][0] = 1;
                                                    directorio1[bloque][1] = 1;
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio1[bloque][2] = 0;
                                                    directorio1[bloque][0] = 1;
                                                    directorio1[bloque][1] = 1;
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio1[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 1, 2); // trae el dato a su cache
                                                    cacheDatos2[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio1[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio1[bloque][0] = 1; //pone bloque en modificado
                                                    directorio1[bloque][1] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos1[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 1, 3); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio1[bloque][3] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio1[bloque][0] = 1; //pone bloque en modificado
                                                    directorio1[bloque][1] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos1[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                        }else{
                            if(bloque < 16){ //si el bloque está en el directorio2
                                if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio2[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio2[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio2[bloque][1] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio2[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio2[bloque][2] = 1 && directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                mlocks[2] = 1;
                                                mlocks[3] = 1;
                                                cacheDatos2[seccion][5] = 2;
                                                directorio2[bloque][2] = 0;
                                                cacheDatos3[seccion][5] = 2;
                                                directorio2[bloque][3] = 0;
                                               
                                                directorio2[bloque][0] = 1;
                                                directorio2[bloque][1] = 1;
                                                cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio2[bloque][3] = 0;
                                                    directorio2[bloque][0] = 1;
                                                    directorio2[bloque][1] = 1;
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio2[bloque][2] = 0;
                                                    directorio2[bloque][0] = 1;
                                                    directorio2[bloque][1] = 1;
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio2[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 1, 2); // trae el dato a su cache
                                                    cacheDatos2[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio2[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio2[bloque][0] = 1; //pone bloque en modificado
                                                    directorio2[bloque][1] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos1[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 1, 3); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio2[bloque][3] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio2[bloque][0] = 1; //pone bloque en modificado
                                                    directorio2[bloque][1] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos1[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                            }else { //si el bloque est en directorio3
                                if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio3[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio3[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio3[bloque][1] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio3[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio3[bloque][2] = 1 && directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                mlocks[2] = 1;
                                                mlocks[3] = 1;
                                                cacheDatos2[seccion][5] = 2;
                                                directorio3[bloque][2] = 0;
                                                cacheDatos3[seccion][5] = 2;
                                                directorio3[bloque][3] = 0;
                                               
                                                directorio3[bloque][0] = 1;
                                                directorio3[bloque][1] = 1;
                                                cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio3[bloque][3] = 0;
                                                    directorio3[bloque][0] = 1;
                                                    directorio3[bloque][1] = 1;
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio3[bloque][2] = 0;
                                                    directorio3[bloque][0] = 1;
                                                    directorio3[bloque][1] = 1;
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio3[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 1, 2); // trae el dato a su cache
                                                    cacheDatos2[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio3[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio3[bloque][0] = 1; //pone bloque en modificado
                                                    directorio3[bloque][1] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos3[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 1, 3); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio3[bloque][3] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio3[bloque][0] = 1; //pone bloque en modificado
                                                    directorio3[bloque][1] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos1[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos1[seccion][palabraBloque] = reg1[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                            }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                        }
                    }
                }    
            
            }
        }
            break;
        }
        case 2:{
            while(siga){
                if(pthread_mutex_trylock(&mCacheDatos2) == 0) { 
                       mlocks[2] = 1;
                       if((cacheDatos2[seccion][4] == bloque) && (cacheDatos2[seccion][5] == 1)){ //cuando el bloque esta en cache y estado "M"
                           
                           cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                           mlocks = liberarRecursos(mlocks);
                           siga = false;
                       }
                }else{
                    if((cacheDatos2[seccion][4] == bloque) && (cacheDatos2[seccion][5] == 0)){ //cuando el bloque esta en cache y estado "C"
                        if(bloque < 8){ //si el bloque est en directorio1
                            
                            if(pthread_mutex_trylock(&mDirectorio1) == 0){
                                mlocks[4] = 1;
                                
                                if(directorio1[bloque][1] = 1 && directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos1) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                        mlocks[2] = 1;
                                        mlocks[3] = 1;
                                        cacheDatos1[seccion][5] = 2;
                                        directorio1[bloque][1] = 0;
                                        cacheDatos3[seccion][5] = 2;
                                        directorio1[bloque][3] = 0;
                                       
                                        directorio1[bloque][0] = 1;
                                        directorio1[bloque][2] = 1;
                                        cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                            mlocks[3] = 1;
                                            cacheDatos3[seccion][5] = 2;
                                            directorio1[bloque][3] = 0;
                                            directorio1[bloque][0] = 1;
                                            directorio1[bloque][2] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio1[bloque][2] = 0;
                                            directorio1[bloque][0] = 1;
                                            directorio1[bloque][2] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }else{
                            if(bloque < 16){ //si el bloque está en el directorio2
                                if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                mlocks[4] = 1;
                                
                                if(directorio2[bloque][1] = 1 && directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                        mlocks[1] = 1;
                                        mlocks[3] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio2[bloque][1] = 0;
                                        cacheDatos3[seccion][5] = 2;
                                        directorio2[bloque][3] = 0;
                                       
                                        directorio2[bloque][0] = 1;
                                        directorio2[bloque][2] = 1;
                                        cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                            mlocks[3] = 1;
                                            cacheDatos3[seccion][5] = 2;
                                            directorio2[bloque][3] = 0;
                                            directorio2[bloque][0] = 1;
                                            directorio2[bloque][2] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                            mlocks[1] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio2[bloque][1] = 0;
                                            directorio2[bloque][0] = 1;
                                            directorio2[bloque][2] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }else{ // si esta en directorio 3
                            if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                mlocks[6] = 1;
                                
                                if(directorio3[bloque][1] = 1 && directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos1) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                        mlocks[1] = 1;
                                        mlocks[3] = 1;
                                        cacheDatos1[seccion][5] = 2;
                                        directorio3[bloque][1] = 0;
                                        cacheDatos3[seccion][5] = 2;
                                        directorio3[bloque][3] = 0;
                                       
                                        directorio3[bloque][0] = 1;
                                        directorio3[bloque][2] = 1;
                                        cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                            mlocks[3] = 1;
                                            cacheDatos3[seccion][5] = 2;
                                            directorio3[bloque][3] = 0;
                                            directorio3[bloque][0] = 1;
                                            directorio3[bloque][2] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                            mlocks[1] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio3[bloque][1] = 0;
                                            directorio3[bloque][0] = 1;
                                            directorio3[bloque][2] = 1;
                                            cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }    
                }else{ // si el bloque no esta en la cache o si esta invalido
                    if(bloque < 8){ //si el bloque est en directorio1
                            if(pthread_mutex_trylock(&mDirectorio1) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio1[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio1[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio1[bloque][2] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio1[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio1[bloque][1] = 1 && directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos1) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                mlocks[1] = 1;
                                                mlocks[3] = 1;
                                                cacheDatos1[seccion][5] = 2;
                                                directorio1[bloque][1] = 0;
                                                cacheDatos3[seccion][5] = 2;
                                                directorio1[bloque][3] = 0;
                                               
                                                directorio1[bloque][0] = 1;
                                                directorio1[bloque][2] = 1;
                                                cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio1[bloque][3] = 0;
                                                    directorio1[bloque][0] = 1;
                                                    directorio1[bloque][2] = 1;
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1;
                                                    cacheDatos1[seccion][5] = 2;
                                                    directorio1[bloque][1] = 0;
                                                    directorio1[bloque][0] = 1;
                                                    directorio1[bloque][2] = 1;
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio1[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 2, 1); // trae el dato a su cache
                                                    cacheDatos1[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio1[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio1[bloque][0] = 1; //pone bloque en modificado
                                                    directorio1[bloque][2] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos2[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 2, 3); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio1[bloque][3] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio1[bloque][0] = 1; //pone bloque en modificado
                                                    directorio1[bloque][2] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos2[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                        }else{
                            if(bloque < 16){ //si el bloque está en el directorio2
                                if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio2[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio2[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio2[bloque][2] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio2[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio2[bloque][1] = 1 && directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                mlocks[1] = 1;
                                                mlocks[3] = 1;
                                                cacheDatos1[seccion][5] = 2;
                                                directorio2[bloque][1] = 0;
                                                cacheDatos3[seccion][5] = 2;
                                                directorio2[bloque][3] = 0;
                                               
                                                directorio2[bloque][0] = 1;
                                                directorio2[bloque][2] = 1;
                                                cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio2[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio2[bloque][3] = 0;
                                                    directorio2[bloque][0] = 1;
                                                    directorio2[bloque][2] = 1;
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio2[bloque][3] = 0;
                                                    directorio2[bloque][0] = 1;
                                                    directorio2[bloque][2] = 1;
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio2[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 2, 1); // trae el dato a su cache
                                                    cacheDatos1[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio2[bloque][1] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio2[bloque][0] = 1; //pone bloque en modificado
                                                    directorio2[bloque][2] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos2[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 2, 3); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio2[bloque][3] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio2[bloque][0] = 1; //pone bloque en modificado
                                                    directorio2[bloque][2] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos2[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                            }else { //si el bloque est en directorio3
                                if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio3[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio3[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio3[bloque][2] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio3[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio3[bloque][2] = 1 && directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                mlocks[1] = 1;
                                                mlocks[3] = 1;
                                                cacheDatos1[seccion][5] = 2;
                                                directorio3[bloque][2] = 0;
                                                cacheDatos3[seccion][5] = 2;
                                                directorio3[bloque][3] = 0;
                                               
                                                directorio3[bloque][0] = 1;
                                                directorio3[bloque][1] = 1;
                                                cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio3[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio3[bloque][3] = 0;
                                                    directorio3[bloque][0] = 1;
                                                    directorio3[bloque][2] = 1;
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio3[bloque][1] = 0;
                                                    directorio3[bloque][0] = 1;
                                                    directorio3[bloque][2] = 1;
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio3[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 2, 1); // trae el dato a su cache
                                                    cacheDatos1[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio3[bloque][1] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio3[bloque][0] = 1; //pone bloque en modificado
                                                    directorio3[bloque][2] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos2[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    mlocks[3] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 2, 3); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio3[bloque][3] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio3[bloque][0] = 1; //pone bloque en modificado
                                                    directorio3[bloque][2] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos2[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos2[seccion][palabraBloque] = reg2[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                            }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                        }
                    }
                }    
            
            }
        }
            break;
        }
        case 3:{
            while(siga){
                if(pthread_mutex_trylock(&mCacheDatos3) == 0) { 
                       mlocks[3] = 1;
                       if((cacheDatos3[seccion][4] == bloque) && (cacheDatos3[seccion][5] == 1)){ //cuando el bloque esta en cache y estado "M"
                           
                           cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                           mlocks = liberarRecursos(mlocks);
                           siga = false;
                       }
                }else{
                    if((cacheDatos3[seccion][4] == bloque) && (cacheDatos3[seccion][5] == 0)){ //cuando el bloque esta en cache y estado "C"
                        if(bloque < 8){ //si el bloque est en directorio1
                            
                            if(pthread_mutex_trylock(&mDirectorio1) == 0){
                                mlocks[4] = 1;
                                
                                if(directorio1[bloque][2] = 1 && directorio1[bloque][1] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos1) == 0){
                                        mlocks[2] = 1;
                                        mlocks[1] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio1[bloque][2] = 0;
                                        cacheDatos1[seccion][5] = 2;
                                        directorio1[bloque][1] = 0;
                                       
                                        directorio1[bloque][0] = 1;
                                        directorio1[bloque][3] = 1;
                                        cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio1[bloque][1] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                            mlocks[1] = 1;
                                            cacheDatos1[seccion][5] = 2;
                                            directorio1[bloque][1] = 0;
                                            directorio1[bloque][0] = 1;
                                            directorio1[bloque][3] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio1[bloque][2] = 0;
                                            directorio1[bloque][0] = 1;
                                            directorio1[bloque][3] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }else{
                            if(bloque < 16){ //si el bloque está en el directorio2
                                if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                mlocks[4] = 1;
                                
                                if(directorio2[bloque][2] = 1 && directorio2[bloque][1] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos1) == 0){
                                        mlocks[2] = 1;
                                        mlocks[1] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio2[bloque][2] = 0;
                                        cacheDatos1[seccion][5] = 2;
                                        directorio2[bloque][1] = 0;
                                       
                                        directorio2[bloque][0] = 1;
                                        directorio2[bloque][3] = 1;
                                        cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio2[bloque][1] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                            mlocks[1] = 1;
                                            cacheDatos1[seccion][5] = 2;
                                            directorio2[bloque][1] = 0;
                                            directorio2[bloque][0] = 1;
                                            directorio2[bloque][3] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio2[bloque][2] = 0;
                                            directorio2[bloque][0] = 1;
                                            directorio2[bloque][3] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }else{ // si esta en directorio 3
                            if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                mlocks[6] = 1;
                                
                                if(directorio3[bloque][2] = 1 && directorio3[bloque][1] = 1){ //cuando el bloque esta compartido con en cache2
                                    if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos1) == 0){
                                        mlocks[2] = 1;
                                        mlocks[1] = 1;
                                        cacheDatos2[seccion][5] = 2;
                                        directorio3[bloque][2] = 0;
                                        cacheDatos1[seccion][5] = 2;
                                        directorio3[bloque][1] = 0;
                                       
                                        directorio3[bloque][0] = 1;
                                        directorio3[bloque][3] = 1;
                                        cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                        mLocks = liberarRecursos(mlocks);
                                siga = false;
                                    }else{
                                        mlocks = liberarRecursos(mlocks);
                                    }
                                } else {
                                    if(directorio3[bloque][1] = 1){ //cuando el bloque esta compartido con en cache3
                                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                            mlocks[1] = 1;
                                            cacheDatos1[seccion][5] = 2;
                                            directorio3[bloque][1] = 0;
                                            directorio3[bloque][0] = 1;
                                            directorio3[bloque][3] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    } else {
                                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                            mlocks[2] = 1;
                                            cacheDatos2[seccion][5] = 2;
                                            directorio3[bloque][2] = 0;
                                            directorio3[bloque][0] = 1;
                                            directorio3[bloque][3] = 1;
                                            cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                            mLocks = liberarRecursos(mlocks);
                                            siga = false;
                                        }else{
                                            mlocks = liberarRecursos(mlocks);
                                        }
                                    }
                                }
                                
                            }else{
                                mlocks = liberarRecursos(mlocks);  
                            }
                        }    
                }else{ // si el bloque no esta en la cache o si esta invalido
                    if(bloque < 8){ //si el bloque est en directorio1
                            if(pthread_mutex_trylock(&mDirectorio1) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio1[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio1[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio1[bloque][3] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio1[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio1[bloque][2] = 1 && directorio1[bloque][1] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                mlocks[2] = 1;
                                                mlocks[1] = 1;
                                                cacheDatos2[seccion][5] = 2;
                                                directorio1[bloque][2] = 0;
                                                cacheDatos1[seccion][5] = 2;
                                                directorio1[bloque][1] = 0;
                                               
                                                directorio1[bloque][0] = 1;
                                                directorio1[bloque][3] = 1;
                                                cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio1[bloque][3] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1;
                                                    cacheDatos1[seccion][5] = 2;
                                                    directorio1[bloque][1] = 0;
                                                    directorio1[bloque][0] = 1;
                                                    directorio1[bloque][3] = 1;
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio1[bloque][2] = 0;
                                                    directorio1[bloque][0] = 1;
                                                    directorio1[bloque][3] = 1;
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio1[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 3, 2); // trae el dato a su cache
                                                    cacheDatos2[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio1[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio1[bloque][0] = 1; //pone bloque en modificado
                                                    directorio1[bloque][3] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos3[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 3, 1); // trae el dato a su cache
                                                    cacheDatos1[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio1[bloque][1] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio1[bloque][0] = 1; //pone bloque en modificado
                                                    directorio1[bloque][3] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos3[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                        }else{
                            if(bloque < 16){ //si el bloque está en el directorio2
                                if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio2[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio2[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio2[bloque][1] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio2[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio2[bloque][2] = 1 && directorio2[bloque][1] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                mlocks[2] = 1;
                                                mlocks[1] = 1;
                                                cacheDatos2[seccion][5] = 2;
                                                directorio2[bloque][2] = 0;
                                                cacheDatos1[seccion][5] = 2;
                                                directorio2[bloque][1] = 0;
                                               
                                                directorio2[bloque][0] = 1;
                                                directorio2[bloque][3] = 1;
                                                cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio2[bloque][1] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1;
                                                    cacheDatos1[seccion][5] = 2;
                                                    directorio2[bloque][1] = 0;
                                                    directorio2[bloque][0] = 1;
                                                    directorio2[bloque][3] = 1;
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio2[bloque][2] = 0;
                                                    directorio2[bloque][0] = 1;
                                                    directorio2[bloque][3] = 1;
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio2[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 3, 2); // trae el dato a su cache
                                                    cacheDatos2[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio2[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio2[bloque][0] = 1; //pone bloque en modificado
                                                    directorio2[bloque][3] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos1[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 3, 1); // trae el dato a su cache
                                                    cacheDatos1[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio2[bloque][1] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio2[bloque][0] = 1; //pone bloque en modificado
                                                    directorio2[bloque][3] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos3[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                            }else { //si el bloque est en directorio3
                                if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                mlocks[4] = 1;
                                // estado 0 = "C", 1 = "M", 2 = "U"
                                if(directorio3[bloque][0] = 2){ //el bloque este uncached
                                    falloCacheDatos(bloque, dir, seccion, id_hilo);
                                    directorio3[bloque][0] = 1; // Pone  estado del bloque a modificado en el direcotrio
                                    directorio3[bloque][1] = 1; // Pone modificado en el directorio para cpu1
                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                    mLocks = liberarRecursos(mLocks);  
                                    siga = false;
                                }else{
                                    if(directorio3[bloque][0] = 0){ //si el bloque esta compartido
                                        falloCacheDatos(bloque, dir, seccion, id_hilo);
                                        if(directorio3[bloque][2] = 1 && directorio3[bloque][1] = 1){ //cuando el bloque esta compartido con en cache2
                                            if(pthread_mutex_trylock(&mCacheDatos2) == 0 && pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                mlocks[2] = 1;
                                                mlocks[1] = 1;
                                                cacheDatos2[seccion][5] = 2;
                                                directorio3[bloque][2] = 0;
                                                cacheDatos1[seccion][5] = 2;
                                                directorio3[bloque][1] = 0;
                                               
                                                directorio3[bloque][0] = 1;
                                                directorio3[bloque][3] = 1;
                                                cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                mLocks = liberarRecursos(mlocks);
                                        siga = false;
                                            }else{
                                                mlocks = liberarRecursos(mlocks);
                                            }
                                        } else {
                                            if(directorio3[bloque][1] = 1){ //cuando el bloque esta compartido con en cache3
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1;
                                                    cacheDatos3[seccion][5] = 2;
                                                    directorio3[bloque][1] = 0;
                                                    directorio3[bloque][0] = 1;
                                                    directorio3[bloque][3] = 1;
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            } else {
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1;
                                                    cacheDatos2[seccion][5] = 2;
                                                    directorio3[bloque][2] = 0;
                                                    directorio3[bloque][0] = 1;
                                                    directorio3[bloque][3] = 1;
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                    mlocks = liberarRecursos(mlocks);
                                                }
                                            }
                                        }
                                    }else{ //cuando el bloque esta modificado
                                        if(directorio3[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    mlocks[2] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 3, 2); // trae el dato a su cache
                                                    cacheDatos2[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio3[bloque][2] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio3[bloque][0] = 1; //pone bloque en modificado
                                                    directorio3[bloque][3] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos3[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                        }else{ //el bloque este modificado en cpu3
                                            if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    mlocks[1] = 1
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3); // guarda el dato en memoria antes de invalidarlo
                                                    guardaCacheDatosCache(bloque, seccion, 3, 1); // trae el dato a su cache
                                                    cacheDatos3[seccion][5] = 2; //estado en cache 2 invalido
                                                    directorio3[bloque][1] = 0; //indica que cache2 no tiene ese bloque
                                                    directorio3[bloque][0] = 1; //pone bloque en modificado
                                                    directorio3[bloque][3] = 1; // indica que la cache 1 tiene el bloque
                                                    cacheDatos3[seccion][5] = 1; //pone estado en cache1 modificado
                                                    cacheDatos3[seccion][palabraBloque] = reg3[palabra[2]];  
                                                    mLocks = liberarRecursos(mlocks);
                                                    siga = false;
                                                }else{
                                                   mLocks = liberarRecursos(mlocks); 
                                                }
                                            }
                                        }
                                    }
                                }
                            }else{
                                mLocks = liberarRecursos(mLocks);     
                            }
                        }
                    }
                }    
            
            }
        }
            break;
        }
    }
    break;
}
*/

// Método que procesa la palabra con su respectiva instrucción
// Entra en el switch que le corresponde y realiza los cambios en los registros como sea necesario
void procesarPalabra(vector<int> palabra, int id_hilo) {
    //aumento de PC
    switch (id_hilo) {
                case 1:{
                    pc1 += 4;
                    break;
                 }
                case 2:{
                    pc2+=4;
                    break;
                }
                case 3:{
                    pc3+=4;
                    break;
                }
    }
    switch (palabra[0] )
        {
         case 8:{
            //DADDI; RX, RY, #n; Rx <-- (Ry) + n
            switch (id_hilo) {
                case 1:{
                    reg1[palabra[2]] = reg1[palabra[1]] + palabra[3];
                    break;
                 }
                case 2:{
                    reg2[palabra[2]] = reg2[palabra[1]] + palabra[3];
                    break;
                }
                case 3:{
                    reg3[palabra[2]] = reg3[palabra[1]] + palabra[3];
                    break;
                }
            
            }
            break;
        }
        case 32:{
            // DADD; RX, RY, RZ; Rx <-- (Ry) + (Rz)
            switch (id_hilo) {
                case 1:{
                    reg1[palabra[3]] = reg1[palabra[1]] + reg1[palabra[2]];
                    break;
                }
                case 2:{
                    reg2[palabra[3]] = reg2[palabra[1]] + reg2[palabra[2]];
                    break;
                }
                case 3:{
                    reg3[palabra[3]] = reg3[palabra[1]] + reg3[palabra[2]];
                    break;
                }
            }
            break;
        }
        case 34:{
            // DSUB; RX, RY, RZ; Rx <-- (Ry) - (Rz)
            switch (id_hilo) {
                case 1:{
                    reg1[palabra[3]] = reg1[palabra[1]] - reg1[palabra[2]];
                    break;
                }
                case 2:{
                    reg2[palabra[3]] = reg2[palabra[1]] - reg2[palabra[2]];
                    break;
                }
                case 3:{
                    reg3[palabra[3]] = reg3[palabra[1]] - reg3[palabra[2]];
                    break;
                }
            }
            break;
        }
        case 12:{
            // DMUL; RX, RY, RZ; Rx <-- (Ry) * (Rz)
            switch (id_hilo) {
                case 1:{
                    reg1[palabra[3]] = reg1[palabra[1]] * reg1[palabra[2]];
                    break;
                }
                case 2:{
                    reg2[palabra[3]] = reg2[palabra[1]] * reg2[palabra[2]];
                    break;
                }
                case 3:{
                    reg3[palabra[3]] = reg3[palabra[1]] * reg3[palabra[2]];
                    break;
                }
            }
            break;
        }
        case 14:{
            // DDIV; RX, RY, RZ; Rx <-- (Ry) / (Rz)
            switch (id_hilo) {
                case 1:{
                    reg1[palabra[3]] = reg1[palabra[1]] / reg1[palabra[2]];
                    break;
                }
                case 2:{
                    reg2[palabra[3]] = reg2[palabra[1]] / reg2[palabra[2]];
                    break;
                }
                case 3:{
                    reg3[palabra[3]] = reg3[palabra[1]] / reg3[palabra[2]];
                    break;
                }
            }
            break;
        }
        case 4:{
            // BEQZ; RX, ETIQ; Si Rx = 0 SALTA
            int salto = palabra[3] * 4;
            switch (id_hilo) {
                case 1:{
                    if(reg1[palabra[1]]==0){
                        pc1+=salto;
                    }
                    break;
                }
                case 2:{
                    if(reg2[palabra[1]]==0){
                        pc2+=salto;
                    }
                    break;
                }
                case 3:{
                    if(reg3[palabra[1]]==0){
                        pc3+=salto;
                    }
                    break;
                }
            }
            break;
        }
        case 5:{
            //BNEZ; RX, ETIQ; Si Rx <> 0 SALTA
            int salto = palabra[3] * 4;
            switch (id_hilo) {
                case 1:{
                    if(reg1[palabra[1]]!=0){
                        pc1+=salto;
                    }
                    break;
                 }
                case 2:{
                    if(reg2[palabra[1]]!=0){
                        pc2+=salto;
                    }
                    break;
                }
                case 3:{
                    if(reg3[palabra[1]]!=0){
                        pc3+=salto;
                    }
                    break;
                }
            }
            break;
        }
        case 3:{
            // JAL; n; R31<--PC, PC<-- PC+n
            switch (id_hilo) {
                case 1:{
                    reg1[31] = pc1;
                    pc1 += palabra[3]; 
                    break;
                }
                case 2:{
                    reg2[31] = pc2;
                    pc2 += palabra[3]; 
                    break;
                }
                case 3:{
                    reg3[31] = pc3;
                    pc3 += palabra[3]; 
                    break;
                }
            }
            break;
        }
        case 2:{
            // JR; RX; PC <-- (Rx)
            switch (id_hilo) {
                case 1:{
                    pc1 = reg1[palabra[1]]; 
                    break;
                }
                case 2:{
                    pc2 = reg2[palabra[1]]; 
                    break;
                }
                case 3:{
                    pc3 = reg3[palabra[1]];  
                    break;
                }
            }
            break;
        }
        
        /*****************************************/
        /* REALIZAR EL LOAD PARA HABLAR EL JUEVES*/
        /*****************************************/
        case 35:{
            // LW; RX, n(RY) Rx <-- M(n + (Ry)) 
            // estado 0 = "C", 1 = "M", 2 = "I"
            cout<< "entra al load";
            loadWord(id_hilo,palabra);
            
            /*switch (id_hilo) {
            
                case 1:{
                    int bloque, palabraInterna, dir, seccion;
                    bool pasoInseguro = true;
                    
                    dir = reg1[palabra[1]] + palabra[3];
                    bloque = dir/16;
                    palabraInterna = dir%16;
                    dir = dir / 4;
                    seccion = bloque%4;  
                    while(pasoInseguro){
                        if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                            if((cacheDatos1[seccion][4] == bloque) && (cacheDatos1[seccion][5] != 2)){ //si el bloque esta en la cache o el estado es M o C se lee la palabra directamene
                                   reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna];
                                   pasoInseguro = false;
                                   pthread_mutex_unlock(&mCacheDatos1); // libera cache1
                            }else{
                                
                                if(bloque < 7){ //si el numero de bloque es menor que 7 esta en la memoria del CPU1
                                    if(pthread_mutex_trylock(&mDirectorio1) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(cacheDatos1[seccion][5] == 1){ //si el dato de la cache al que se le va a caer encima esta modificado este lo guarda
                                    
                                            if(cacheDatos1[seccion][4] < 8){
                                                guardaCacheDatosMem(cacheDatos1[seccion][4], dir, seccion, 1); // Guarda en memoria el dato para no perder la información
                                                directorio1[bloque][0] = 2; //pone compartido el bloque que se esta trayendo de memoria
                                                directorio1[bloque][1] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            }else{
                                                if(cacheDatos1[seccion][4] < 16){
                                                    if(pthread_mutex_trylock(&mDirectorio2) == 0){
                                                        guardaCacheDatosMem(cacheDatos1[seccion][4], dir, seccion, 1); // Guarda en memoria el dato para no perder la información
                                                        directorio2[bloque][0] = 2; //pone compartido el bloque que se esta trayendo de memoria
                                                        directorio2[bloque][1] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                        
                                                    }else{
                                                        pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                        pthread_mutex_unlock(&mDirectorio1); //libera direcorio
                                                    }
                                                } else {
                                                    if(pthread_mutex_trylock(&mDirectorio3) == 0){
                                                        guardaCacheDatosMem(cacheDatos11[seccion][4], dir, seccion, 1); // Guarda en memoria el dato para no perder la información
                                                        directorio3[bloque][0] = 2; //pone compartido el bloque que se esta trayendo de memoria
                                                        directorio3[bloque][1] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                        
                                                    } else {
                                                        pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                        pthread_mutex_unlock(&mDirectorio1); //libera direcorio
                                                    }
                                                }
                                            }
                                            
                                        }
                                        if(directorio1[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio1[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                            pthread_mutex_unlock(&mDirectorio1); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio1[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2);
                                                    guardaCacheDatosCache(bloque, seccion, 1, 2);
                                                    directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio1[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio1); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos2); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos1);
                                                    pthread_mutex_unlock(&mDirectorio1);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3);
                                                    guardaCacheDatosCache(bloque, seccion, 1, 3);
                                                    directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio1[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio1); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos3); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos1);
                                                    pthread_mutex_unlock(&mDirectorio1);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos1);
                                    }
                                }else{
                                    if(bloque < 15){ //si el numero de bloque esta entre que 8 y 15 esta en la memoria del CPU2
                                        dir = dir - 32; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        bloque = bloque - 8; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        
                                        if(pthread_mutex_trylock(&mDirectorio2) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio2[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio2[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                            pthread_mutex_unlock(&mDirectorio2); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio2[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2);
                                                    guardaCacheDatosCache(bloque, seccion, 1, 2);
                                                    directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio2[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio2); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos2); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos1);
                                                    pthread_mutex_unlock(&mDirectorio2);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3);
                                                    guardaCacheDatosCache(bloque, seccion, 1, 3);
                                                    directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio2[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio2); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos3); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos1);
                                                    pthread_mutex_unlock(&mDirectorio2);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos1);
                                    }
                                    }else{ //si el numero de bloque es mayor a 15 esta en la memoria del CPU3
                                        dir = dir - 64; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        bloque = bloque - 16;// Se resta la dirección para que coincida con los tamaños de la cache datos
                                        if(pthread_mutex_trylock(&mDirectorio3) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio3[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio3[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                            pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio3[bloque][2] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2);
                                                    guardaCacheDatosCache(bloque, seccion, 1, 2);
                                                    directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio3[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos1);
                                                    pthread_mutex_unlock(&mDirectorio3);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3);
                                                    guardaCacheDatosCache(bloque, seccion, 1, 3);
                                                    directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio3[bloque][1] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg1[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos1);
                                                    pthread_mutex_unlock(&mDirectorio3);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos1);
                                    }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                case 2:{
                    int bloque, palabraInterna, dir, seccion;
                    bool pasoInseguro = true;
                    
                    dir = reg2[palabra[1]] + palabra[3];
                    bloque = dir/16;  
                    dir = dir / 4;
                    palabraInterna = dir%16;
                    seccion = bloque%4;  
                    while(pasoInseguro){
                        if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                            if((cacheDatos2[seccion][4] == bloque) && (cacheDatos2[seccion][5] != 2)){ //si el bloque esta en la cache o el estado es M o C se lee la palabra directamene
                                   reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna];
                                   pasoInseguro = false;
                                   pthread_mutex_unlock(&mCacheDatos2); // libera cache1
                            }else{
                                if(bloque < 7){ //si el numero de bloque es menor que 7 esta en la memoria del CPU1
                                    if(pthread_mutex_trylock(&mDirectorio1) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio1[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio1[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                            pthread_mutex_unlock(&mDirectorio1); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio1[bloque][1] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1);
                                                    guardaCacheDatosCache(bloque, seccion, 2, 1);
                                                    directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio1[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio1); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos1); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos2);
                                                    pthread_mutex_unlock(&mDirectorio1);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3);
                                                    guardaCacheDatosCache(bloque, seccion, 2, 3);
                                                    directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio1[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio1); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos3); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos2);
                                                    pthread_mutex_unlock(&mDirectorio1);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos2);
                                    }
                                }else{
                                    if(bloque < 15){ //si el numero de bloque esta entre que 8 y 15 esta en la memoria del CPU2
                                        dir = dir - 32; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        bloque = bloque - 8; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        if(pthread_mutex_trylock(&mDirectorio2) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio2[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio2[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                            pthread_mutex_unlock(&mDirectorio2); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio2[bloque][1] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1);
                                                    guardaCacheDatosCache(bloque, seccion, 2, 1);
                                                    directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio2[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio2); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos1); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos2);
                                                    pthread_mutex_unlock(&mDirectorio2);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3);
                                                    guardaCacheDatosCache(bloque, seccion, 2, 3);
                                                    directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio2[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio2); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos3); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos2);
                                                    pthread_mutex_unlock(&mDirectorio2);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos2);
                                    }
                                    }else{ //si el numero de bloque es mayor a 15 esta en la memoria del CPU3
                                        dir = dir - 64; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        bloque = bloque - 16; // Se resta la dirección para que coincida con los tamaños de la cache datos
                                        if(pthread_mutex_trylock(&mDirectorio3) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio3[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio3[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                            pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio3[bloque][1] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1);
                                                    guardaCacheDatosCache(bloque, seccion, 2, 1);
                                                    directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio3[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg2[palabra[2]] = cacheDatos1[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos2);
                                                    pthread_mutex_unlock(&mDirectorio3);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 3);
                                                    guardaCacheDatosCache(bloque, seccion, 2, 3);
                                                    directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio3[bloque][2] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg2[palabra[2]] = cacheDatos2[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos2);
                                                    pthread_mutex_unlock(&mDirectorio3);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos2);
                                    }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                case 3:{
                    int bloque, palabraInterna, dir, seccion;
                    bool pasoInseguro = true;
                    
                    dir = reg3[palabra[1]] + palabra[3];
                    bloque = dir/16;  
                    dir = dir / 4;
                    palabraInterna = dir%16;
                    seccion = bloque%4;  
                    while(pasoInseguro){
                        if(pthread_mutex_trylock(&mCacheDatos3) == 0){
                            if((cacheDatos3[seccion][4] == bloque) && (cacheDatos3[seccion][5] != 2)){ //si el bloque esta en la cache o el estado es M o C se lee la palabra directamene
                                   reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna];
                                   pasoInseguro = false;
                                   pthread_mutex_unlock(&mCacheDatos3); // libera cache1
                            }else{
                                if(bloque < 7){ //si el numero de bloque es menor que 7 esta en la memoria del CPU1
                                    
                                    if(pthread_mutex_trylock(&mDirectorio1) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio1[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio1[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                            pthread_mutex_unlock(&mDirectorio1); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio1[bloque][1] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1);
                                                    guardaCacheDatosCache(bloque, seccion, 3, 1);
                                                    directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio1[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio1); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos1); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos3);
                                                    pthread_mutex_unlock(&mDirectorio1);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2);
                                                    guardaCacheDatosCache(bloque, seccion, 3, 2);
                                                    directorio1[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio1[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio1); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos2); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos3);
                                                    pthread_mutex_unlock(&mDirectorio1);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos3);
                                    }
                                }else{
                                    if(bloque < 15){ //si el numero de bloque esta entre que 8 y 15 esta en la memoria del CPU2
                                    dir = dir - 32;// Se resta la dirección para que coincida con los tamaños de la cache datos
                                    bloque = bloque - 8;// Se resta la dirección para que coincida con los tamaños de la cache datos
                                        if(pthread_mutex_trylock(&mDirectorio2) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio2[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio2[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                            pthread_mutex_unlock(&mDirectorio2); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio2[bloque][1] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1);
                                                    guardaCacheDatosCache(bloque, seccion, 3, 1);
                                                    directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio2[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio2); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos1); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos3);
                                                    pthread_mutex_unlock(&mDirectorio2);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2);
                                                    guardaCacheDatosCache(bloque, seccion, 3, 2);
                                                    directorio2[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio2[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio2); // libera directorio
                                                    pthread_mutex_unlock(&mCacheDatos2); // libera Cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos3);
                                                    pthread_mutex_unlock(&mDirectorio2);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos3);
                                    }
                                    }else{ //si el numero de bloque es mayor a 15 esta en la memoria del CPU3
                                        dir = dir - 64;// Se resta la dirección para que coincida con los tamaños de la cache datos
                                        bloque = bloque - 16;// Se resta la dirección para que coincida con los tamaños de la cache datos
                                        if(pthread_mutex_trylock(&mDirectorio3) == 0){ //se trata de bloquear el directorio 1
                                        // estadodirectorios 0 = "C", 1 = "M", 2 = "U" 
                                        if(directorio3[bloque][0] != 1){ //si el bloque esta compartido o uncached se trae de memoria
                                            
                                            falloCacheDatos(bloque, dir, seccion, id_hilo); //se trae el dato de memoria
                                            directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                            directorio3[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                            cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                            reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna];
                                            pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                            pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                            pasoInseguro = false;
                                            
                                        }else{ //si el bloque esta modificado hay que traerlo de la cache del que tiene el dato modificado
                                            if(directorio3[bloque][1] == 1){ //el bloque este modificaco en cache cpu2
                                                if(pthread_mutex_trylock(&mCacheDatos1) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 2
                                                    guardaCacheDatosMem(bloque, dir, seccion, 1);
                                                    guardaCacheDatosCache(bloque, seccion, 3, 1);
                                                    directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio3[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos1[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                                    pthread_mutex_unlock(&mCacheDatos1); //libera cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos3);
                                                    pthread_mutex_unlock(&mDirectorio3);
                                                }    
                                            }else{//el bloque este modificaco en cache cpu3
                                                if(pthread_mutex_trylock(&mCacheDatos2) == 0){
                                                    
                                                    //se guardan los datos a memoria y se traen a la cache 3
                                                    guardaCacheDatosMem(bloque, dir, seccion, 2);
                                                    guardaCacheDatosCache(bloque, seccion, 3, 2);
                                                    directorio3[bloque][0] = 0; //pone compartido el bloque que se esta trayendo de memoria
                                                    directorio3[bloque][3] = 1; //pone compartido el bloque que se esta trayendo de memoria
                                                    cacheDatos3[seccion][5] = 0; // se pone compartido el bloque en la cache de datos
                                                    cacheDatos2[seccion][5] = 0; // se pone compartido el bloque en la cache de datos donde estaba el dato modificaco
                                                    reg3[palabra[2]] = cacheDatos3[seccion][palabraInterna]; // se carga la palabra al registro
                                                    pthread_mutex_unlock(&mCacheDatos3); //libera cache
                                                    pthread_mutex_unlock(&mDirectorio3); //libera direcorio
                                                    pthread_mutex_unlock(&mCacheDatos2); //libera cache
                                                    pasoInseguro = false;
                                                    
                                                }else{  //como no se obtiene el recurso se libera lo que estaba siendo usado
                                                    pthread_mutex_unlock(&mCacheDatos3);
                                                    pthread_mutex_unlock(&mDirectorio3);
                                                }
                                            }
                                        }
                                    }else{ //como no se obtiene el recurso se libera lo que estaba siendo usado
                                        pthread_mutex_unlock(&mCacheDatos3);
                                    }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }*/
            break;
        }
        case 43:{
            // SW; RX, n(RY) M(n + (Ry)) <-- Rx
            // NOTA 1: FALTA EDITAR TODO A PARTIR DE AQUI
            //storeWord(id_hilo, palabra); // Llama al método que tiene toda la lógica del SW
            
            break;
        }
        case 63:{
            // FIN Detiene el programa
            switch (id_hilo) {
                case 1:{
                    estado1 = 0;
                    break;
                }
                case 2:{
                    estado2 = 0;
                    break;
                }
                case 3:{
                    estado3 = 0;
                    break;
                }
            }
            break;
         }
     }
     cuentaIns(id_hilo);
}

// Chequea si a un CPU le quedan hilos sin terminar
bool checkContextos(int id_hilo){
    bool check = false; //retorna true si aun hay contextos
    
    switch (id_hilo) {
        case 1:{
            for(int i = 0; i < 4; i++) { // Revisa hilo por hilo en los contextos si queda alguno sin terminar
                if(contextos1[i][33] == 1) {
                    check = true;
                }
            }
            break;
        }
        case 2:{
            for(int i = 0; i < 4; i++) { // Revisa hilo por hilo en los contextos si queda alguno sin terminar
                if(contextos2[i][33] == 1) {
                    check = true;
                }
            }
            break;
        }
        case 3:{
            for(int i = 0; i < 4; i++) { // Revisa hilo por hilo en los contextos si queda alguno sin terminar
                if(contextos3[i][33] == 1) {
                    check = true;
                }
            }
            break;
        }
    }
    return check;
}

void cargarHilos(int hilos){
    int contHilos = 0; // Conteo de los hilos que se van a insertar a los CPU's
    int procesador = 1; // Variable que contiene el procesador actual al cual se le agregará un hilo
    int contMenInt1 = pc1 = 128; // Contador para la inserción de instruccinoes en la memoria del CPU 1
    int contMenInt2 = pc2 = 128; // Contador para la inserción de instruccinoes en la memoria del CPU 2
    int contMenInt3 = pc3 = 128; // Contador para la inserción de instruccinoes en la memoria del CPU 3
    int contHilo1 = 0; // Contador para la inserción de hilos a la tabla de contextos del CPU 1
    int contHilo2 = 0; // Contador para la inserción de hilos a la tabla de contextos del CPU 2
    int contHilo3 = 0; // Contador para la inserción de hilos a la tabla de contextos del CPU 3
    
    // Variables para la lectura de los archivos
    DIR *d;
    char *p1,*p2;
    int ret;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while (((dir = readdir(d)) != NULL) && (contHilos < hilos)) // Recorre la cantidad de hilos que se le asign�� al iniciar el programa
        {
            p1=strtok(dir->d_name,".");
            p2=strtok(NULL,".");
            if(p2!=NULL)
            {
                ret=strcmp(p2,"txt"); // Chequea si el archivo es un "".txt"
                if(ret==0)
                {
                    contHilos++;
                    strcat (p1,".txt"); // Agregamos la extensión
                	ifstream file(p1);
                    string str; 
                        switch (procesador) {
                            
                        case 1:{
                            contextos1[contHilo1][0] = contMenInt1; // Guarda el PC para saber donde empieza el hilo en la memoria
                            contextos1[contHilo1][33] = 1; // indica que el proceso esta activo
                            contextos1[contHilo1][36] = contHilos; // Guarda el ID del hilo
                            contHilo1++; //aumento de ID del hilo
                            while (file >> str){ // recorre el archivo y guarda las instrucciones en la memoria principal
                                memPrin1[contMenInt1] = atoi(str.c_str());
                                contMenInt1++;
                            }
                            procesador = 2;
                            break;
                         }
                        case 2:{
                            contextos2[contHilo2][0] = contMenInt2; // Guarda el PC para saber donde empieza el hilo en la memoria
                            contextos2[contHilo2][33] = 1; // indica que el proceso esta activo
                            contextos2[contHilo2][36] = contHilos; // Guarda el ID del hilo
                            contHilo2++; //aumento de ID del hilo
                            while (file >> str){ // recorre el archivo y guarda las instrucciones en la memoria principal
                                memPrin2[contMenInt2] = atoi(str.c_str());
                                contMenInt2++;
                            }
                            procesador = 3;
                            break;
                        }
                        case 3:{
                            contextos3[contHilo3][0] = contMenInt3; // Guarda el PC para saber donde empieza el hilo en la memoria
                            contextos3[contHilo3][33] = 1;
                            contextos3[contHilo3][36] = contHilos;
                            contHilo3++;
                            while (file >> str){
                                memPrin3[contMenInt3] = atoi(str.c_str());
                                contMenInt3++;
                            }
                            procesador = 1;
                            break;
                        }
                    }
                }
            }
        }
        closedir(d);
    }
}

// Método principal que va a ejecutar cada CPU
void *CPU(void *param)
{
    // Sincroniza los hilos con el principal.
    pthread_barrier_wait(&barrier);
    
    int id_hilo = *((int*)(&param)); //Id del procesador
    vector<int> plb(4); // Arreglo donde se guarda el bloque buscado
    
    switch (id_hilo) {
        case 1:{
            cntxActual1 = 0; //el primer proceso que entra es el primero que sera ejecutado.
            contextos1[cntxActual1][34] = ciclo; // Guarda el inicio del primer ciclo en la tabla de contextos
            while(checkContextos(id_hilo) == true) {
                plb = buscarBloque(id_hilo); //Busca la palabra que esta en el PC1
                procesarPalabra(plb,id_hilo); //Procesa la palabra que esta en el plb.
                pthread_barrier_wait(&barrier);
            }
            while(cpu1 || cpu2 || cpu3){ 
               // cout<<"fin CPU1"<<endl;
                cpu1 = false;
                pthread_barrier_wait(&barrier); //Barrera de control para cuando CPU1 termina sus hilos, que espere a los otros mientras teminan
            }
            
            
            break;
        }
        case 2:{
            cntxActual2 = 0; //el primer proceso que entra es el primero que sera ejecutado.
            contextos2[cntxActual2][34] = ciclo; // Guarda el inicio del primer ciclo en la tabla de contextos
            while(checkContextos(id_hilo) == true) {
                plb = buscarBloque(id_hilo); //Busca la palabra que esta en el PC1
                procesarPalabra(plb,id_hilo); //Procesa la palabra que esta en el plb.
                pthread_barrier_wait(&barrier);
            }
            while(cpu1 || cpu2 || cpu3){ 
               // cout<<"fin CPU1"<<endl;
                cpu2 = false;
                pthread_barrier_wait(&barrier); //Barrera de control para cuando CPU1 termina sus hilos, que espere a los otros mientras teminan
            }
            
            
            break;
        }
        case 3:{
            cntxActual3 = 0; //el primer proceso que entra es el primero que sera ejecutado.
            contextos3[cntxActual3][34] = ciclo; // Guarda el inicio del primer ciclo en la tabla de contextos
            while(checkContextos(id_hilo) == true) {
                plb = buscarBloque(id_hilo); //Busca la palabra que esta en el PC1
                procesarPalabra(plb,id_hilo); //Procesa la palabra que esta en el plb.
                pthread_barrier_wait(&barrier);
            }
            while(cpu1 || cpu2 || cpu3){ 
               // cout<<"fin CPU1"<<endl;
                cpu3 = false;
                pthread_barrier_wait(&barrier); //Barrera de control para cuando CPU1 termina sus hilos, que espere a los otros mientras teminan
            }
            
            break;
        }
            
    }
}

int main (int argc, char** argv) {
    pthread_t hilo1, hilo2, hilo3; // Se crean los 3 hilos que manejan los CPU
    int ret;
    cout << "Por favor digite el número de hilos tendrá el programa:" << endl;
    cin >> subHilos; // Se guarda el número de hilos que el programa va a ejecutar
    cargarHilos(subHilos);
    
    llenaMemDatos();
    cout << "Por favor digite el quantum que tendrá el programa:" << endl;
    cin >> quantum; // Se guarda le quantum global que se usará en el programa
    //imprimirMemP();
    pthread_mutex_init(&mCacheDatos1, NULL);
    pthread_mutex_init(&mCacheDatos2, NULL);
    pthread_mutex_init(&mCacheDatos3, NULL);
    pthread_mutex_init(&mDirectorio1, NULL);
    pthread_mutex_init(&mDirectorio2, NULL);
    pthread_mutex_init(&mDirectorio3, NULL);
    
    quantum1 = quantum;
    quantum2 = quantum;
    quantum3 = quantum; // Se sincronizan los 3 quantum con el quantum que se ingresa

    ciclo = 1; // Iniciamos la variable del conteo de ciclos en 1
    estado1 = estado2 = estado3 = 1; // Los estados de los suhilos los inicilizamos en 1 --> 1 = hilo en ejecución y 0 lo contrario.
    
    //cout << "Inicia la ejecucion" << endl;

    pthread_barrier_init(&barrier, NULL, 4); //inicializa la barrera para que espere 
    ret =  pthread_create(&hilo1, NULL, &CPU, (void*)1); // Se crea el hilo y se manda a ejecutar su programa principal
    ret =  pthread_create(&hilo2, NULL, &CPU, (void*)2); // Se crea el hilo y se manda a ejecutar su programa principal
    ret =  pthread_create(&hilo3, NULL, &CPU, (void*)3); // Se crea el hilo y se manda a ejecutar su programa principal

    // Sincroniza el principal con los hilos, los hilos hacen wait en el metodo CPU.
    pthread_barrier_wait(&barrier);
    
    // Se chequea en el main si no queden hilos sin terminar en cada CPU
    // Si un CPU termina, espera a que los demás terminen su trabajo
    while(cpu1 || cpu2 || cpu3){ 
        ciclo++; // Se aumenta el contador de ciclos
        pthread_barrier_wait(&barrier);
    }
    
    
    

    pthread_join(hilo1, 0);
    pthread_join(hilo2, 0);
    pthread_join(hilo3, 0);

    imprimirInfoHilo();

    cout << endl;
    cout << "Todos terminaron" << endl;
    
    pthread_barrier_destroy(&barrier);
    pthread_exit(NULL);
    return 0;
}
