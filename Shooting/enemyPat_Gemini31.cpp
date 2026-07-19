// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ブーメラン・ウェーブ（時差式引き波弾幕）
static void ShotBoomerangWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ----------------------------------------------------
    // 【フェーズ1：往路】 0〜90フレームの間、10フレーム間隔でバラ撒き
    // ----------------------------------------------------
    if (pEnemyShotSet->count >= 0 && pEnemyShotSet->count <= 90 && pEnemyShotSet->count % 10 == 0) {

        // 軽い発射音を鳴らす
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 30; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 真下（DX_PI / 2.0）を中心に、120度（±60度）の扇状に発射
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;

            // 速度にバラつきを持たせることで、画面外に到達するタイミングをずらす
            pEnemyShot->speed = (150 + GetRand(150)) / 100.0; // 1.5 〜 3.0

            // 最初は無害に見える「白の小玉」
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 6:白

            // ★重要：画面外で待機させるため、消去マージンを大きく設定する
            pEnemyShot->margin = 300.0;

            // 状態管理フラグ (0:往路, 1:待機, 2:復路)
            pEnemyShot->param_i[0] = 0;

            // リストへの追加処理
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----------------------------------------------------
    // 【フェーズ2：復路トリガー】 200フレーム目で一斉に逆走開始
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 200) {
        // 迫力のある重い音で危険を知らせる
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            // 待機中（画面下に溜まっている）の弾だけを対象にする
            if (pEnemyShot->param_i[0] == 1) {
                pEnemyShot->param_i[0] = 2;       // 復路へ移行
                pEnemyShot->muki = -DX_PI / 2.0;  // 真上に向ける
                pEnemyShot->speed = 5.5;          // 往路より少し速めの均一な速度の「壁」にする

                // ★視覚的な殺意：赤い鱗弾に変化させる
                pEnemyShot->kind = img_enemyShotScale[0]; // 0:赤
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // ----------------------------------------------------
    // 【毎フレームの移動と状態更新処理】
    // ----------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 往路の弾が画面下端を越えたら、一時停止させる
        if (pEnemyShot->param_i[0] == 0 && pEnemyShotSet->count < 200) {
            // 画面サイズ480より少し下の 500.0 の位置を待機ラインとする
            if (pEnemyShot->y > 470.0) {
                pEnemyShot->y = 470.0;      // ラインを揃える
                pEnemyShot->speed = 0.0;    // 停止
                pEnemyShot->param_i[0] = 1; // 待機状態へ
            }
        }

        // 座標更新 (停止中の弾は speed が 0 なので動かない)
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_Reverse_Gemini()
{
    // 出現時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 待機中は8の字を描くように、ふわふわと移動させる
        enemy.x = 240.0 + 80.0 * sin(count / 60.0);
        enemy.y = 100.0 + 20.0 * sin(count / 30.0);
    }

    // 300フレーム周期で弾幕発動
    if (count % 300 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBoomerangWave; // 作成した弾幕関数をセット
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = 0; // 今回は固定

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // Setのリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}