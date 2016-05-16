/* Proyecto Arquitectura de Computadoras. */
// http://tuxthink.blogspot.com/2013/01/ussing-barriers-in-pthreads.html
// http://www.bogotobogo.com/cplusplus/multithreading_pthread.php
// Compilar con pthreads: g++ -pthread  proyecto2.cpp -o proyecto2

// Se incluye la biclioteca de hilos
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <string.h>
#include <vector>

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
void imprimirReg() {
    cout << endl;
    
    cout << "Ciclo: ";
    cout << ciclo;
    cout << endl;
    
    
    cout << endl;
    cout << pc1;
    
    cout << "Reg1: ";
    cout << endl;
    for(int i= 0; i < 32; i++){
        cout << reg1[i] << " - ";
    }
    
    cout << endl;
    cout << "Reg2: ";
    cout << endl;
    for(int i= 0; i < 32; i++){
        cout << reg2[i] << " - ";
    }
    cout << endl;
    cout << "Reg3: ";
    cout << endl;
    for(int i= 0; i < 32; i++){
        cout << reg3[i] << " - ";
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
            //cout<<"cuentaIns \n";
            if(estado1==0 || quantum1==0) {
               // printf("CambioContexto CPU1\n");
                cambioContexto(id_hilo);
            } else {
                //cout<<"quantum cpu1: ";
                //cout<< quantum1 << endl;
                quantum1--;
            }
            break;
        case 2:
            if(estado2==0 || quantum2==0) {
               // printf("CambioContexto CPU2\n");
                cambioContexto(id_hilo);
            } else {
                quantum2--;
            }
            break;
        case 3:
            if(estado3==0 || quantum3==0) {
                //printf("CambioContexto CPU3\n");
                cambioContexto(id_hilo);
            } else {
                quantum3--;
            }
            break;
    }
}

void falloCache (int id_bloque, int direccion, int seccion, int id_hilo) {
     
    switch (id_hilo)
      {
         case 1:{
            
            for(int a = 0; a < 16; a++) {
                //printf("falloCache CPU1\n");
                cache1[seccion][a] = memPrin1[direccion];
                direccion++;
                pthread_barrier_wait(&barrier);
            }
            cache1[seccion][16] = id_bloque; 
           // printf("listo falloCache CPU1\n");
            cuentaIns(id_hilo);
            break;
         }
         case 2:{
             
            for(int a = 0; a < 16; a++) {
             //   printf("falloCache CPU2\n");
                cache2[seccion][a] = memPrin2[direccion];
                direccion++;
                pthread_barrier_wait(&barrier);
            }
           // printf("listo falloCache CPU2\n");
            cache2[seccion][16] = id_bloque;
            cuentaIns(id_hilo);
            break;
         }
         case 3:{
            for(int a = 0; a < 16; a++) {
               // printf("falloCache CPU3\n");
                cache3[seccion][a] = memPrin3[direccion];
                direccion++;
                pthread_barrier_wait(&barrier);
            }
            //printf("listo falloCache CPU3\n");
            cache3[seccion][16] = id_bloque;
            cuentaIns(id_hilo);
            break;
         }
    }
}

vector<int> buscarBloque(int id_hilo) {
    int bloque;
    int palabra;
    int seccion;
    vector<int> plb (4);
                
    switch (id_hilo)
      {
         case 1:{ 
            bloque = pc1/16;
            palabra = pc1%16;
            seccion = bloque%4;
            

            while(cache1[seccion][16] != bloque) {
                int dirFisica = bloque * 16;
                falloCache(bloque, dirFisica, seccion, id_hilo);
                if(quantum1 == quantum) {
                    bloque = pc1/16;
                    palabra = pc1%16;
                    seccion = bloque%4;
                }
            }
            
            for(int a = 0; a < 4; a++) {
                plb[a] = cache1[seccion][palabra];
                palabra++;
            }
            break;
         }
         case 2:{
            bloque = pc2/16;
            palabra = pc2%16;
            seccion = bloque%4;
          
            while(cache2[seccion][16] != bloque) {
                int dirFisica = bloque * 16;
                falloCache(bloque, dirFisica, seccion, id_hilo);
                if(quantum2 == quantum) {
                    bloque = pc2/16;
                    palabra = pc2%16;
                    seccion = bloque%4;
                }
            }
        
            for(int a = 0; a < 4; a++) {
                plb[a] = cache2[seccion][palabra];
                palabra++;
            }
            break;
         }
         case 3:{
            bloque = pc3/16;
            palabra = pc3%16;
            seccion = bloque%4;

            while(cache3[seccion][16] != bloque) {
                int dirFisica = bloque * 16;
                falloCache(bloque, dirFisica, seccion, id_hilo);
                if(quantum3 == quantum) {
                    bloque = pc1/16;
                    palabra = pc1%16;
                    seccion = bloque%4;
                }
            }

            for(int a = 0; a < 4; a++) {
                plb[a] = cache3[seccion][palabra];
                palabra++;
            }
            break;
         }
    }
    
    //cout<<"palabra (buscarBloque): "<<plb[0]<<endl;
    return plb;
}

void procesarPalabra(vector<int> palabra, int id_hilo) {
    //aumento de PC
    switch (id_hilo) {
                case 1:{
                    
                    pc1 += 4;
                    //cout<<"pc: (procpalabra)"<<pc1<<endl;
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
    //cout<<"palabra 0 (procpalabra): ";
    //cout<<palabra[0]<<endl;
    switch (palabra[0] )
      {
         case 8:{
            //DADDI; RX, RY, #n; Rx <-- (Ry) + n
            switch (id_hilo) {
                case 1:{
                    //cout<<"daddi pc1 \n";
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
                    //cout<<"dadd pc1 \n";
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
                    //cout<<"dsub pc1\n";
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
                    //cout<<"dmul pc1\n";
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
                    cout << reg1[palabra[2]];
                    //cout<<"ddiv pc1\n";
                    
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
                    //cout<<"Beqz pc1\n";
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
                   // cout<<"Bnez pc1\n";
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
                   // cout<<"Jal pc1\n";
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
                    //cout<<"JR pc1\n";
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
                    cout<<"Fin pc1\n";
                    estado1 = 0;
                    break;
                 }
                case 2:{
                    cout<<"Fin pc2\n";
                    estado2 = 0;
                    break;
                }
                case 3:{
                    cout<<"Fin pc3\n";
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
                if(contextos1[i][33] == 1) {
                    check = true;
                }
            }
            break;
        }
        case 2:{
            for(int i = 0; i < 4; i++) {
                if(contextos2[i][33] == 1) {
                    check = true;
                }
            }
            break;
        }
        case 3:{
            for(int i = 0; i < 4; i++) {
                if(contextos1[i][33] == 1) {
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
                            contextos1[contHilo1][0] = contMenInt1;
                            contextos1[contHilo1][33] = 1;
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
                            contextos2[contHilo2][0] = contMenInt2;
                            contextos2[contHilo2][33] = 1;
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
                            contextos3[contHilo3][0] = contMenInt3;
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




void *CPU(void *param)
{
    int id_hilo = *((int*)(&param)); //Id del procesador
    vector<int> plb(4); // Arreglo donde se guarda el bloque buscado
    //printf("Estoy aqui1\n");
    //cargar palabra en CPU para procesar.
    
    switch (id_hilo) {
        case 1:{
            //printf("Estoy aquiCPU1\n");
            
            while(checkContextos(id_hilo) == true) {
               //cout<<"Buscar bloque\n";
               // cout<<pc1<<endl;
                plb = buscarBloque(id_hilo);
                
                //cout<<"palabra (cpu)"<<plb[0]<<endl;
                //printf("bloqueListo CPU1\n");
               // cout<<"Procesar palabra"<< endl;
                procesarPalabra(plb,id_hilo);
               // printf("palabra procesada CPU1\n");
            }
            break;
        }
        case 2:{
            //printf("Estoy aquiCPU2\n");
            while(checkContextos(id_hilo) == true) {
                plb = buscarBloque(id_hilo);
               // printf("bloqueListo CPU2\n");
                procesarPalabra(plb,id_hilo);
                //printf("Listo CPU2\n");
            }
            break;
        }
        case 3:{
            //printf("Estoy aquiCPU3\n");
            while(checkContextos(id_hilo) == true) {
                plb = buscarBloque(id_hilo);
               // printf("bloqueListo CPU3\n");
                procesarPalabra(plb,id_hilo);
                //printf("Listo CPU3\n");
            }
            break;
        }
            
    }
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
    ciclo = 1; // Iniciamos la variable del conteo de ciclos en 1
    estado1 = estado2 = estado3 = 1; // Los estados de los suhilos los inicilizamos en 1
    cargarHilos(3);
    imprimirReg();
    imprimirMemP();
    imprimircontextos();
    pthread_t hilo1, hilo2, hilo3;
    int ret;
    string a;
    quantum = quantum1 = quantum2 = quantum3 = 5;
    pthread_barrier_init(&barrier, NULL, 2);
    ret =  pthread_create(&hilo1, NULL, &CPU, (void*)1);
    ret =  pthread_create(&hilo2, NULL, &CPU, (void*)2);
    ret =  pthread_create(&hilo3, NULL, &CPU, (void*)3);
    
    while(checkContextos(1) == true || checkContextos(2) == true || checkContextos(3) == true){
        //cin>>a;
        //imprimirReg();
        ciclo++;
        pthread_barrier_wait(&barrier);
    }
    
    imprimirMemP();
    imprimircontextos();
    
    pthread_join(hilo1, 0);
    pthread_join(hilo2, 0);
    pthread_join(hilo3, 0);
    pthread_barrier_destroy(&barrier);
    pthread_exit(NULL);
    imprimirMemP();
    imprimircontextos();
    
    
    return 0;
}
