/* Proyecto Arquitectura de Computadoras. */
// Compilar con pthreads: g++ -pthread  proyecto.cpp -o proyecto

// Se incluye la biclioteca de hilos
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <dirent.h>                                                            
#include <string.h>
#include <vector>
#include <list>

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
bool cpu2= true;
bool cpu3= true;

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
int cacheDatos1[4][6];
int cacheDatos2[4][6];
int cacheDatos3[4][6];

// booleano que simula semaforo para el ingreso a cache true = cache libre, false = cache ocupado
bool cacheDisponible[3];

// Directorio que maneja informacion y estado de los Bloques que tiene cada CPU
int directorio1[8][4];
int directorio2[8][4];
int directorio3[8][4];

// booleano que simula semaforo para el ingreso a Directorio true = directoro libre, false = directorio ocupado
bool DirDisponible[3];

// cola para ingreso Cache de datos que tiene cada CPU
list<int> colaCache1;
list<int> colaCache2;
list<int> colaCache3;

// cola para ingreso Cache de datos que tiene cada CPU
list<int> colaDir1;
list<int> colaDir2;
list<int> colaDir3;


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
void falloCache (int id_bloque, int direccion, int seccion, int id_hilo) {
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

vector<int> buscarBloque(int id_hilo) {
    int bloque; //Bloque que se va a buscar PC/16
    int palabra; //número de palabra del bloque PC%16
    int seccion; //seccion en cache a la que pertenece el bloque Bolque%4 
    vector<int> plb (4); // vector que contiene la palabra
                
    switch (id_hilo)
      {
         case 1:{ 
            bloque = pc1/16; //bloque CPU1 
            palabra = pc1%16; //palabra CPU1
            seccion = bloque%4; //seccion CPU1
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
            bloque = pc2/16; //bloque CPU2
            palabra = pc2%16; //palabra CPU2
            seccion = bloque%4; //seccion CPU2
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
            bloque = pc3/16; //bloque CPU3
            palabra = pc3%16; //palabra CPU3
            seccion = bloque%4; //seccion CPU3
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
         case 50:{
            // LL
            switch (id_hilo) {
                case 1:{
                    /*int bloque, palabra;
                    dir = reg1[palabra[1]] + palabra[3];
                    bloque = dir/16; 
                    palabra = dir%16;*/
                    
                    
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
        while (((dir = readdir(d)) != NULL) && (contHilos < hilos)) // Recorre la cantidad de hilos que se le asignó al iniciar el programa
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
            cout << "CPU1" << checkContextos(1)  << checkContextos(2)  <<checkContextos(3) <<endl;
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
            cout << "CPU2" << checkContextos(1)  << checkContextos(2)  <<checkContextos(3) <<endl;
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
            cout << "CPU3" <<checkContextos(1)  << checkContextos(2)  <<checkContextos(3) <<endl;
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
    cout << "Por favor digite el quantum que tendrá el programa:" << endl;
    cin >> quantum; // Se guarda le quantum global que se usará en el programa
    
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

    //cout << endl;
    //cout << "Todos terminaron" << endl;
    
    pthread_barrier_destroy(&barrier);
    pthread_exit(NULL);
    return 0;
}
