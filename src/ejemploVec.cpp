#include <iostream>
using namespace std;

void getVector(int* v) {
    int vec[5];
    for(int i=0; i<5; i++) {
        vec[i] = (i*12/2)+1;
    }
    
    *v = *vec;
}

int main() {
    int *miVec;
    
    getVector(miVec);
    
    for(int a=0; a<5; a++) {
        cout << miVec[a]+"";
    }
}