#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕:マシンガン「ワイルドスイープ(荒れない掃射)」
//
// 1サイクルの流れ(フレーム数 / 60fps想定)
//   2F        : ため開始(予告音)
//   32F       : ための締め → プレイヤー狙いの大玉を1発(低速)
//   33〜53F   : 1回目の掃射(銃弾を4F間隔で6連射、角度ランダム±10度)
//   63〜83F   : 2回目の掃射(同6連射、狙いを少し右へ振って掃射)
//   93F       : 締めに7方向の鱗弾を拡散
//   150F      : サイクル終了 → リセット
// ============================================================

#define MG_CYCLE         150+20  // 1サイクルの長さ
#define MG_CHARGE_SHOT    32  // 大玉発射フレーム
#define MG_BURST1_START   33  // 1回目の掃射開始
#define MG_BURST1_END     53
#define MG_BURST2_START   63  // 2回目の掃射開始
#define MG_BURST2_END     83
#define MG_SPREAD_FRAME   93  // 締めの拡散弾

// 弾幕本体
static void ShotMachineGun(sEnemyShotSet * pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int c = pEnemyShotSet->count;

    // サイクル終了 → カウントを戻して繰り返す
    if (c >= MG_CYCLE) {
        pEnemyShotSet->count = 0;
        c = 0;
    }

    // ---- ため(予告音) ----
    //if (c == 2) {
    //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    // ---- ための締め:プレイヤー狙いの大玉(低速・プレッシャー役) ----
    if (c == MG_CHARGE_SHOT) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShot->speed = 1.8;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 大玉:赤
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ---- マシンガン掃射(4フレーム間隔で6連射×2バースト) ----
    if ((c >= MG_BURST1_START && c <= MG_BURST1_END && (c - MG_BURST1_START) % 4 == 0) ||
        (c >= MG_BURST2_START && c <= MG_BURST2_END && (c - MG_BURST2_START) % 4 == 0)) {

        // バースト先頭で連射音
        if (c == MG_BURST1_START || c == MG_BURST2_START) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 毎発プレイヤー方向を狙い直し
        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        // 1バースト目は左へ、2バースト目は右へ振って掃射する
        double sweep = (c >= MG_BURST2_START) ? 4.0 : -4.0;
        // 散布度:発射角度をランダムに±10度揺らす(集弾しつつブレる)
        double angle = aim + sweep / 180.0 * DX_PI + (GetRand(20) - 10) / 180.0 * DX_PI;

        pEnemyShot = new sEnemyShot;
        // 銃口のブレ
        pEnemyShot->x = pEnemyShotSet->x + GetRand(8) - 4; // -4〜4
        pEnemyShot->y = pEnemyShotSet->y + GetRand(8) - 4;
        pEnemyShot->muki = angle;
        pEnemyShot->speed = 6.0 + GetRand(100) / 100.0; // 6.0〜7.0 の速い小弾
        // 銃弾:発射ごとに白/黄を切り替えて曳光弾(トレーサー)っぽく
        int col = (GetRand(1) == 0) ? 6 : 1; // 6:白 1:黄
        pEnemyShot->kind = img_enemyShotBullet[col];
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ---- 締め:「ダダダッ、バラッ!」7方向の鱗弾を拡散 ----
    if (c == MG_SPREAD_FRAME) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = -3; i <= 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = aim + i * 4.0 / 180.0 * DX_PI; // 4度間隔で7方向
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotScale[4]; // 鱗弾:青
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 既存弾の移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 掃射しながら左右に揺れる(弾道が横に舐めるイメージ)
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // マシンガン1サイクル(150F)+ひと呼吸おいて次のサイクルを開始
    if (count % 180 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMachineGun;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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
}