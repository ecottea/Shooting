#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// 弾幕パターン：土俵（固定弾）
// ============================================================
static void ShotDohyo(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 画面中央(240, 240)から半径220の円周上に固定弾を配置（画面端の表現）
        int num = 64;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double rad = (double)i * 2.0 * DX_PI / (double)num;
            pEnemyShot->x = 240.0 + 220.0 * cos(rad);
            pEnemyShot->y = 240.0 + 220.0 * sin(rad);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0; // 固定弾として速度を0にする
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 6:白い小玉（土俵の境界表現）
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 固定弾のため、移動処理は記述しない
}

// ============================================================
// 弾幕パターン：仕切り（自機狙い＋ばら撒き）
// ============================================================
static void ShotShikiri(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 0:赤い中玉
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

// ============================================================
// 弾幕パターン：張り手（突進時の扇状高速弾）
// ============================================================
static void ShotHarite(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 7; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 3) * 0.3;
            pEnemyShot->speed = 4.0 + GetRand(100) / 50.0;
            pEnemyShot->kind = img_enemyShotDiamond[1]; // 1:黄色い菱形弾
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

// ============================================================
// 弾幕パターン：四股踏み（着地衝撃波）
// ============================================================
static void ShotShiko(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * 2.0 * DX_PI / 16.0;
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 4:青い大玉
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

// ============================================================
// 敵本体のパターン：大相撲絵巻「鬼神の立ち合い」
// ============================================================
void EnemyPat_Sumou_Qwen()
{
    static int phase = 0;
    static int timer = 0;
    static double dash_muki = 0.0;
    static double dash_speed = 0.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        timer = 0;

        // 土俵の固定弾を初期化時に一度だけ生成
        sEnemyShotSet* pDohyo = new sEnemyShotSet;
        pDohyo->count = 0;
        pDohyo->patternFunc = ShotDohyo;
        pDohyo->x = 240.0;
        pDohyo->y = 240.0;
        pDohyo->muki = 0.0;
        pDohyo->kind = 0;
        pDohyo->pEnemyShotHead = new sEnemyShot;
        pDohyo->pEnemyShotHead->prev = pDohyo->pEnemyShotHead;
        pDohyo->pEnemyShotHead->next = pDohyo->pEnemyShotHead;
        pDohyo->prev = enemyShotSetHead.prev;
        pDohyo->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pDohyo;
        enemyShotSetHead.prev = pDohyo;
    }

    timer++;

    // フェーズ0: 仕切り
    if (phase == 0) {
        enemy.y = 100.0 + sin(timer * 0.05) * 20.0; // 軽く揺れて気合を溜める演出

        if (timer % 60 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotShikiri;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 20.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        if (timer > 180) {
            phase = 1;
            timer = 0;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 突進予告音
        }
    }
    // フェーズ1: 突進予告（間）
    else if (phase == 1) {
        if (timer > 60) {
            phase = 2;
            timer = 0;
            dash_muki = atan2(player.y - enemy.y, player.x - enemy.x);
            dash_speed = 7.0;
        }
    }
    // フェーズ2: 突進と斥力
    else if (phase == 2) {
        enemy.x += dash_speed * cos(dash_muki);
        enemy.y += dash_speed * sin(dash_muki);

        // 画面端に当たったら簡易的に跳ね返る
        if (enemy.x < 40.0 || enemy.x > 440.0 || enemy.y < 40.0 || enemy.y > 440.0) {
            dash_muki += DX_PI;
        }

        // 斥力の判定（ボスと自機が近いときのみ）
        double dist = hypot(enemy.x - player.x, enemy.y - player.y);
        if (dist < 150.0) {
            double pullSpeed = 6.0; // 引き寄せの強さ
            double angle = atan2(player.y - enemy.y, player.x - enemy.x); // 引き寄せの向き（ボスから自機へのベクトル＝斥力方向）
            double ratio = 3.0; // 比例定数
            spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);

            // player.x, player.yを変更することで斥力を発生させる
            player.x += pullSpeed * cos(angle);
            player.y += pullSpeed * sin(angle);

            dash_speed *= 0.85; // 衝突によりボスも減速
        }

        // 突進中に「張り手」をばら撒く
        if (timer % 8 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotHarite;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = dash_muki;
            pEnemyShotSet->kind = 1;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        if (timer > 120 || dash_speed < 1.5) {
            phase = 3;
            timer = 0;
        }
    }
    // フェーズ3: 四股踏み（着地衝撃波）
    else if (phase == 3) {
        dash_speed = 0.0;
        enemy.y = 120.0; // 着地位置へ戻す

        if (timer == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotShiko;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 2;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        if (timer > 90) {
            phase = 0; // パターンループ
            timer = 0;
        }
    }
}