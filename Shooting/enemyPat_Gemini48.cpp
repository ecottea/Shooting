// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：大噴流「アクア・プレッシャー」
// 画面最下部から真上に向けて大量の水を噴き上げ、画面上部で失速して激しい水飛沫（放物線落下）に変化させる
static void ShotAquaPressure(sEnemyShotSet* pEnemyShotSet)
{
    // ------------------------------------------------------------------------
    // 1. 弾の新規生成フェーズ（pEnemyShotSet->count に応じて挙動を切り替え）
    // ------------------------------------------------------------------------

    // 【フェーズ①：充填（予兆線）】（0 〜 59フレーム）
    if (pEnemyShotSet->count < 60) {
        // フェーズ開始時に予告音を再生
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 4フレームに1回、細いレーザーを真上に高速で撃ち上げて予兆線とする
        if (pEnemyShotSet->count % 4 == 0) {
            sEnemyShot* pNew = new sEnemyShot;
            pNew->x = pEnemyShotSet->x + (GetRand(20) - 10); // 中心から左右に少しブレさせる
            pNew->y = pEnemyShotSet->y;
            pNew->muki = -DX_PI / 2.0;                       // 真上（-90度）
            pNew->speed = 15.0;                              // 超高速で突き抜ける
            pNew->kind = img_enemyShotLaser[3];              // 短レーザー（シアン）
            pNew->param_i[0] = 2;                            // 状態2: 予兆弾（直進のみ）

            // リストの末尾に追加
            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }
    // 【フェーズ②：本射（圧倒的な垂直噴流）】（60 〜 139フレーム）
    else if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count < 140) {
        // 本射開始時および一定間隔で激しい発射音を再生
        if (pEnemyShotSet->count == 60 || pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 激しい噴流（水柱）を表現するため、毎フレーム3発ずつ高密度で生成
        for (int i = 0; i < 1; i++) {
            sEnemyShot* pNew = new sEnemyShot;
            pNew->x = pEnemyShotSet->x + (GetRand(30) - 15); // 水柱の太さ（幅30px）
            pNew->y = pEnemyShotSet->y;
            pNew->muki = -DX_PI / 2.0 + (GetRand(80) - 40) / 1000.0; // ほぼ真上（わずかな揺らぎ）
            pNew->speed = 8.0 + (GetRand(200) / 100.0);      // 8.0 〜 10.0 の高速噴射

            // 青とシアンの弾を混ぜて濁流の質感を演出
            if (GetRand(1) == 0) {
                pNew->kind = img_enemyShotLargeBall[4];      // 大玉（青）
            }
            else {
                pNew->kind = img_enemyShotMediumOval[3];     // 中楕円弾（シアン）
            }
            pNew->param_i[0] = 0;                            // 状態0: 上昇水流

            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }
    // 【フェーズ③：飛沫（時間差の全方位拡散）】（140フレーム以降）
    // 新規の弾生成は停止。あとは打ち上がった弾が飛沫となって降り注ぐインターバル（ボスの無防備時間）

    // ------------------------------------------------------------------------
    // 2. 既存弾の移動・状態更新ループ
    // ------------------------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ループ内で分裂（弾の追加）が発生しても破綻しないよう、予め次のポインタを保持
        sEnemyShot* pNextShot = pShot->next;

        // --- 状態0: 上昇水流 ---
        if (pShot->param_i[0] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 上空へ進むにつれて急激に失速させる（潮吹き演出）
            pShot->speed *= 0.985;

            // 速度が落ちきるか、画面上部（y < 80）に到達したら激しく飛沫化
            if (pShot->speed < 1.2 || pShot->y < 80.0) {
                pShot->kind = img_enemyShotSmallBall[6]; // 小玉（白）に変形
                pShot->param_i[0] = 1;                   // 状態1: 飛沫落下へ移行

                // 元の弾の放物線用の初速を設定（斜め上〜真上方向へ広がるように散らす）
                double angle = -DX_PI / 2.0 + (GetRand(200) - 100) / 100.0 * (DX_PI * 0.4);
                double spd = 1.0 + (GetRand(250) / 100.0); // 1.0 〜 3.5
                pShot->param_d[0] = spd * cos(angle);      // param_d[0] = X速度成分
                pShot->param_d[1] = spd * sin(angle);      // param_d[1] = Y速度成分

                // 周囲へさらに細かな水飛沫を飛び散らせる（分裂生成）
                int burst = 1 + GetRand(2); // 1〜3個追加
                for (int b = 0; b < burst; b++) {
                    sEnemyShot* pSp = new sEnemyShot;
                    pSp->x = pShot->x;
                    pSp->y = pShot->y;
                    pSp->kind = img_enemyShotSmallBall[6]; // 小玉（白）
                    pSp->param_i[0] = 1;                   // 状態1: 飛沫落下
                    pSp->margin = 100;

                    // 360度全方位へ美しく拡散
                    double spAngle = (GetRand(360) / 180.0) * DX_PI;
                    double spSpd = 0.5 + (GetRand(200) / 100.0); // 0.5 〜 2.5
                    pSp->param_d[0] = spSpd * cos(spAngle);
                    pSp->param_d[1] = spSpd * sin(spAngle);

                    // 新しい飛沫弾をリストの末尾に挿入
                    pSp->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSp->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSp;
                    pEnemyShotSet->pEnemyShotHead->prev = pSp;
                }
            }
        }
        // --- 状態1: 飛沫型小玉（重力による放物線落下） ---
        else if (pShot->param_i[0] == 1) {
            pShot->param_d[1] += 0.04; // 下方向への重力加速度を毎フレーム加算

            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];

            // メイン側の仕様（向きや速度の参照）に対応するため、標準変数も同期させておく
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
            pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0] + pShot->param_d[1] * pShot->param_d[1]);
        }
        // --- 状態2: 予兆レーザー（等速直線運動で直進） ---
        else if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNextShot;
    }
}

// 敵本体のパターン（関数指定通り変更）
void EnemyPat_Spout_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面（480x480）の上部にボスを初期配置
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200固定仕様
        muki = 1;
    }
    else {
        // ボス本体は上空で緩やかに左右に揺れ動く（無防備なボスを狙う楽しさを作るため）
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 240フレーム（4秒）サイクルで大噴流弾幕セットを1つ配置
    if (count % 240 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAquaPressure;

        // クジラの潮吹きモチーフに基づき、
        // 弾幕の発生起点（噴射口）を画面最下部中央に固定して真上に噴き上げさせる
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 460.0;
        pEnemyShotSet->muki = -DX_PI / 2.0; // 真上向き
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストへ挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}