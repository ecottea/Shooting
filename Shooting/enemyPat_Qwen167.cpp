#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>

// ============================================================
//  弾幕パターン：顔弾幕「へのへのもへじの嘲笑」
// ============================================================
static void ShotPattern_Hehenomoheji(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ1: 顔の描画（count == 0 のみ実行）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y + 60.0; // 顔の中心をボスの少し下に設定

        // 1. じ (輪郭): 青い中玉を楕円リング状に配置
        int num_ji = 32;
        for (int i = 0; i < num_ji; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            double theta = (2.0 * DX_PI / num_ji) * i;
            shot->param_d[0] = cx + 100.0 * cos(theta); // 目標X
            shot->param_d[1] = cy + 120.0 * sin(theta); // 目標Y
            shot->x = cx;
            shot->y = cy;
            shot->muki = atan2(shot->param_d[1] - shot->y, shot->param_d[0] - shot->x);
            shot->speed = 5.0;
            shot->kind = img_enemyShotMediumBall[4]; // 4:青
            shot->param_i[0] = 0; // パーツID: 0=じ
            shot->param_d[2] = theta;      // 初期角度 (回転用)
            shot->param_d[3] = 100.0;      // 初期X半径
            shot->param_d[4] = 120.0;      // 初期Y半径

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }

        // 2. へ (眉左): 黄色い鱗弾を逆V字に配置
        for (int i = 0; i < 5; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->param_d[0] = cx - 40.0 - 8.0 * i;
            shot->param_d[1] = cy - 60.0 - 8.0 * i;
            shot->x = cx;
            shot->y = cy;
            shot->muki = atan2(shot->param_d[1] - shot->y, shot->param_d[0] - shot->x);
            shot->speed = 5.0;
            shot->kind = img_enemyShotScale[1]; // 1:黄
            shot->param_i[0] = 1; // パーツID: 1=眉左

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }

        // 3. へ (眉右): 黄色い鱗弾を逆V字に配置
        for (int i = 0; i < 5; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->param_d[0] = cx + 40.0 + 8.0 * i;
            shot->param_d[1] = cy - 60.0 - 8.0 * i;
            shot->x = cx;
            shot->y = cy;
            shot->muki = atan2(shot->param_d[1] - shot->y, shot->param_d[0] - shot->x);
            shot->speed = 5.0;
            shot->kind = img_enemyShotScale[1]; // 1:黄
            shot->param_i[0] = 2; // パーツID: 2=眉右
            shot->margin = 10;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }

        // 4. の (左目): マゼンタ大玉
        sEnemyShot* shotL = new sEnemyShot;
        shotL->param_d[0] = cx - 50.0;
        shotL->param_d[1] = cy - 20.0;
        shotL->x = cx;
        shotL->y = cy;
        shotL->muki = atan2(shotL->param_d[1] - shotL->y, shotL->param_d[0] - shotL->x);
        shotL->speed = 5.0;
        shotL->kind = img_enemyShotLargeBall[5]; // 5:マゼンタ
        shotL->param_i[0] = 3; // パーツID: 3=の左

        shotL->prev = pEnemyShotSet->pEnemyShotHead->prev;
        shotL->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = shotL;
        pEnemyShotSet->pEnemyShotHead->prev = shotL;

        // 5. の (右目): マゼンタ大玉
        sEnemyShot* shotR = new sEnemyShot;
        shotR->param_d[0] = cx + 50.0;
        shotR->param_d[1] = cy - 20.0;
        shotR->x = cx;
        shotR->y = cy;
        shotR->muki = atan2(shotR->param_d[1] - shotR->y, shotR->param_d[0] - shotR->x);
        shotR->speed = 5.0;
        shotR->kind = img_enemyShotLargeBall[5]; // 5:マゼンタ
        shotR->param_i[0] = 4; // パーツID: 4=の右

        shotR->prev = pEnemyShotSet->pEnemyShotHead->prev;
        shotR->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = shotR;
        pEnemyShotSet->pEnemyShotHead->prev = shotR;

        // 6. も (鼻): 赤い鱗弾を「3」の字に配置
        double nose_x[3] = { 0, 15, 0 };
        double nose_y[3] = { 20, 45, 70 };
        for (int i = 0; i < 3; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->param_d[0] = cx + nose_x[i];
            shot->param_d[1] = cy + nose_y[i];
            shot->x = cx;
            shot->y = cy;
            shot->muki = atan2(shot->param_d[1] - shot->y, shot->param_d[0] - shot->x);
            shot->speed = 5.0;
            shot->kind = img_enemyShotScale[0]; // 0:赤
            shot->param_i[0] = 5; // パーツID: 5=も

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }

        // 7. へ (口): 黄色い鱗弾を微笑む弧状に配置
        for (int i = 0; i < 9; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            double t = -1.0 + (2.0 / 8.0) * i; // -1.0 から 1.0
            shot->param_d[0] = cx + 60.0 * t;
            shot->param_d[1] = cy + 50.0 + 30.0 * (1.0 - t * t); // 放物線状の弧
            shot->x = cx;
            shot->y = cy;
            shot->muki = atan2(shot->param_d[1] - shot->y, shot->param_d[0] - shot->x);
            shot->speed = 5.0;
            shot->kind = img_enemyShotScale[1]; // 1:黄
            shot->param_i[0] = 6; // パーツID: 6=口
            shot->param_i[3] = i; // 生成時のインデックスを保存 (ウェーブの位相用)

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // フェーズ2: 各弾の移動と攻撃処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 目標位置への移動処理 (param_i[1] == 0 の間)
        if (pShot->param_i[1] == 0) {
            double dx = pShot->param_d[0] - pShot->x;
            double dy = pShot->param_d[1] - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < pShot->speed) {
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->speed = 0.0;
                pShot->param_i[1] = 1; // 到達済みフラグ
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        // 攻撃フェーズ (pEnemyShotSet->count > 40 かつ 到達済み)
        if (pEnemyShotSet->count > 40 && pShot->param_i[1] == 1) {
            switch (pShot->param_i[0]) {
            case 0: // じ: 回転しながら収縮する壁
            {
                double r_scale = 1.0 - (pEnemyShotSet->count - 40) * 0.008;
                if (r_scale < 0.1) r_scale = 0.1; // 最小サイズ制限
                double theta = pShot->param_d[2] + (pEnemyShotSet->count - 40) * 0.03;
                pShot->x = pEnemyShotSet->x + pShot->param_d[3] * r_scale * cos(theta);
                pShot->y = (pEnemyShotSet->y + 60.0) + pShot->param_d[4] * r_scale * sin(theta);
                break;
            }
            case 1: // 眉左: プレイヤー狙いの連射
            case 2: // 眉右: プレイヤー狙いの連射
            {
                // GetRand(40) は 0～40 の41種類を返す。-20 して -20～20 の範囲にし、角度のばらつきとする
                if (pShot->count % 80 == 0) {
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(40) - 20) / 20.0 * (DX_PI / 6.0);
                    pShot->speed = 3.5;
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                break;
            }
            case 3: // の左: 螺旋状に動きながら子弾を放射
            case 4: // の右: 螺旋状に動きながら子弾を放射
            {
                double dir = (pShot->param_i[0] == 3) ? 1.0 : -1.0; // 左目はCW, 右目はCCW
                double theta = (pShot->count * 0.15) * dir;
                double r = 10.0 + (pShot->count - 40) * 0.8;
                if (r > 80.0) r = 80.0; // 半径の上限
                pShot->x = pShot->param_d[0] + r * cos(theta);
                pShot->y = pShot->param_d[1] + r * sin(theta);

                // 螺旋軌跡に沿って小さな白弾を生成
                if (pShot->count % 8 == 0) {
                    sEnemyShot* subShot = new sEnemyShot;
                    subShot->x = pShot->x;
                    subShot->y = pShot->y;
                    subShot->muki = theta + (dir * DX_PI / 2.0); // 接線方向へ発射
                    subShot->speed = 2.5;
                    subShot->kind = img_enemyShotSmallBall[6]; // 6:白
                    subShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    subShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = subShot;
                    pEnemyShotSet->pEnemyShotHead->prev = subShot;
                }
                break;
            }
            case 5: // も: プレイヤー狙いの重弾（くしゃみ）
            {
                if (pShot->count % 120 == 0) {
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 5.0;
                    pShot->kind = img_enemyShotLargeBall[0]; // 0:赤 (大玉に変化して重弾感を演出)
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                break;
            }
            case 6: // 口: 画面下に向かって横一文字に広がるウェーブ状の弾
            {
                // 保存しておいたインデックス(param_i[3])を使って、パーツごとに位相をずらす
                double wave = sin(pShot->count * 0.1 + pShot->param_i[3] * 0.6) * 30.0;
                pShot->x = pShot->param_d[0] + wave;
                pShot->y += 2.5;
                break;
            }
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン（へのへのもへじ専用）
// ============================================================
void EnemyPat_Henohenomoheji_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // HPを少し上げてこのパターンの持続を確保
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1; // 左右移動
    }

    // 300フレーム(約5秒)ごとに顔弾幕を展開
    if (count % 300 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPattern_Hehenomoheji;
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