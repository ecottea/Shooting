// enemyPat_blizzard.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：吹雪（疑似物理演算による風と雪の表現）
static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    // 240フレーム（約4秒間）、継続的に雪（弾）を生成し続ける
    if (pEnemyShotSet->count < 240) {

        // 弾幕が濃いため、発射音は間引いて再生（うるさすぎないよう調整）
        if (pEnemyShotSet->count % 4 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 1フレームあたり 3〜5 発の雪を生成
        int spawnNum = 3; // +GetRand(2); // 3, 4, 5
        for (int i = 0; i < spawnNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 敵本体の座標を中心に、少し幅を持たせて発生させる
            pEnemyShot->x = enemy.x + (GetRand(80) - 40);
            pEnemyShot->y = enemy.y + (GetRand(20) - 10);

            // 初期の向き：下方向を中心に 120 度の扇状にばらまく
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
            // 初期の速度：ゆっくり（後で風に乗って加速する）
            pEnemyShot->speed = (50 + GetRand(100)) / 100.0;
            pEnemyShot->count = 0;

            // 色の決定（3:シアン、4:青、6:白）
            int colorPicker = GetRand(2); // 0, 1, 2
            int color = (colorPicker == 0) ? 3 : (colorPicker == 1) ? 4 : 6;

            // 形の決定（0:小玉、4:鱗弾、5:菱形弾）
            int typePicker = GetRand(2);
            if (typePicker == 0)      pEnemyShot->kind = img_enemyShotSmallBall[color];
            else if (typePicker == 1) pEnemyShot->kind = img_enemyShotScale[color];
            else                      pEnemyShot->kind = img_enemyShotDiamond[color];

            // リストへ追加（ダミーヘッドの直前＝末尾）
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 風のシミュレーション ---
    // セット自体の生存時間を使って、時間経過で左右に揺れる風（突風）を計算
    double windX = sin(pEnemyShotSet->count * 2.0 * DX_PI / 180.0) * 0.12;
    double windY = 0.03; // 常に下へ引っ張る弱い力

    // すでに発射されている全弾の座標と速度を更新
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 現在の進行ベクトルに、風の力をベクトル加算
        double vx = pEnemyShot->speed * cos(pEnemyShot->muki) + windX;
        double vy = pEnemyShot->speed * sin(pEnemyShot->muki) + windY;

        // 加算後のベクトルから、新しい速度と向きを再計算
        pEnemyShot->speed = sqrt(vx * vx + vy * vy);
        pEnemyShot->muki = atan2(vy, vx);

        // 雪が風に煽られて舞うように、10分の1の確率で向きをわずかに乱す
        if (GetRand(9) == 0) {
            pEnemyShot->muki += (GetRand(20) - 10) / 180.0 * DX_PI * 0.7;
        }

        // 最終的な座標の更新
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        //pEnemyShot->count++;
        pEnemyShot = pEnemyShot->next;
    }

    //pEnemyShotSet->count++;
}


static void ShotBlizzard_sub(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(240) - 120) / 180.0 * DX_PI;
        pEnemyShot->speed = 1.0;
        pEnemyShot->kind = img_enemyShotLargeBall[6];
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Blizzard_Gemini()
{
    // 出現時の初期化処理
    if (count == 1) {
        // ゲーム画面の中央上部付近に配置
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 吹雪を画面全体に広げるため、サイン波を利用してゆっくりと「8の字」を描く軌道
        enemy.x = 240.0 + 120.0 * sin(count * 2.0 * DX_PI / 300.0);
        enemy.y = 80.0 + 30.0 * sin(count * 4.0 * DX_PI / 300.0);
    }

    // 300フレーム（約5秒）ごとに新しい「吹雪セット」を発生させる
    // ※セット内で240フレーム弾を出し続けるため、60フレームの静寂（風の切れ間）が生まれる
    if (count % 300 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // 弾用ダミーヘッドの構築
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストへの登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count % 3 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard_sub;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = -DX_PI / 2.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}