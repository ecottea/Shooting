// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// はかいこうせん風のビーム弾幕（弾種・色を限定）
static void ShotBeam(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ビームフェーズ（段階的に本数を増やす）
    int phase = pEnemyShotSet->count / 60; // 0,1,2
    if (phase > 2) phase = 2;

    int beamCount;
    switch (phase) {
    case 0: beamCount = 8;  break; // 初期は8本
    case 1: beamCount = 16; break; // 次は16本
    case 2: beamCount = 24; break; // 最後は24本
    }

    // 初回またはフェーズが変わったときにビームを再生成
    if (pEnemyShotSet->count % 60 == 0 && pEnemyShotSet->count <= 600) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ビームを放射状に配置
        for (int i = 0; i < beamCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 360度を等分した方向 + フェーズごとに少し回転
            double baseAngle = (DX_PI * 2.0 * i) / beamCount;
            double phaseRot = phase * DX_PI / 24.0; // フェーズごとに少し回転
            pEnemyShot->muki = baseAngle + phaseRot;

            // ビームの速度もフェーズごとに少し変える
            pEnemyShot->speed = 2.5 + phase * 0.5;

            // 弾種と色を「はかいこうせん」らしいものに限定
            int type = GetRand(1);        // 0:銃弾, 1:菱形弾
            int colorIndex = GetRand(2);  // 0:赤, 1:青, 2:白
            int color;
            switch (colorIndex) {
            case 0: color = 0; break; // 赤
            case 1: color = 4; break; // 青
            case 2: color = 6; break; // 白
            }

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotBullet[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 時間経過でビームを少しずつ回転させる
        pEnemyShot->muki += DX_PI / 360.0; // 毎フレーム 0.5度ずつ回転

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hakaikousen_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔でビーム弾幕セットを生成
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBeam;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // ビームは弾ごとに方向を持つのでここでは未使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}