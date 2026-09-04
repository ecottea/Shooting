// enemyPat_dopplerCompression.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：赤方交差「ドップラー・コンプレッション」
static void ShotDopplerCompression(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 高頻度で弾を撃ち出すため、重い音だとノイズになるので軽い音を連続で鳴らす
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        
        int way = 32; // 全方位に放つ弾の数
        double v0 = 2.5; // 基準となる弾の発射速度
        double v_boss = pEnemyShotSet->param_d[0]; // 発射時のボスの移動速度(X成分)

        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 基準となる発射角度
            double angle = (DX_PI * 2.0 / way) * i;

            // 【ドップラー効果の肝】
            // 弾の初速ベクトルにボスの移動ベクトルを合成する。
            // これにより、前方の弾は高速化し、後方の弾は低速化する。
            double vx = v0 * cos(angle) + v_boss * 0.85; // ボスの速度の85%を乗せる
            double vy = v0 * sin(angle);

            // 合成後の実際の速度と向き
            pEnemyShot->speed = sqrt(vx * vx + vy * vy);
            pEnemyShot->muki = atan2(vy, vx);

            // ボスの進行方向（右:1.0, 左:-1.0）
            double dir_x = (v_boss > 0.0) ? 1.0 : -1.0;

            // 弾の基準発射角度が、ボスの進行方向にどれだけ近いか（内積：1.0 なら真正面、-1.0 なら真後ろ）
            double dot = cos(angle) * dir_x;

            // 位置取りによって色と形を変化させる
            if (dot >= 0.6) {
                // 進行前方（ブルーシフト極大）：青の短レーザー（鋭い針）
                pEnemyShot->kind = img_enemyShotLaser[4];
            }
            else if (dot >= 0.0) {
                // 進行前方斜め：白の菱形弾（やや鋭い）
                pEnemyShot->kind = img_enemyShotDiamond[6];
            }
            else if (dot >= -0.6) {
                // 進行後方斜め：橙の中玉（丸い）
                pEnemyShot->kind = img_enemyShotMediumBall[8];
            }
            else {
                // 進行後方（レッドシフト極大）：赤の中玉（丸い）
                pEnemyShot->kind = img_enemyShotMediumBall[0];
            }

            // 双方向循環リストへの追加処理
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Doppler_Gemini()
{
    static double vx;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 100;
        vx = 8.0 - 4.5; // かなりの高速で往復させる
    }
    else {
        // ボスの移動と画面端での反転
        enemy.x += vx;
        if (enemy.x > 440.0) {
            enemy.x = 440.0;
            vx *= -1.0;
        }
        else if (enemy.x < 40.0) {
            enemy.x = 40.0;
            vx *= -1.0;
        }
    }

    // 波の圧縮（ドップラー効果）を見せるため、わずか数フレーム間隔で全方位弾を連射する
    if (count % 8 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDopplerCompression;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 発射時のボスの移動速度(X成分)を弾幕関数へ渡す
        pEnemyShotSet->param_d[0] = vx;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}