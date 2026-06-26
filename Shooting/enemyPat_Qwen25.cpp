// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：扇風機
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 弾の生成 (このShotSetが生成された最初のフレームのみ)
    if (pEnemyShotSet->count == 0) {
        int num_blades = 3; // 扇風機の羽の枚数
        for (int i = 0; i < num_blades; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 羽の角度に合わせて弾の向きを設定 (120度間隔)
            pEnemyShot->muki = pEnemyShotSet->muki + i * (2.0 * DX_PI / num_blades);
            pEnemyShot->speed = 2.2; // 風の初期速さ

            // 弾の種類: 鱗弾のシアン色 (涼しげな風を表現)
            pEnemyShot->kind = img_enemyShotScale[3];

            pEnemyShot->margin = 480;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 風が徐々に加速していくイメージ
        pEnemyShot->speed += 0.015;
        // 弾が少しずつ回転して渦を巻く (扇風機の気流)
        pEnemyShot->muki += 0.02;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_ElectricFan_Qwen()
{
    if (count == 1) {
        // 初期位置 (画面中央上部)
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 扇風機の首振り運動 (大きく左右に移動)
        enemy.x = 240.0 + 160.0 * sin(count * 0.025);
    }

    // 6フレームごとに弾を発射 (羽が回る速度感)
    if (count % 6 == 0) {
        // 効果音の再生 (音が重ならないように制御)
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFan;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 扇風機の回転角度と首振り角度を合成して発射方向を決定
        double spin_angle = count * 0.25;             // 回転する羽の角度
        double swing_angle = sin(count * 0.025) * 0.6; // 首振りに伴う角度の揺らぎ
        pEnemyShotSet->muki = spin_angle + swing_angle;

        pEnemyShotSet->kind = 0;

        // 弾管理用リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ShotSetリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}