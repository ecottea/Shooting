#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// フェーズ1：重力崩壊（チャージ・収縮）
// 外周から中心（ボス付近）へ向かって、青・白の小粒が吸い込まれるように加速して集まります。
// ------------------------------------------------------------
static void ShotGravityCollapse(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // GetRand(x) は 0 から x までの x+1 種類の整数を返す
            double angle = (double)i / 16.0 * 2.0 * DX_PI + (GetRand(50) / 100.0);
            double dist = 250.0 + GetRand(50)+50;
            pEnemyShot->x = pEnemyShotSet->x + dist * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(angle);
            // ボスの中心へ向かう逆ベクトル
            pEnemyShot->muki = angle + DX_PI;
            pEnemyShot->speed = 1.0 + GetRand(100) / 100.0;

            // 色一覧: 4:青、6:白 の小玉を使用
            int colorIdx = (GetRand(1) == 0) ? 4 : 6;
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾が中心に向かって収縮しながら加速する演出
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->speed += 0.04; // フレームごとに加速
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ2：超新星爆発（衝撃波の解放）
// ボスの中心から全方位（32WAY）へ、赤・橙の中玉が超高速で放たれます。
// ------------------------------------------------------------
static void ShotSupernovaExplosion(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int way = 32;
        for (int i = 0; i < way; i++) for (int j = 0; j < 5; j++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (double)i / way * 2.0 * DX_PI;
            pEnemyShot->speed = 6.0 + GetRand(200) / 100.0 - j;

            // 色一覧: 0:赤、8:橙 の中玉を使用
            int colorIdx = (i % 2 == 0) ? 0 : 8;
            pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ3-1：残骸の飛散（ネビュラ）
// 爆発の余韻として、黄緑・マゼンタの菱形弾がゆっくりとうねりながら漂います。
// ------------------------------------------------------------
static void ShotNebulaDrift(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 6*2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(100) - 50;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(50) - 25;
            pEnemyShot->muki = (GetRand(180) - 90) / 180.0 * DX_PI + DX_PI / 2.0;
            pEnemyShot->speed = 1.0 + GetRand(100) / 100.0;

            // 色一覧: 2:緑（黄緑系）、5:マゼンタ の菱形弾を使用
            int colorIdx = (i % 2 == 0) ? 2 : 5;
            pEnemyShot->kind = img_enemyShotDiamond[colorIdx];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 微小にカーブを描かせて星雲の漂う雰囲気を演出
        pShot->muki += 0.01 * (pShot->count % 2 == 0 ? 1 : -1);
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ3-2：極ジェット
// 上下に向けてシアンの短レーザーを激しく放射します。
// ------------------------------------------------------------
static void ShotPolarJet(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double angles[2] = { DX_PI / 2.0, -DX_PI / 2.0 }; // 真下、真上
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angles[i];
            pEnemyShot->speed = 5.0;

            // 色一覧: 3:シアン の短レーザーを使用
            pEnemyShot->kind = img_enemyShotLaser[3];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン（関数名：void EnemyPat_Supernova_Gemini()）
// ------------------------------------------------------------
void EnemyPat_Supernova_Gemini()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // ボスが中央付近を左右に移動
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 140.0 || enemy.x > 340.0) muki *= -1;
    }

    // 450フレームを1サイクルとして、超新星爆発の一連の流れをループ
    int cycle = (count - 1) % 450;

    // フェーズ1：重力崩壊（チャージ・収縮） (0 〜 119フレーム目)
    if (cycle >= 0 && cycle < 120) {
        if (cycle % 20 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotGravityCollapse;
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
    }
    // フェーズ2：超新星爆発の瞬間 (125フレーム目)
    else if (cycle == 125) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSupernovaExplosion;
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
    // フェーズ3：残骸の飛散と極ジェット (150 〜 419フレーム目)
    else if (cycle >= 150 && cycle < 420) {
        // 残骸（ネビュラ）の生成
        if (cycle % 30 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotNebulaDrift;
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
        // 極ジェットの生成
        if (cycle % 15 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotPolarJet;
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
    }
}