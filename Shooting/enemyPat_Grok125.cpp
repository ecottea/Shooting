#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：南京玉すだれ風「変形すだれ」
// 縦列の小玉がカーテン状に降りてきて波打ち、途中で扇状に変形して散開する
static void ShotTamasudare(sEnemyShotSet* pEnemyShotSet)
{
    // すだれの列数・段数
    const int COLS = 7 * 2 + 1;
    const int ROWS = 7 * 2;
    const double COL_SPACING = 42.0 / 2;
    const double ROW_SPACING = 10.0;
    const double BASE_SPEED = 1.9;

    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                pEnemyShot = new sEnemyShot;
                // 初期位置：敵の下に縦長のグリッド（すだれ状）
                pEnemyShot->x = pEnemyShotSet->x + (c - (COLS - 1) / 2.0) * COL_SPACING;
                pEnemyShot->y = pEnemyShotSet->y + r * ROW_SPACING;
                // 真下方向
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = BASE_SPEED;
                // 緑の小玉（すだれの竹をイメージ）
                pEnemyShot->kind = img_enemyShotSmallBall[2];
                // パラメータ保存
                pEnemyShot->param_i[0] = c;                 // 列番号
                pEnemyShot->param_i[1] = r;                 // 段番号
                pEnemyShot->param_d[0] = (c - 3.0) * 0.35;  // 波の位相オフセット
                pEnemyShot->param_d[1] = BASE_SPEED;        // 基本速度を保持

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレームの弾更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int setCnt = pEnemyShotSet->count;
        const int col = pShot->param_i[0];
        const double phase = pShot->param_d[0];
        const double baseSpd = pShot->param_d[1];

        double vx = 0.0;
        double vy = baseSpd;

        if (setCnt < 80) {
            // フェーズ1：波打ち下降（すだれを揺らす）
            // 列ごとに位相をずらして左右にうねらせる
            vx = sin(pShot->count * 0.13 + phase) * 1.6;
            vy = baseSpd;
        }
        else if (setCnt < 140) {
            if (setCnt == 80) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }

            // フェーズ2：変形（扇状に開く／折りたたむ）
            // 外側の列ほど大きく外側へ、内側はほぼ真下
            double t = (setCnt - 80) / 60.0;          // 0→1
            double fan = (col - COLS/2) * 0.55 * t;      // 扇の開き具合
            vx = sin(fan) * (baseSpd + t * 1.2);
            vy = cos(fan) * (baseSpd + t * 0.8);
            // 少し加速
            pShot->param_d[1] = baseSpd + t * 0.06;
        }
        else {
            if (setCnt == 140) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // フェーズ3：散開（すだれを解く）
            // 現在の速度を維持しつつ、外側へ少しだけ広げる
            double t = (setCnt - 140) / 50.0;
            if (t > 1.0) t = 1.0;
            vx = pShot->speed * cos(pShot->muki) + (col - COLS/2) * 0.3 * t;
            vy = pShot->speed * sin(pShot->muki);
        }

        // 速度ベクトルから muki と speed を再設定
        pShot->muki = atan2(vy, vx);
        pShot->speed = sqrt(vx * vx + vy * vy);
        if (pShot->speed < 0.1) pShot->speed = 0.1;

        // 位置更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NankinTamasudare_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (count % 160 == 80) muki *= -1;
        // 画面端で反転（念のため）
        if (enemy.x < 70.0) muki = 1;
        if (enemy.x > 410.0) muki = -1;
    }

    // 一定間隔で「すだれ」を展開
    if (count % 50 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTamasudare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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