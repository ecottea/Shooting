#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：飛鉢「伝説の飛空円盤」
static void ShotFlyingSaucer(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // 1. 狙い撃ち弾の発射 (15フレームごと)
    if (c % 7 == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;

        // プレイヤーへの角度を計算
        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
        pShot->speed = 3.5;

        // 弾の種類と色: シアン(3)の中玉 (UFOの光をイメージ)
        pShot->kind = img_enemyShotMediumBall[3];

        // 双方向リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 2. リング弾の発射 (60フレームごと)
    if (c % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int ringCount = 36;
        for (int i = 0; i < ringCount; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // 時間経過で回転する美しいリングにする (c * 0.05 で回転速度を調整)
            double angle = (DX_PI * 2.0 / ringCount) * i + (c * 0.05);
            pShot->muki = angle;
            pShot->speed = 2.2;

            // 弾の種類と色: 青(4)の小玉
            pShot->kind = img_enemyShotSmallBall[4];

            // 双方向リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 3. 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_HikouEnban_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        // スペルカード予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 円盤の移動（リサージュ曲線による浮遊感のある動き）
    if (count > 1) {
        double time = count * 0.015;
        // X軸とY軸で異なる周期と振幅を持たせることで、予測しにくい滑らかな軌道を描く
        enemy.x = 240.0 + sin(time) * 180.0;
        enemy.y = 60.0 + sin(time * 1.3 + 1.0) * 60.0;
    }

    // 弾幕セットの生成 (予告音の後、60フレーム待って開始)
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlyingSaucer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 既存の飛空円盤パターンの発生源位置を、敵本体の現在位置に追従させる
    // これにより、移動中の円盤から正しく弾が発射される
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->patternFunc == ShotFlyingSaucer) {
            pSet->x = enemy.x;
            pSet->y = enemy.y;
        }
        pSet = pSet->next;
    }
}