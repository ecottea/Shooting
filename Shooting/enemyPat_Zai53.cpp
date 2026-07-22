// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：「Z」字斬撃弾幕
static void ShotZSlash(sEnemyShotSet* pEnemyShotSet)
{
    // Zの描画パラメータ
    const double cx = 240.0;      // Zの中心X座標
    const double cy = 240.0;      // Zの中心Y座標
    const double size = 240.0;    // Zの半分のサイズ
    const double interval = 14.0; // 弾の配置間隔

    // --- フェーズ1：Zの形に弾を配置（count == 0 の初回のみ） ---
    if (pEnemyShotSet->count == 0) {
        // 予告音を再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot;

        // 1. 上の横線 (左上 -> 右上)
        for (double x = -size; x <= size; x += interval) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + x;
            pEnemyShot->y = cy - size;
            pEnemyShot->speed = 0.0; // 自前で位置制御するため0
            // 素材選定：赤色(0)の中玉(7.0x7.0)で骨格を形成
            pEnemyShot->kind = img_enemyShotMediumBall[0];
            // 極座標変換のために相対座標と初期角度・距離を保存
            pEnemyShot->param_d[0] = x;
            pEnemyShot->param_d[1] = -size;
            pEnemyShot->param_d[2] = atan2(-size, x);
            pEnemyShot->param_d[3] = sqrt(x * x + size * size);
            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 斜め線 (右上 -> 左下)
        double len = sqrt((2.0 * size) * (2.0 * size) + (2.0 * size) * (2.0 * size));
        double dx = (-2.0 * size) / len;
        double dy = (2.0 * size) / len;
        for (double t = 0; t <= len; t += interval) {
            pEnemyShot = new sEnemyShot;
            double relX = size + dx * t;
            double relY = -size + dy * t;
            pEnemyShot->x = cx + relX;
            pEnemyShot->y = cy + relY;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0];
            pEnemyShot->param_d[0] = relX;
            pEnemyShot->param_d[1] = relY;
            pEnemyShot->param_d[2] = atan2(relY, relX);
            pEnemyShot->param_d[3] = sqrt(relX * relX + relY * relY);
            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 3. 下の横線 (左下 -> 右下)
        for (double x = -size; x <= size; x += interval) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + x;
            pEnemyShot->y = cy + size;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0];
            pEnemyShot->param_d[0] = x;
            pEnemyShot->param_d[1] = size;
            pEnemyShot->param_d[2] = atan2(size, x);
            pEnemyShot->param_d[3] = sqrt(x * x + size * size);
            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // セットのパラメータ初期化
        pEnemyShotSet->param_d[0] = cx; // 中心Xを記憶
        pEnemyShotSet->param_d[1] = cy; // 中心Yを記憶
        pEnemyShotSet->param_i[0] = 0;  // 角からの追撃弾発射フラグ
    }

    // --- フェーズ2：角から扇状に追撃弾を発射 ---
    // Zが完成してプレイヤーが文字の線上に避けているのを崩すため
    if (pEnemyShotSet->count == 60 && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1; // 1回だけ発動させる
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // Zの3つの角の座標（相対座標）
        double corners[3][2] = {
            { -size, -size }, // 左上
            {  size, -size }, // 右上
            { -size,  size }  // 左下
        };

        for (int c = 0; c < 3; c++) {
            double cornerX = cx + corners[c][0];
            double cornerY = cy + corners[c][1];
            double baseAngle = atan2(player.y - cornerY, player.x - cornerX);

            // 1つの角から5方向に扇状に発射
            for (int i = -2; i <= 2; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cornerX;
                pEnemyShot->y = cornerY;
                pEnemyShot->muki = baseAngle + i * (DX_PI / 18.0); // 10度刻み
                pEnemyShot->speed = 4.5;
                // 素材選定：青色(4)の小玉(2.5x2.5)で骨格と見分けをつける
                pEnemyShot->kind = img_enemyShotSmallBall[4];
                pEnemyShot->margin = 200;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --- フェーズ3：全弾の移動・回転処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->speed > 0.0) {
            // 追撃弾（青小玉）の通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pEnemyShotSet->count >= 120) {
            // Z字弾（赤中玉）の回転＆収縮開始
            // 反時計回りに回転 (角度を減算)
            pShot->param_d[2] += -0.025;
            // 中心に向かって収縮
            pShot->param_d[3] -= 0.4;
            if (pShot->param_d[3] < 8.0) pShot->param_d[3] = 8.0; // 最小距離で留める

            // 極座標から直交座標に再変換して位置を更新
            pShot->x = pEnemyShotSet->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]);
            pShot->y = pEnemyShotSet->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]);

            if (pEnemyShotSet->count == 400) {
                pShot->muki = GetRand(360) / 360.0 * 2 * DX_PI;
                pShot->speed = 2.0 + GetRand(200) / 100.0;
            }
        }
        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_Daimonji_Zai()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        shot_count = 0;
    }
    else {
        // 少し揺らしながら待機
        enemy.x = 240.0 + sin(count * 0.02) * 40.0;
    }

    // 600フレーム（10秒）周期でZ字弾幕を発動
    if (count % 400 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotZSlash;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}