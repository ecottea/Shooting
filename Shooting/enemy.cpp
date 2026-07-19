#include "DxLib.h"
#include "gv.h"
#include "enemy.h"
#include "imgSoundLoad.h"
#include "stateManager.h"
#include "player.h"

void enemyDisp() {
    // 表示に使うハンドルを先に決める
    int handle = (StateManager::GetState() != Joutai::Win)
        ? imageData[img_enemy[0]].handle
        : imageData[img_enemy[1]].handle;

    // 画像サイズを取得
    int w, h;
    GetGraphSize(handle, &w, &h);

    // 自機と敵機の距離を計算
    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double distance = sqrt(dx * dx + dy * dy);

    // 透明度のパラメータ（お好みで調整）
    const double FAR_DIST = 80.0;     // この距離以遠は完全不透明
    const double NEAR_DIST = 40.0;    // この距離以内は最も透明
    const int    MIN_ALPHA = 64;      // 最も近いときのアルファ値（0～255）
    const int    MAX_ALPHA = 255;     // 遠いときのアルファ値（完全不透明）

    int alpha;
    if (distance >= FAR_DIST) {
        alpha = MAX_ALPHA;
    }
    else if (distance <= NEAR_DIST) {
        alpha = MIN_ALPHA;
    }
    else {
        // 距離に比例してアルファ値を線形補間（近いほど小さい値＝透明）
        double t = (distance - NEAR_DIST) / (FAR_DIST - NEAR_DIST); // 0.0～1.0
        alpha = (int)(MIN_ALPHA + t * (MAX_ALPHA - MIN_ALPHA));
    }

    // アルファが255未満のときだけ半透明描画を設定
    if (alpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    }

    // 中心座標になるように描画
    DrawGraph((int)(enemy.x - w / 2), (int)(enemy.y - h / 2), handle, TRUE);

    // 描画モードを元に戻す
    if (alpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void enemyHit() {
    if (isMuteki) return;

    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double r = imageData[img_player].radiusX + imageData[img_enemy[0]].radiusX;
    if (dx * dx + dy * dy < r * r) {
        if (CheckSoundMem(sound_playerDestroyed)) StopSoundMem(sound_playerDestroyed);
        PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);
        StateManager::ChangeState(Joutai::Lose);
    }
}