// enemyPat_tmp.cpp
// 音ゲー風リズム弾幕パターン (EnemyPat_RhythmGame_Sakana) - 上級版

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：音ゲー風リズム弾幕（4レーン＋多様な挙動）
static void ShotRhythmAdvanced(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 発射時に効果音を鳴らす
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // レーン番号（0:右, 1:左, 2:下, 3:上）
        int lane = pEnemyShotSet->kind;

        // レーンごとに色と弾種を固定
        int color = lane; // 0:赤, 1:黄, 2:緑, 3:シアン

        // レーンごとに弾の挙動を変える
        switch (lane) {
        case 0: // 右レーン：スライダー風（曲がる弾）
        {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0; // 右向き
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotBullet[color];
            pEnemyShot->param_i[0] = lane;
            pEnemyShot->param_d[0] = 0.0; // 経過時間
            pEnemyShot->param_i[1] = 0;   // スライダー弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        break;

        case 1: // 左レーン：ホールド風（連続発射）
        {
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = DX_PI; // 左向き
                pEnemyShot->speed = 1.8;
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                pEnemyShot->param_i[0] = lane;
                pEnemyShot->param_i[1] = i; // 何発目か

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        break;

        case 2: // 下レーン：連打風（散らばる弾）
        {
            for (int i = 0; i < 6; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 下方向を中心に少し散らす
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.5 + GetRand(10) / 10.0;
                pEnemyShot->kind = img_enemyShotScale[color];
                pEnemyShot->param_i[0] = lane;
                pEnemyShot->param_i[1] = i;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        break;

        case 3: // 上レーン：追尾風（プレイヤーを狙う弾）
        {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = img_enemyShotDiamond[color];
            pEnemyShot->param_i[0] = lane;
            pEnemyShot->param_i[1] = 0; // 追尾弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        break;
        }
    }

    // 弾の移動処理＋特殊挙動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int lane = pEnemyShot->param_i[0];
        int type = pEnemyShot->param_i[1];

        switch (lane) {
        case 0: // 右レーン：スライダー風（曲がる）
            if (type == 0) {
                pEnemyShot->param_d[0] += 1.0;
                double t = pEnemyShot->param_d[0] * 0.04;
                double curve = sin(t) * 0.4; // 上下に揺れる
                pEnemyShot->muki = curve;
            }
            break;

        case 1: // 左レーン：ホールド風（時間差で加速）
            if (type > 0) {
                pEnemyShot->speed += 0.02;
            }
            break;

        case 2: // 下レーン：連打風（画面端で反射）
            if (pEnemyShot->x < 0 || pEnemyShot->x > 480) {
                pEnemyShot->muki = DX_PI - pEnemyShot->muki; // 左右反射
            }
            if (pEnemyShot->y < 0 || pEnemyShot->y > 480) {
                pEnemyShot->muki = -pEnemyShot->muki; // 上下反射
            }
            break;

        case 3: // 上レーン：追尾風（一定時間ごとに再照準）
            if (pEnemyShot->count % 30 == 0) {
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            }
            break;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（音ゲー風リズム弾幕 - 上級版）
void EnemyPat_RhythmGame_Sakana()
{
    static int muki;
    static int phase = 0; // フェーズ（レーン切り替え用）

    if (count == 1) {
        // 初期位置（画面中央上）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // リズムに合わせてレーンを切り替えながら発射
    if (count % 15 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRhythmAdvanced;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // ここでは使わない（弾幕内で個別に設定）

        // レーン番号を param_i[0] に保存
        pEnemyShotSet->kind = phase % 4; // 0〜3をループ
        phase++;

        // 弾リストのダミーヘッド
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}