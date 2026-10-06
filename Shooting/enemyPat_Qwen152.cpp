#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ミュラーリヤー錯視（矢羽根錯視）
static void ShotMullerLyer(sEnemyShotSet* pEnemyShotSet)
{
    // 線分のパラメータ
    const double lineY1 = 120.0-100; // 上側の線分
    const double lineY2 = 240.0-100; // 下側の線分
    const int numSegments = 6;
    double startX = pEnemyShotSet->param_d[0];
    double stepX = pEnemyShotSet->param_d[1];

    if (pEnemyShotSet->count == 0) {
        // 効果音再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);        

        // 1. 上側の線分（静止）
        for (int i = 0; i < numSegments; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX + i * stepX;
            pShot->y = lineY1;
            pShot->muki = 0.0;
            pShot->speed = 0.0; // 静止させて線分として機能させる
            pShot->kind = img_enemyShotMediumBall[6]; // 中玉, 白
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 2. 下側の線分（静止）
        for (int i = 0; i < numSegments; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX + i * stepX;
            pShot->y = lineY2;
            pShot->muki = 0.0;
            pShot->speed = 0.0; // 静止させて線分として機能させる
            pShot->kind = img_enemyShotMediumBall[6]; // 中玉, 白
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    if (pEnemyShotSet->count % 20 == 0) {
        // 効果音再生
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 3. 上側の矢羽根（外向き：＞ ― ＜）
        // 錯視で「長く」見えるが、実際は外側へ開くため隙間は広く【安全】
        // 左端
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX; pShot->y = lineY1 + pEnemyShotSet->count * 2.0;
            pShot->muki = DX_PI * 0.8; // 左斜め上
            pShot->speed = 0.6*2;
            pShot->kind = img_enemyShotDiamond[8]; // 菱形弾, 橙
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX; pShot->y = lineY1 + pEnemyShotSet->count * 2.0;
            pShot->muki = -DX_PI * 0.8; // 左斜め下
            pShot->speed = 0.6*2;
            pShot->kind = img_enemyShotDiamond[8];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        // 右端
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX + (numSegments - 1) * stepX; pShot->y = lineY1 + pEnemyShotSet->count * 2.0;
            pShot->muki = DX_PI * 0.2; // 右斜め上
            pShot->speed = 0.6*2;
            pShot->kind = img_enemyShotDiamond[8];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX + (numSegments - 1) * stepX; pShot->y = lineY1 + pEnemyShotSet->count * 2.0;
            pShot->muki = -DX_PI * 0.2; // 右斜め下
            pShot->speed = 0.6*2;
            pShot->kind = img_enemyShotDiamond[8];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 4. 下側の矢羽根（内向き：＜ ― ＞）
        // 錯視で「短く」見えるが、実際は内側へ入り込むため隙間は狭く【危険】
        // 左端
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX; pShot->y = lineY2 + pEnemyShotSet->count * 2.0;
            pShot->muki = DX_PI * 0.2; // 右斜め上 (内側)
            pShot->speed = 1.0*2; // 少し速くして圧迫感と危険性を出す
            pShot->kind = img_enemyShotDiamond[0]; // 菱形弾, 赤
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX; pShot->y = lineY2 + pEnemyShotSet->count * 2.0;
            pShot->muki = -DX_PI * 0.2; // 右斜め下 (内側)
            pShot->speed = 1.0*2;
            pShot->kind = img_enemyShotDiamond[0];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        // 右端
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX + (numSegments - 1) * stepX; pShot->y = lineY2 + pEnemyShotSet->count * 2.0;
            pShot->muki = DX_PI * 0.8; // 左斜め上 (内側)
            pShot->speed = 1.0*2;
            pShot->kind = img_enemyShotDiamond[0];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = startX + (numSegments - 1) * stepX; pShot->y = lineY2 + pEnemyShotSet->count * 2.0;
            pShot->muki = -DX_PI * 0.8; // 左斜め下 (内側)
            pShot->speed = 1.0*2;
            pShot->kind = img_enemyShotDiamond[0];
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理（線分部分は speed=0 なので移動せず、矢羽根部分のみ動く）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->y += 2.0;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MullerLyer_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 240フレーム(約4秒)ごとに錯視弾幕を発射
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMullerLyer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_d[0] = 140.0 + enemy.x - 240;;
        pEnemyShotSet->param_d[1] = 40.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}