#include "DxLib.h"
#include "gv.h"
#include "player.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double Sqrt2 = 1.41421356237309504880;

static bool isSlowMode = false;
bool isMuteki = false;


void playerControl() {
    double playerSpeed;
    if (key[KEY_INPUT_C] != 0) { playerSpeed = 1.5; isSlowMode = true; }
    else                      { playerSpeed = 4.5; isSlowMode = false; }

    if (key[KEY_INPUT_M] == 1) {
        isMuteki = !isMuteki;
    }

    if (key[KEY_INPUT_NUMPAD6] != 0) {
        if      (key[KEY_INPUT_NUMPAD5] != 0) { player.x += playerSpeed / Sqrt2; player.y += playerSpeed / Sqrt2; }
        else if (key[KEY_INPUT_NUMPAD8] != 0) { player.x += playerSpeed / Sqrt2; player.y -= playerSpeed / Sqrt2; }
        else                                   { player.x += playerSpeed; }
    }
    else if (key[KEY_INPUT_NUMPAD4] != 0) {
        if      (key[KEY_INPUT_NUMPAD5] != 0) { player.x -= playerSpeed / Sqrt2; player.y += playerSpeed / Sqrt2; }
        else if (key[KEY_INPUT_NUMPAD8] != 0) { player.x -= playerSpeed / Sqrt2; player.y -= playerSpeed / Sqrt2; }
        else                                   { player.x -= playerSpeed; }
    }
    else if (key[KEY_INPUT_NUMPAD5] != 0) { player.y += playerSpeed; }
    else if (key[KEY_INPUT_NUMPAD8] != 0) { player.y -= playerSpeed; }

    if      (player.x < 12.5)  player.x = 12.5;
    else if (player.x > 467.5) player.x = 467.5;
    if      (player.y < 17.5)  player.y = 17.5;
    else if (player.y > 462.5) player.y = 462.5;
}

void playerDisp() {
    DrawGraph((int)(player.x - 18.0), (int)(player.y - 26.0), imageData[img_player].handle, TRUE);
    if (isSlowMode) {
        DrawCircle((int)player.x, (int)player.y, 4, GetColor(255,0,0), TRUE);
        DrawCircle((int)player.x, (int)player.y, 2, GetColor(255,255,255), TRUE);
    }
}