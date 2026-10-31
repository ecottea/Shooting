#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 転世「一条戻り橋」再現
// 仕組み:
//  - 自機の背後(画面下外  y=500)から自機に向けて弾を発射する逆流弾
//  - 前方(敵本体)から高速自機狙い5wayを重ねて挟み撃ち
// 原作解説: 「背後から自分に向けて弾を発射」「横移動で避ける」
// ------------------------------------------------------------

// 背後からの逆流弾パターン
static void ShotReverseBridge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 7～9発で隙間を作る。GetRand(x)は0～xの x+1種類
        int n = 7 + GetRand(2); // 7,8,9
        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;
            // 自機背後スポナーからの横ブレ
            pEnemyShot->x = pEnemyShotSet->x + GetRand(120) - 60;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            // pEnemyShotSet->mukiは生成時点で自機狙いに設定済み
            double spread = (GetRand(60) - 30) / 180.0 * DX_PI; // +-30度のランダム
            if (n > 1) {
                double norm = (double)i / (n - 1) - 0.5; // -0.5～0.5
                spread += norm * 0.6; // 等間隔扇状に寄せる
            }
            pEnemyShot->muki = pEnemyShotSet->muki + spread;
            pEnemyShot->speed = 2.2 + GetRand(80) / 100.0; // 2.2～3.0 遅めで横移動で避けさせる
            // 素材選定: 小玉(2.5x2.5)で密度を出しつつ視認性確保。幽世感で青/シアン
            int col = (i % 2 == 0) ? 4 : 3; // 4:青 3:シアン
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            pEnemyShot->count = 0;

            // 双方向リスト連結
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

// 前方からの自機狙い高速弾(挟み撃ち用)
static void ShotFrontBridge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = 5;
        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double base = pEnemyShotSet->muki;
            double offset = ((double)i - (n - 1) / 2.0) * (12.0 / 180.0 * DX_PI); // 12度間隔
            offset += (GetRand(10) - 5) / 180.0 * DX_PI; // 微ランダムで人間味
            pEnemyShot->muki = base + offset;
            pEnemyShot->speed = 3.8 + GetRand(60) / 100.0; // 3.8～4.4 原作の「速め」再現
            // 素材選定: 中玉(7.0x7.0)で脅威感。色は赤/白で前方弾と識別
            int col = (i == 2) ? 6 : 0; // 中央だけ白(6)、他赤(0)
            pEnemyShot->kind = img_enemyShotMediumBall[col];

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

// 敵本体
void EnemyPat_Modoribashi_MetaAI()
{
    static int dir;
    static int kindCounter;
    if (count == 1) {
        // 画面は480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 150;
        dir = 1;
        kindCounter = 0;
    }
    else {
        // 橋の上でゆっくり往復する動き
        enemy.x += 0.5 * (double)dir;
        if (enemy.x < 110.0) dir = 1;
        if (enemy.x > 370.0) dir = -1;
        // わずかに上下動で浮遊感
        enemy.y = 70.0 + sin(count * 0.02) * 8.0;
    }

    // 1) 背後からの逆流：20F毎に自機背後から発射
    if (count % 20 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReverseBridge;
        // 自機の少し後ろ(画面外下)にスポナー設置。これが「戻り橋」の核
        pEnemyShotSet->x = player.x + GetRand(80) - 40;
        pEnemyShotSet->y = 500.0; // 480より下
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = kindCounter++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 2) 前方からの挟み撃ち：60F毎に敵本体から高速自機狙い
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFrontBridge;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = kindCounter;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}