// enemyPat_Tmp.cpp
// ショットガンをモチーフにした弾幕「Buckshot Cascade」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotBuckshotCascade(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射タイミング（バースト）
    if (pEnemyShotSet->count % 12 == 0 && pEnemyShotSet->count <= 60) {  // 5回バースト（0,12,24,36,48）

        // ショットガンらしい重い射撃音
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // プレイヤー方向を基準にした扇の中心角度
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 各バーストで少し角度をずらす（カスケード感）
        double offset = (pEnemyShotSet->count / 12 - 2) * 0.15;  // -0.3 ~ +0.3 くらいのずれ

        // 7発の散弾を同時発射
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;

            // 扇状に角度を広げる（-35°〜+35°）
            double angleOffset = (i - 3) * 0.20;        // 約 -0.6〜+0.6 rad（±34°程度）
            pEnemyShot->muki = baseAngle + angleOffset + offset;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 速度にばらつきを持たせて遠くで広がるように
            pEnemyShot->speed = 2.8 + (i % 3) * 0.25;   // 中心がやや速め、外側が遅め

            // 弾の見た目（ショットガンらしい太めの弾）
            pEnemyShot->kind = img_enemyShotLargeBall[GetRand(5)];  // 大玉系でずっしり感

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Shotgun_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.8 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;
    }

    // 定期的にショットセット生成（約1.2秒間隔）
    if (count % 120 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBuckshotCascade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;                    // 初期角度は使わない
        pEnemyShotSet->kind = 0;

        // 弾の双方向リスト初期化
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