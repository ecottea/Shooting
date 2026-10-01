// ============================================================
//  罔両「禅寺に棲む妖蝶」風弾幕
//  ・渦を巻く蝶(鱗弾)の二重輪
//  ・自機狙いでひらひら舞う蝶(鱗弾)
//  ・禅寺(画面上端)に舞い散る蝶(鱗弾)
//
//  ※ count / pEnemyShotSet->count / pEnemyShot->count の
//    インクリメントと画面外弾の消去はメインルーチンで行われる前提。
//  ※ GetRand(x) は 0 以上 x 以下の x+1 種類の整数を返すので注意。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾セット生成ヘルパー
// ------------------------------------------------------------
static void CreateShotSet(void (*func)(sEnemyShotSet*), double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
// 渦を巻く蝶の輪(主攻撃)
// 緑と青の二重の輪が互い違いに渦を巻き、徐々に回転が弱まって直進する
// ------------------------------------------------------------
static void ShotButterflySwirl(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int ring = 0; ring < 2; ring++) {
            const int    num = 26;
            const int    color = (ring == 0) ? 2 : 4;          // 0:緑の輪 1:青の輪
            const double speed = 2.4 + ring * 0.5;
            const double swirl = (ring == 0) ? -0.022 : 0.022; // 逆方向に渦を巻く
            for (int i = 0; i < num; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki
                    + i * (2.0 * DX_PI / num)
                    + ring * (DX_PI / num);       // 輪の目を互い違いに
                pEnemyShot->speed = speed;
                pEnemyShot->kind = img_enemyShotScale[color];
                pEnemyShot->margin = 240;
                pEnemyShot->param_d[0] = swirl;                 // 1フレームあたりの向き変化(渦の強さ)
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // 渦を巻きながら飛ぶ。徐々に回転が弱まり直進に近づく
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki += pShot->param_d[0];
        pShot->param_d[0] *= 0.992;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 自機狙いの蝶
// 3-way × 3回の鱗弾。正弦的に向きを揺らし、蝶がひらひら舞うような軌道
// ------------------------------------------------------------
static void ShotButterflyAim(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    // 12フレーム間隔で3回発射(計9発)
    if (pEnemyShotSet->count % 12 == 0 && pEnemyShotSet->count <= 24) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 0.18;
            pEnemyShot->speed = 3.6;
            pEnemyShot->kind = img_enemyShotScale[1];          // 黄色の蝶
            // 中心弾と外側弾でひらひらの位相を変える
            pEnemyShot->param_d[0] = 0.05 * ((i == 0) ? 1.0 : -1.0)
                * ((pEnemyShotSet->count / 12) % 2 == 0 ? 1.0 : -1.0);
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 蝶のひらひらした動き: 向きを正弦的に揺らす
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki += sin(pShot->count * 0.25) * pShot->param_d[0];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 禅寺に舞い散る蝶
// 画面上端から白い蝶が1匹降りてくる。左右に揺れながら落下
// ------------------------------------------------------------
static void ShotButterflyRain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 20.0 + GetRand(440);                // 20〜460
        pEnemyShot->y = -10.0;
        pEnemyShot->muki = DX_PI / 2.0;                        // 真下
        pEnemyShot->speed = 1.4 + GetRand(100) / 100.0;         // 1.4〜2.4
        pEnemyShot->kind = img_enemyShotScale[6];              // 白い蝶
        pEnemyShot->param_d[0] = 0.04 + GetRand(40) / 1000.0;   // 揺れの強さ
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    // 落下しながら左右に揺れる(蝶が羽ばたくイメージ)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki = DX_PI / 2.0 + sin(pShot->count * 0.1) * (0.3 + pShot->param_d[0] * 5.0);
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Zendera_Zai()
{
    static int swirlNum; // 渦巻輪の回数(初期向きをずらすのに使用)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        swirlNum = 0;
    }

    // 禅寺(画面上部)を漂うようにゆったり移動する
    enemy.x = 240.0 + sin(count * 0.020) * 110.0;
    enemy.y = 50.0 + sin(count * 0.040) * 18.0;

    // 自機狙いの蝶(鱗弾 3-way)
    if (count % 72 == 5) {
        CreateShotSet(ShotButterflyAim,
            enemy.x, enemy.y + 10.0,
            atan2(player.y - enemy.y, player.x - enemy.x),
            0);
    }

    // 禅寺に舞い散る蝶
    if (count % 12 == 3) {
        CreateShotSet(ShotButterflyRain, 0.0, 0.0, DX_PI / 2.0, 0);
    }

    // 渦を巻く蝶の輪: 予告音 → 80フレーム後に発射
    if (count % 120 == 40) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 120 == 0 && count > 0) {
        // 毎回初期向きをずらして単調にならないようにする
        CreateShotSet(ShotButterflySwirl,
            enemy.x, enemy.y,
            swirlNum * 0.13,
            swirlNum);
        swirlNum++;
    }
}