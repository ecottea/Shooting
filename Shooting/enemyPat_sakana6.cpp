#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 弾幕：波と粒の境界（修正版）
static void ShotWaveParticleBoundary(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // カウンタ更新
    //pEnemyShotSet->count++;

    // フェーズ管理（見た目の切り替え用）
    int phase = (pEnemyShotSet->count / 60) % 2; // 60フレームごとにフェーズ切り替え
    // phase == 0: 波フェーズ
    // phase == 1: 粒フェーズ

    // 弾生成はフェーズに関係なく一定間隔で行う
    if (pEnemyShotSet->count % 4 == 0) {
        pEnemyShot = new sEnemyShot;

        // 敵の周囲を円状に配置（波フェーズ風）
        double angle = (pEnemyShotSet->count * 0.1);
        double radius = 60.0 + sin(pEnemyShotSet->count * 0.05) * 20.0;

        pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
        pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);

        if (phase == 0) {
            // 波フェーズ：円運動
            pEnemyShot->muki = angle + DX_PI / 2.0; // 接線方向
            pEnemyShot->speed = 1.5;
        }
        else {
            // 粒フェーズ：プレイヤー方向へ
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 2.5;
        }

        // 弾の種類と色（素材の一覧から）
        int type = GetRand(2); // 0〜2（小玉・中玉・大玉）
        int color = GetRand(7); // 0〜7（赤〜白）
        switch (type) {
        case 0:
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            break;
        case 1:
            pEnemyShot->kind = img_enemyShotMediumBall[color];
            break;
        case 2:
            pEnemyShot->kind = img_enemyShotLargeBall[color];
            break;
        }

        // リストに挿入
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 音（素材の一覧から）
        if (pEnemyShotSet->count % 20 == 0) {
            int sound_type = GetRand(2); // 0,1 のいずれか
            switch (sound_type) {
            case 0:
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                break;
            case 1:
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                break;
            }
        }
    }

    // 既存弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 粒フェーズでは少し速度を上げる（任意）
        if (phase == 1) {
            pEnemyShot->speed += 0.01;
            if (pEnemyShot->speed > 4.0) pEnemyShot->speed = 4.0;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン：波と粒の境界（修正版）
void EnemyPat_Namistubu_Sakana()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 150;
        muki = 1;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 弾幕セットを生成（サンプルと同じ間隔）
    if (count % 300 == 30 && count <= 930) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWaveParticleBoundary;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0; // 使わないが一応初期化

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}