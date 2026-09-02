// enemyPat_tmp.cpp
// ひまわり弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#define PI 3.14159265358979323846

// ひまわり弾幕のショットセット用パターン関数
static void SunflowerPattern(sEnemyShotSet* pEnemyShotSet)
{
    // 各フェーズの長さ（フレーム数）
    const int PHASE1_DURATION = 60; // 花盤のみ
    const int PHASE2_DURATION = 180; // 花弁
    const int PHASE3_DURATION = 180; // 種子＋花弁
    const int TOTAL_CYCLE = PHASE1_DURATION + PHASE2_DURATION + PHASE3_DURATION;

    int count = pEnemyShotSet->count;
    int phaseCount = count % TOTAL_CYCLE;

    int phase;
    if (phaseCount < PHASE1_DURATION) {
        phase = 0; // 花盤
    }
    else if (phaseCount < PHASE1_DURATION + PHASE2_DURATION) {
        phase = 1; // 花弁
    }
    else {
        phase = 2; // 種子＋花弁
    }

    // phaseCountを現在フェーズ内の経過フレームに変換
    if (phase == 0) {
        // phaseCountはそのまま
    }
    else if (phase == 1) {
        phaseCount -= PHASE1_DURATION;
    }
    else {
        phaseCount -= (PHASE1_DURATION + PHASE2_DURATION);
    }

    // ショットセットの基準座標を敵の現在位置に合わせる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 初期化（count==0 のときのみ実行）
    if (count == 0) {
        // 花盤（中心の円盤）を形成する小玉を配置
        const int diskBulletCount = 20;     // 円周上の弾数
        const double diskRadius = 30.0;     // 半径

        for (int i = 0; i < diskBulletCount; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (2.0 * PI * i) / diskBulletCount;

            pShot->x = enemy.x + diskRadius * cos(angle);
            pShot->y = enemy.y + diskRadius * sin(angle);
            pShot->muki = 0.0;
            pShot->speed = 0.0;                       // 速度0で固定
            pShot->kind = img_enemyShotSmallBall[8];  // オレンジ色（茶色の代用）
            pShot->param_i[0] = 0;                    // 種類フラグ: 0=花盤弾
            pShot->param_d[0] = angle;                // 初期角度オフセット

            // リンクリストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // パラメータ初期化
        pEnemyShotSet->param_d[0] = 0.0;  // 花盤の現在回転角度
        pEnemyShotSet->param_d[1] = 0.0;  // 種子の現在発射角度
    }

    // 花盤の回転角度を更新（毎フレーム約1.15度）
    pEnemyShotSet->param_d[0] += 0.02;

    // 全弾の座標更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 花盤弾：敵の周囲を公転させる
            double diskRadius = 30.0;
            double offsetAngle = pShot->param_d[0];
            double rotAngle = pEnemyShotSet->param_d[0];
            pShot->x = enemy.x + diskRadius * cos(offsetAngle + rotAngle);
            pShot->y = enemy.y + diskRadius * sin(offsetAngle + rotAngle);
        }
        else {
            // 通常弾（花弁・種子）：速度と向きに従って移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // フェーズ2（花弁）またはフェーズ3（花弁＋種子）で花弁を発射
    if (phase == 1 || phase == 2) {
        const int petalInterval = 30 / 2;   // 発射間隔
        if (phaseCount % petalInterval == 0) {
            int petalDirections = 12;   // 12方向
            double petalRot = count * 0.02; // 花弁全体の回転

            for (int dir = 0; dir < petalDirections; dir++) {
                double baseAngle = petalRot + (2.0 * PI * dir) / petalDirections;
                // 各方向に3連（角度差±0.1rad）
                for (int j = -1; j <= 1; j++) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = enemy.x;
                    pShot->y = enemy.y;
                    pShot->muki = baseAngle + j * 0.1 / 2;
                    pShot->speed = (150.0 + GetRand(20)) / 100; // 150～170
                    pShot->kind = img_enemyShotBullet[1]; // 黄色い針弾
                    pShot->param_i[0] = 1;                // 花弁弾フラグ

                    // リンクリストに追加
                    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pShot;
                }
            }

            // 効果音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    // フェーズ3（種子）で黄金角スパイラル状に種子を発射
    if (phase == 2) {
        const int seedInterval = 8 / 8;     // 発射間隔
        if (phaseCount % seedInterval == 0) {
            const double GOLDEN_ANGLE = 2.399963229728653; // 137.5度（ラジアン）

            double angle = pEnemyShotSet->param_d[1];
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = angle;
            pShot->speed = 200.0 / 100;
            pShot->kind = img_enemyShotSmallBall[7]; // 黒い小玉（種子）
            pShot->param_i[0] = 2;                   // 種子弾フラグ

            // リンクリストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            // 次の種子の発射角度を黄金角だけ進める
            pEnemyShotSet->param_d[1] += GOLDEN_ANGLE;

            // 軽い効果音
            if (phaseCount % 8 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }
}

// 敵本体パターン（ひまわり）
void EnemyPat_Sunflower_DeepSeek()
{
    static int muki;          // 移動方向
    static int shot_count;    // 未使用だがサンプルに合わせて保持

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;

        // ひまわり弾幕用のショットセットを1つだけ作成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = SunflowerPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットを全体リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵の左右移動（ゆっくり）
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;

        // 画面端で反転
        if (enemy.x < 40) { enemy.x = 40; muki = 1; }
        if (enemy.x > 440) { enemy.x = 440; muki = -1; }
    }
}