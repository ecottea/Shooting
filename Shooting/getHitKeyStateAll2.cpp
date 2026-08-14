#include "DxLib.h"

int getHitKeyStateAll_2(int getHitKeyStateAll_InputKey[]){
    char getHitKeyStateAll_Key[256];
	
    GetHitKeyStateAll( getHitKeyStateAll_Key );

    int pad = GetJoypadInputState(DX_INPUT_PAD1);
	getHitKeyStateAll_Key[KEY_INPUT_NUMPAD8] |= (pad & PAD_INPUT_UP) ? 1 : 0;
	getHitKeyStateAll_Key[KEY_INPUT_NUMPAD5] |= (pad & PAD_INPUT_DOWN) ? 1 : 0;
	getHitKeyStateAll_Key[KEY_INPUT_NUMPAD4] |= (pad & PAD_INPUT_LEFT) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_NUMPAD6] |= (pad & PAD_INPUT_RIGHT) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_R] |= (pad & PAD_INPUT_1) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_V] |= (pad & PAD_INPUT_2) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_N] |= (pad & PAD_INPUT_3) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_Q] |= (pad & PAD_INPUT_4) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_NUMPAD7] |= (pad & PAD_INPUT_5) ? 1 : 0;
    getHitKeyStateAll_Key[KEY_INPUT_NUMPAD9] |= (pad & PAD_INPUT_6) ? 1 : 0;


    for(int i=0; i<256; i++){
        if(getHitKeyStateAll_Key[i]==1) getHitKeyStateAll_InputKey[i]++;
        else                            getHitKeyStateAll_InputKey[i]=0;
    }
    return 0;
}

