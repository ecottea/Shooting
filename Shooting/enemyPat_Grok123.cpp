// enemyPat_Tmp.cpp
// エアホッケー風弾幕「リバウンド・パック」
// 大きな弾（パック）が画面内を跳ね返り、左右端に当たるたびに扇状・放射状の弾を撒く

#include "gv.h"  // 必要に応じて実際のプロジェクトのインクルードに合わせて調整

// 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
// 弾の種類: 小玉(SmallBall), 中玉(MediumBall), 大玉(LargeBall), 銃弾(Bullet), 鱗弾(Scale), 菱形弾(Diamond), 中楕円弾(MediumOval), 短レーザー(Laser)
// 色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙

// エアホッケー風ショットパターン
static void ShotAirHockey(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count == 0 のときだけパックを生成（メインルーチンで count は自動インクリメント）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // パック（大玉・白）を生成
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 240.0;
        pEnemyShot->y = 120.0;
        pEnemyShot->muki = 0.0;          // 角度は使わず、param_d で速度を管理
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[6];  // 白の大玉
        pEnemyShot->param_i[0] = 1;      // 1 = パック判定用フラグ
        pEnemyShot->param_d[0] = 3.8*2;    // vx （初期右向き）
        pEnemyShot->param_d[1] = 1.6*2;    // vy
        pEnemyShot->param_d[2] = 0.0;    // 予備（加速用などに使える）

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 全弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // ===== パックの処理 =====
            // 移動
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];

            bool bouncedSide = false;

            // 左右壁で反射（エアホッケーのパドルに当たったイメージ）
            if (pShot->x < 24.0) {
                pShot->x = 24.0;
                pShot->param_d[0] = fabs(pShot->param_d[0]);  // 必ず右向きに
                bouncedSide = true;
            }
            else if (pShot->x > 456.0) {
                pShot->x = 456.0;
                pShot->param_d[0] = -fabs(pShot->param_d[0]); // 必ず左向きに
                bouncedSide = true;
            }

            // 上下壁でも反射（テーブル端）
            if (pShot->y < 24.0) {
                pShot->y = 24.0;
                pShot->param_d[1] = fabs(pShot->param_d[1]);
            }
            else if (pShot->y > 456.0) {
                pShot->y = 456.0;
                pShot->param_d[1] = -fabs(pShot->param_d[1]);
            }

            // 左右に当たったときだけ弾を撒く
            if (bouncedSide) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 左右に当たるたびに少し加速（上限あり）
                if (fabs(pShot->param_d[0]) < 6.5) {
                    pShot->param_d[0] *= 1.035;
                }
                // 上下速度もわずかに変化させて軌道を崩す
                pShot->param_d[1] += (GetRand(40) - 20) / 100.0 * 0.15;
                if (fabs(pShot->param_d[1]) > 3.5) {
                    pShot->param_d[1] *= 0.85;
                }

                // 1. プレイヤー方向への扇状弾（中玉）
                double angleToPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
                for (int i = 0; i < 7; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = angleToPlayer + (i - 3) * 0.18;
                    pNew->speed = 2.4 + GetRand(80) / 100.0;
                    // 色はランダム（0〜8）
                    pNew->kind = img_enemyShotMediumBall[GetRand(8)];
                    pNew->param_i[0] = 0;

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }

                // 2. 放射状の小玉（パックが弾けたイメージ）
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = (DX_PI * 2.0 * i) / 8.0 + GetRand(40) / 100.0 * 0.3;
                    pNew->speed = 1.6 + GetRand(60) / 100.0;
                    pNew->kind = img_enemyShotSmallBall[3];  // シアンの小玉
                    pNew->param_i[0] = 0;

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }

                // 3. 少し遅い菱形弾を追加で散らす（変化を付ける）
                for (int i = 0; i < 4; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = angleToPlayer + (GetRand(120) - 60) / 180.0 * DX_PI;
                    pNew->speed = 1.2 + GetRand(40) / 100.0;
                    pNew->kind = img_enemyShotDiamond[GetRand(5)];  // 暖色寄り
                    pNew->param_i[0] = 0;

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
            }
        }
        else {
            // ===== 通常弾の処理 =====
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン（関数名は指定通り EnemyPat_AirHockey_Grok）
void EnemyPat_AirHockey_Grok()
{
    // 初期化（count はメインルーチンで自動インクリメント）
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;  // 200で固定
    }

    // 敵はほぼ動かさず中央上に固定（パックが主役のため）
    // 少しだけ左右に揺れる程度
    if (count > 1) {
        enemy.x = 240.0 + sin(count * 0.03) * 30.0;
    }

    // ショットセットを一度だけ生成（長時間生き残るパック用）
    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAirHockey;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}