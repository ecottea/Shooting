// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：吹雪 (Blizzard)
// 自機を狙って放たれた無数の弾が、冷たい吹雪のようにユラユラと
// 舞い散りながら押し寄せてくる。
// ============================================================
static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 生成時の初期化 (count == 0)
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num_shots = 40; // 吹雪らしく大量にばら撒く
        for (int i = 0; i < num_shots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 自機狙い (pEnemyShotSet->muki) を中心に、左右に広くばら撒く
            double spread = (GetRand(120) - 60) / 180.0 * DX_PI; // -60度 ～ +60度
            pEnemyShot->muki = pEnemyShotSet->muki + spread;

            // 初速度は遅め（ふわっと空中に浮かぶイメージ）
            pEnemyShot->speed = 1.5 + GetRand(15) / 10.0;

            // 雪の結晶や氷の粒をイメージして、白とシアンの小玉・菱形弾をランダム生成
            int rand_val = GetRand(2);
            if (rand_val == 0) {
                pEnemyShot->kind = img_enemyShotDiamond[6];   // 白の菱形弾 (雪の結晶)
            }
            else if (rand_val == 1) {
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白の小玉 (氷の粒)
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉 (冷たい氷)
            }

            // リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 吹雪の風向：時間とともにサインカーブに変化（横風）
        double wind_angle = sin(pEnemyShotSet->count * 0.02) * (DX_PI / 4.0); // ±45度

        // 弾 individual のユラユラ感（舞い散るノイズ）
        pEnemyShot->muki += sin(pEnemyShot->count * 0.2 + pEnemyShot->muki * 3.0) * 0.05;

        // 風の方向へ角度を補間（徐々に風下へ流されていく）
        pEnemyShot->muki += wind_angle * 0.01;

        // 風に乗って徐々に加速
        pEnemyShot->speed += 0.03;
        if (pEnemyShot->speed > 4.5) pEnemyShot->speed = 4.5;

        // 座標の更新
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Blizzard_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右に移動
        enemy.x += 1.2 * (double)muki;
        if (enemy.x > 440.0 || enemy.x < 40.0) muki *= -1;
    }

    // 一定間隔で吹雪弾幕を生成
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 自機を狙う角度を計算してセットする
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + 0.5;

        // 弾管理用リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ShotSetリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}