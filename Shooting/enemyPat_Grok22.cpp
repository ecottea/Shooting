// enemyPat_Tmp_rhythm.cpp
// 音ゲーモチーフ弾幕パターン: "Rhythm Beat Cascade"
// - 敵は軽快に左右に動きながら「ビート」に合わせて弾を放つ
// - メインリズム: 4/4拍子風に強弱をつけた連射 + 円形バースト + 波状ノート
// - 視覚的に音ゲーらしい「落ちてくるノート」や「レーン」を意識した配置

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotRhythmBeat(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int c = pEnemyShotSet->count;

    // ビートごとに異なる射出（メインルーチンでcount++される）
    if (c == 0) {
        // 強ビート: 重めの音 + 大玉中心のバースト
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (i * DX_PI * 2 / 12) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.8 + GetRand(60) / 100.0;

            int col = (i % 3 == 0) ? 0 : (i % 3 == 1 ? 4 : 2); // 赤・青・緑のリズムカラー
            pEnemyShot->kind = img_enemyShotLargeBall[col];

            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (c == 8) {
        // 弱ビート1: 小玉の高速ノート連打（音ゲーの高速タップ風）
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = -3; i <= 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + i * 25;
            pEnemyShot->y = pEnemyShotSet->y - 10;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + (i * 0.08);
            pEnemyShot->speed = 4.2;

            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(5) + 1]; // 明るい色中心

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (c == 16) {
        // 弱ビート2: 鱗弾＋菱形の波状散布（波形ビジュアライザ風）
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(80) - 40;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 3.5) * 0.35;
            pEnemyShot->speed = 2.5 + sin(c * 0.3) * 0.8;

            int type = GetRand(1);
            if (type == 0)
                pEnemyShot->kind = img_enemyShotScale[GetRand(6)];
            else
                pEnemyShot->kind = img_enemyShotDiamond[GetRand(6)];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの移動処理（直進＋軽い波打ちでリズム感）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double wave = sin(pEnemyShot->count * 0.2) * 0.8; // 音ゲーらしい波
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);// +wave;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot->count++; // 個別カウントも更新（任意）
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体パターン
void EnemyPat_RhythmGame_Grok()
{
    static int phase = 0;
    static double baseX = 240.0;
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        baseX = 240.0;
        muki = 1;
        phase = 0;
    }
    else {
        // 軽快な左右移動（シンコペーション風に少し加速）
        double moveSpeed = 1.35 + sin(count * 0.05) * 0.4;
        enemy.x += moveSpeed * (double)muki;
        if (enemy.x < 120 || enemy.x > 360) muki *= -1;

        // 軽い上下ボブ（リズムに同期）
        enemy.y = 60.0 + sin(count * 0.12) * 18.0;
    }

    // リズム周期（約4/4拍子を意識した60フレーム周期）
    if (count % 30 == 0 ) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRhythmBeat;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向を基本に、少しオフセットでリズム感を出す
        double targetAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->muki = targetAngle + (GetRand(40) - 20) / 180.0 * DX_PI * 0.6;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}