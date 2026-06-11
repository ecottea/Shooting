#include "DxLib.h"

int getHitKeyStateAll_2(int getHitKeyStateAll_InputKey[]){
    char getHitKeyStateAll_Key[256];
	
    GetHitKeyStateAll( getHitKeyStateAll_Key );
    for(int i=0; i<256; i++){
        if(getHitKeyStateAll_Key[i]==1) getHitKeyStateAll_InputKey[i]++;
        else                            getHitKeyStateAll_InputKey[i]=0;
    }
    return 0;
}

