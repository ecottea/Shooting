#include "DxLib.h"
#include "gv.h"
#include "enemy.h"

void enemyDisp() {
    // 表示に使うハンドルを先に決める
    int handle = (joutaiFlag != Joutai::Win)
        ? imageData[img_enemy[0]].handle
        : imageData[img_enemy[1]].handle;

    // 画像サイズを取得
    int w, h;
    GetGraphSize(handle, &w, &h);

    // 中心座標になるように描画
    DrawGraph((int)(enemy.x - w / 2), (int)(enemy.y - h / 2), handle, TRUE);
}

void enemyHit() {
    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    if (dx*dx + dy*dy < 1600.0) {
        PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);
        joutaiFlag = Joutai::Lose;
    }
}