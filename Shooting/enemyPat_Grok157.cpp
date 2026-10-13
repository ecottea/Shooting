#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 超新星爆発モチーフ弾幕
// 既存弾素材のみ使用：中玉・小玉・菱形弾・大玉・短レーザー
// 色：青/シアン（崩壊）、白/黄/橙（爆発）、青〜黒系（残骸）
// ============================================================

// 超新星シーケンス用ショットセット
static void ShotSupernova(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---------- 崩壊フェーズ開始（count==0）----------
    // 周囲に円形配置した弾を中心へ収束させる
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const int num = 16*2;
        const double radius = 130.0;
        const double arriveFrames = 55.0; // このフレーム数で中心到達予定

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (double)i * 2.0 * DX_PI / (double)num;
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            // 中心方向へ向ける
            pEnemyShot->muki = atan2(pEnemyShotSet->y - pEnemyShot->y, pEnemyShotSet->x - pEnemyShot->x);
            pEnemyShot->speed = radius / arriveFrames;
            // 崩壊弾：中玉・青〜シアン系
            if (i % 2 == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青
            else
                pEnemyShot->kind = img_enemyShotMediumOval[3];  // シアン
            pEnemyShot->param_i[0] = 1; // 崩壊弾マーカー（必要なら加速用）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---------- 爆発フェーズ（count==55）----------
    // 中心から高速放射＋短レーザーで衝撃波を表現
    if (pEnemyShotSet->count == 55) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 放射弾 24方向
        const int radialNum = 24*2;
        for (int i = 0; i < radialNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // わずかにランダムを加えて不規則さを出す
            double baseAngle = (double)i * 2.0 * DX_PI / (double)radialNum;
            pEnemyShot->muki = baseAngle + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.8 + GetRand(180) / 100.0; // 3.8〜5.6

            // 白・黄・橙を混ぜて閃光感
            int colSelect = GetRand(2);
            if (colSelect == 0)
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            else if (colSelect == 1)
                pEnemyShot->kind = img_enemyShotDiamond[1];    // 黄
            else
                pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 短レーザー 8方向（衝撃波）
        const int laserNum = 8*2;
        for (int i = 0; i < laserNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (double)i * DX_PI / 4.0/2;
            pEnemyShot->speed = 2.5 + GetRand(50) / 100.0;
            // 白 or 橙レーザー
            if (i % 2 == 0)
                pEnemyShot->kind = img_enemyShotLaser[6]; // 白
            else
                pEnemyShot->kind = img_enemyShotLaser[8]; // 橙
            pEnemyShot->margin = 80;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---------- 残骸拡散フェーズ（count 60〜200）----------
    // 低速・ランダム方向の大玉・中玉を徐々にばら撒く
    if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count <= 200 && (pEnemyShotSet->count % 5 == 0)) {
        // 音は控えめに（最初だけ）
        if (pEnemyShotSet->count == 60) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        const int debrisPer = 4*2;
        for (int j = 0; j < debrisPer; j++) {
            pEnemyShot = new sEnemyShot;
            // 中心付近からわずかに散らして発生
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(60) - 30);
            // 全方向ランダム
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.5 + GetRand(140) / 100.0; // 0.5〜1.9

            // 残骸らしく暗めの色＋大きめの弾を優先
            int col = 3 + GetRand(4); // 3〜7（シアン〜黒）
            if (GetRand(2) == 0)
                pEnemyShot->kind = img_enemyShotLargeBall[col];  // 大玉
            else if (GetRand(1) == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[col]; // 中玉
            else
                pEnemyShot->kind = img_enemyShotScale[col];      // 鱗弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---------- 全弾の位置更新（毎フレーム）----------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 崩壊弾はわずかに加速させて収束感を強める（任意）
        if (pShot->param_i[0] == 1 && pEnemyShotSet->count < 55) {
            pShot->speed += 0.025;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// 関数名は指定通り void EnemyPat_Supernova_Grok()
// ============================================================
void EnemyPat_Supernova_Grok()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 110.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // 超新星の「脈動」をイメージしたゆるやかな円運動
        enemy.x = 240.0 + 28.0 * sin(count / 45.0);
        enemy.y = 110.0 + 12.0 * cos(count / 70.0);
    }

    // 約4秒間隔（240フレーム）で超新星シーケンスを発動
    // count==1 の直後から始まるように調整
    if (count % 240 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSupernova;
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