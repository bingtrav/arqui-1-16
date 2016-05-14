/* Proyecto Arquitectura de Computadoras. */
// http://tuxthink.blogspot.com/2013/01/ussing-barriers-in-pthreads.html
// http://www.bogotobogo.com/cplusplus/multithreading_pthread.php
// Compilar con pthreads: g++ -pthread  proyecto2.cpp -o proyecto2
// http://jrivc1.blogspot.com/p/5.html

// Se incluye la biclioteca de hilos
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <string.h>

using namespace std;

// Constante con el número de hilos = número de CPU's
#define numHilos 3

// Número de hilos que se manejarán en lso 3 CPU's
int subHilos;

//PC del CPU
int pc1;
int pc2;
int pc3;

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

//Memoria principal
// [0]   --- [127] ---> memoria compartida
// [128] --- [383] ---> memoria compartida
int memPrin1[384];
int memPrin2[384];
int memPrin3[384];


//Registros de CPU
int reg1[32];
int reg2[32];
int reg3[32];

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

/* 
    Este metodo se encarga de realizar el cambio de contexto segun el CPU que lo solicite. 
    En caso de que el contecto actual este inactivo, o sea, contextos[id_hilo][33] == 0, 
    solo hace cambios de ciclo sin cambiar registros.
*/
void cambioContexto(int id_hilo) {
    switch(id_hilo) {
        case 1:
            if(contextos1[cntxActual1][33] == 1) {
                contextos1[cntxActual1][0] = pc1;
                for(int i=1; i<33; i++) {
                    contextos1[cntxActual1][i] = reg1[i-1];
                }
                // Indica en la memoria de contextos que el contexto ya ha finalizado.
                if(estado1==0) {
                    contextos1[cntxActual1][33] == estado1;
                }
                quantum1 = quantum;   
            } else {                                        // Cuando terminó el contexto pero faltan otros.
                /* En caso de que el contexto actual finalizó y hay otros por finalizar. */
            }
            break;
        case 2:
            if(contextos1[cntxActual2][33] == 1) {
             
                quantum2 = quantum;   
            }
            break;
        case 3:
            if(contextos1[cntxActual3][33] == 1) {
             
                quantum3 = quantum;   
            }
            break;
    }
}

/* Cuenta cuantas instrucciones ha hecho para cambio de contexto*/
void cuentaIns(int id_hilo) {
    switch(id_hilo) {
        case 1:
            if(estado1==0 || quantum1==0) {
                cambioContexto(id_hilo);
            } else {
                quantum1--;
            }
            break;
        case 2:
            if(estado2==0 || quantum2==0) {
                cambioContexto(id_hilo);
            } else {
                quantum2--;
            }
            break;
        case 3:
            if(estado3==0 || quantum3==0) {
                cambioContexto(id_hilo);
            } else {
                quantum3--;
            }
            break;
    }
}

void falloCache (int direccion, int seccion, int id_hilo) {
    switch (id_hilo)
      {
         case 1:{
            for(int a = 0; a < 16; a++) {
                cache1[seccion][a] = memPrin1[direccion];
                direccion++;
                pthread_barrier_wait(&barrier);
            }
            quantum1++;
            break;
         }
         case 2:{
            for(int a = 0; a < 16; a++) {
                cache2[seccion][a] = memPrin2[direccion];
                direccion++;
                pthread_barrier_wait(&barrier);
            }
            quantum2++;
            break;
         }
         case 3:{
            for(int a = 0; a < 16; a++) {
                cache3[seccion][a] = memPrin3[direccion];
                direccion++;
                pthread_barrier_wait(&barrier);
            }
            
            cuentaIns(id_hilo);
            break;
         }
    }
}

int* buscarBloque(int pc, int id_hilo) {
    int bloque = pc/16;
    int palabra = pc%16;
    int seccion = bloque%4;
    int plb[4];
    int* plbPtr = plb;

    switch (id_hilo)
      {
         case 1:{
            if(cache1[seccion][16] != bloque) {
                int dirFisica = bloque * 16;
                falloCache(dirFisica, seccion, id_hilo);
            }
        
            for(int a = 0; a < 4; a++) {
                plb[a] = cache1[seccion][palabra];
                palabra++;
            }
            break;
         }
         case 2:{
            if(cache2[seccion][16] != bloque) {
                int dirFisica = bloque * 16;
                falloCache(dirFisica, seccion, id_hilo);
            }
        
            for(int a = 0; a < 4; a++) {
                plb[a] = cache2[seccion][palabra];
                palabra++;
            }
            break;
         }
         case 3:{
            if(cache3[seccion][16] != bloque) {
                int dirFisica = bloque * 16;
                falloCache(dirFisica, seccion, id_hilo);
            }
        
            for(int a = 0; a < 4; a++) {
                plb[a] = cache3[seccion][palabra];
                palabra++;
            }
            break;
         }
    }

    return plbPtr;
}

void procesarPalabra(int* palabra, int id_hilo) {
    //aumento de PC
    switch (id_hilo) {
                case 1:{
                    pc1+=4;
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
        
    switch ( palabra[0] )
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
            break;
            }
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
                    if(reg1[palabra[1]]==0){
                        pc2+=salto;
                    }
                    break;
                }
                case 3:{
                    if(reg1[palabra[1]]==0){
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
                    if(reg1[palabra[1]]!=0){
                        pc2+=salto;
                    }
                    break;
                }
                case 3:{
                    if(reg1[palabra[1]]!=0){
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
      pthread_barrier_wait(&barrier);
}

bool checkContextos(int id_hilo){
    bool check = false; //retorna true si aun hay contextos
    
    switch (id_hilo) {
        case 1:{
            for(int i = 0; i < 4; i++) {
                if(contextos1[i][33] == 0) {
                    check = true;
                }
            }
            break;
        }
        case 2:{
            for(int i = 0; i < 4; i++) {
                if(contextos2[i][33] == 0) {
                    check = true;
                }
            }
            break;
        }
        case 3:{
            for(int i = 0; i < 4; i++) {
                if(contextos1[i][33] == 0) {
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
    int contMenInt1 = 128; // Contador para la inserción de instruccinoes en la memoria del CPU 1
    int contMenInt2 = 128; // Contador para la inserción de instruccinoes en la memoria del CPU 2
    int contMenInt3 = 128; // Contador para la inserción de instruccinoes en la memoria del CPU 3
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
        while (((dir = readdir(d)) != NULL) && (contHilos < hilos))
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
                            cout << "CPU 1: ";
                            cout << endl;
                            contextos1[contHilo1][0] = contMenInt1;
                            contextos1[contHilo1][36] = contHilos;
                            contHilo1++;
                            while (file >> str){
                                memPrin1[contMenInt1] = atoi(str.c_str());
                                contMenInt1++;
                            }
                            procesador = 2;
                            break;
                         }
                        case 2:{
                            cout << "CPU 2: ";
                            cout << endl;
                            contextos2[contHilo2][0] = contMenInt2;
                            contextos2[contHilo2][36] = contHilos;
                            contHilo2++;
                            while (file >> str){
                                memPrin2[contMenInt2] = atoi(str.c_str());
                                contMenInt2++;
                            }
                            procesador = 3;
                            break;
                        }
                        case 3:{
                            cout << "CPU 3: ";
                            cout << endl;    
                            contextos3[contHilo3][0] = contMenInt3;
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

void *CPU(void *param)
{
    int id_hilo = *((int*)(&param)); //Id del procesador
    int* plb[4]; // Arreglo donde se guarda el bloque buscado
    printf("Estoy aqui1\n");
    //cargar palabra en CPU para procesar.
    
    switch (id_hilo) {
        case 1:{
            printf("Estoy aquiCPU1\n");
            
            while(checkContextos(id_hilo) == true) {
                *plb = buscarBloque(pc1, id_hilo);
                printf("bloqueListo CPU1\n");
                procesarPalabra(*plb,id_hilo);
                printf("Listo CPU1\n");
            }
            break;
        }
        case 2:{
            printf("Estoy aquiCPU2\n");
            while(checkContextos(id_hilo) == true) {
                *plb = buscarBloque(pc2, id_hilo);
                printf("bloqueListo CPU2\n");
                procesarPalabra(*plb,id_hilo);
                printf("Listo CPU2\n");
            }
            break;
        }
        case 3:{
            printf("Estoy aquiCPU3\n");
            while(checkContextos(id_hilo) == true) {
                *plb = buscarBloque(pc3, id_hilo);
                printf("bloqueListo CPU3\n");
                procesarPalabra(*plb,id_hilo);
                printf("Listo CPU3\n");
            }
            break;
        }
            
    }
    
    printf("Estoy aqui\n");
}

int main (int argc, char** argv) {
    /*pthread_t hilo1, hilo2, hilo3;
    int ret;
    cout << "Por favor digite el número de contextos tendrá el programa:" << endl;
    cin >> subHilos;
    cargarHilos(subHilos);
    cout << "Por favor digite el quantum que tendrá el programa:" << endl;
    cin >> quantum;
    quantum--;                  // Resta uno porque ya se contempla que se debe de hacer esa instruccion.
    */
    
    cargarHilos(3);
    imprimirMemP();
    imprimircontextos();
    pthread_t hilo1, hilo2, hilo3;
    int ret;
    
    pthread_barrier_init(&barrier, NULL, 3);
    ret =  pthread_create(&hilo1, NULL, &CPU, (void*)1);
    ret =  pthread_create(&hilo2, NULL, &CPU, (void*)2);
    ret =  pthread_create(&hilo3, NULL, &CPU, (void*)3);
    
    
    pthread_join(hilo1, 0);
    pthread_join(hilo2, 0);
    pthread_join(hilo3, 0);
    pthread_barrier_destroy(&barrier);
    pthread_exit(NULL);
    imprimirMemP();
    imprimircontextos();
    
    
    return 0;
}
