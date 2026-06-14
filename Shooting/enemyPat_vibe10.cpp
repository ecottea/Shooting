// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：竜巻状の螺旋弾幕
static void ShotDragonSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 12個の弾を螺旋状に配置
        const int N = 220;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = 2 * DX_PI * i / N + pEnemyShotSet->count / 30.0; // 螺旋の角度
            pEnemyShot->x = pEnemyShotSet->x + 30 * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + 30 * sin(angle);
            pEnemyShot->muki = angle + DX_PI / 2; // 進行方向（接線方向）
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0; // 速度にばらつき

            // 弾の種類と色をランダムに設定（竜をイメージして赤や金を多めに）
            int type = GetRand(2) == 0 ? 4 : GetRand(5); // 鱗弾を多めに
            int color = GetRand(10) < 7 ? 0 : 6; // 赤、白、その他
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
            case 3:
                pEnemyShot->kind = img_enemyShotBullet[color];
                break;
            case 4:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            case 5:
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

    // 弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Dragon_Vibe()
{
    static int moveDirection = 1;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        moveDirection = 1;
    }
    else {
        // 敵を左右に移動
        enemy.x += 1.5 * moveDirection;
        if (enemy.x < 80 || enemy.x > 400) moveDirection *= -1;
    }

    // 60フレームごとに竜巻弾幕を発射
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragonSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}