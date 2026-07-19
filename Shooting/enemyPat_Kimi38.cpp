// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 角度差を -PI 〜 PI に正規化
static double NormalizeAngle(double angle)
{
    while (angle > DX_PI)  angle -= 2.0 * DX_PI;
    while (angle < -DX_PI) angle += 2.0 * DX_PI;
    return angle;
}

// 弾幕：チャージング・レーザーファンネル
//  予告中は敵機中心に短いシアンレーザーを密着配置（プレイヤーには絶対に当たらない）。
//  1.5秒後にその角度で赤いレーザー列に一気に変化し、2秒間維持。
//  3波重ねがけで最大75本のレーザーセグメントが交差する。
static void ShotLaserFunnel(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count == 0: 予告線（敵機中心の短いシアンレーザー）を5本生成
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;

            // プレイヤー方向を中心に扇状（左右18度ずつ）に5方向
            double spread = (i - 2) * (DX_PI / 10.0);
            double baseAngle = pEnemyShotSet->muki + spread;

            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 0.0;

            // 予告線: シアン(3) ※敵機に密着し、当たり判定はない扱い
            pEnemyShot->kind = img_enemyShotLaser[3];

            // param_i[0]: 状態 (0=予告線, 1=実弾)
            // param_i[1]: 予告残りフレーム（45 ≒ 1.5秒）
            // param_i[2]: 実弾残りフレーム（60 ≒ 2秒）
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = 45;

            // param_d[0]: 発射時に固定する角度
            // param_d[1]: 敵機中心からのオフセット距離
            pEnemyShot->param_d[0] = baseAngle;
            pEnemyShot->param_d[1] = 0.0;

            // 敵機中心に配置（短いレーザーが敵機に密着）
            pEnemyShot->x = enemy.x;
            pEnemyShot->y = enemy.y;

            // リスト追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next;

        if (pEnemyShot->param_i[0] == 0) {
            // ===== 予告線: プレイヤー方向に遅延追従 =====
            // 敵機中心からプレイヤー方向を計算
            double targetAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            double diff = NormalizeAngle(targetAngle - pEnemyShot->muki);

            // 18フレーム（0.3秒）で追従完了する補間
            pEnemyShot->muki += diff / 18.0;
            pEnemyShot->param_d[0] = pEnemyShot->muki;

            // 予告線は敵機中心に密着（プレイヤーには絶対に当たらない）
            pEnemyShot->x = enemy.x;
            pEnemyShot->y = enemy.y;

            pEnemyShot->param_i[1]--;
            if (pEnemyShot->param_i[1] <= 0) {
                // ===== 発射！ 予告線を実弾の先頭に変換 =====
                if (!CheckSoundMem(sound_enemyShot_extreme))
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                pEnemyShot->param_i[0] = 1;
                pEnemyShot->param_i[2] = 60;
                pEnemyShot->kind = img_enemyShotLaser[0]; // 赤
                pEnemyShot->param_d[0] = pEnemyShot->muki; // 角度固定
                pEnemyShot->param_d[1] = 0.0; // 敵機中心

                // 残り14セグメントを追加生成（32px〜448px）
                for (int j = 1; j < 15; j++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->muki = pEnemyShot->muki;
                    pNew->speed = 0.0;
                    pNew->kind = img_enemyShotLaser[0]; // 赤
                    pNew->param_i[0] = 1;
                    pNew->param_i[2] = 60;
                    pNew->param_d[0] = pEnemyShot->muki;
                    pNew->param_d[1] = j * 32.0;

                    // 位置は敵機中心＋オフセット
                    pNew->x = enemy.x + pNew->param_d[1] * cos(pNew->muki);
                    pNew->y = enemy.y + pNew->param_d[1] * sin(pNew->muki);

                    // リストに追加（pEnemyShotの後ろに挿入）
                    pNew->prev = pEnemyShot;
                    pNew->next = pEnemyShot->next;
                    pEnemyShot->next->prev = pNew;
                    pEnemyShot->next = pNew;

                    pEnemyShot = pNew; // 現在位置を更新
                }
            }
        }
        else if (pEnemyShot->param_i[0] == 1) {
            // ===== 実弾: 角度固定、敵機中心＋オフセットで位置更新 =====
            pEnemyShot->muki = pEnemyShot->param_d[0];
            pEnemyShot->x = enemy.x + pEnemyShot->param_d[1] * cos(pEnemyShot->muki);
            pEnemyShot->y = enemy.y + pEnemyShot->param_d[1] * sin(pEnemyShot->muki);

            pEnemyShot->param_i[2]--;

            // 消滅前10フレーム: 赤(0)と白(6)を点滅させて警告
            if (pEnemyShot->param_i[2] <= 10) {
                pEnemyShot->kind = (pEnemyShot->param_i[2] % 4 < 2)
                    ? img_enemyShotLaser[0]   // 赤
                    : img_enemyShotLaser[6];  // 白
            }

            // 時間経過で消滅
            if (pEnemyShot->param_i[2] <= 0) {
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;

                pEnemyShot = pNext;
                continue;
            }
        }

        pEnemyShot = pNext;
    }
}

static void ShotScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 9; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            pEnemyShot->speed = (200 + GetRand(200)) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0];

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


// 敵本体のパターン
void EnemyPat_Laser_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;          // 上部に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 60フレーム（2秒）間隔で3波までレーザーセットを生成
    // 第1波: count=0, 第2波: count=60, 第3波: count=120
    if (count % 60 == 1 && count % 240 < 180) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLaserFunnel;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count % 50 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotScatter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}