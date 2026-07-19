#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：境界「波と粒の境界」
static void ShotWaveAndParticle(sEnemyShotSet* pEnemyShotSet)
{
    // 発生源を常に敵本体に追従させる（敵が動いても波の起点がズレないようにする）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    int t = pEnemyShotSet->count;

    // 弾をリストに追加するラムダ式（C++17: リスト連結のコード重複を避ける）
    auto addShot = [&](double muki, double speed, int kind) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pEnemyShotSet->x;
        p->y = pEnemyShotSet->y;
        p->muki = muki;
        p->speed = speed;
        p->kind = kind;

        // 双方向リストへの挿入処理
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    };

    // =========================================================
    // 【波】の境界：交差する正弦波（DNAの二重らせんのような軌跡）
    // =========================================================
    // 4フレームに1回発射し、密度の高い滑らかな線の連なり（波）を作る
    if (t % 4 == 0) {
        if (t % 12 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        double base_angle = DX_PI / 2.0; // 真下基準
        double wave_amp = DX_PI / 2.5;   // 振幅（どれくらい左右に大きく振れるか）
        double wave_freq = 0.04;         // 周波数（どれくらい速く揺れるか）

        // 逆位相の2つの波を計算
        double wave1 = wave_amp * sin(t * wave_freq);
        double wave2 = wave_amp * sin(t * wave_freq + DX_PI); // DX_PIズラして逆向きに

        // それぞれを3wayにして帯を太く見せる（威圧感アップ）
        for (int i = -1; i <= 1; i++) {
            double offset = i * (DX_PI / 15.0); // 12度ずつ角度をずらす

            // 波1（青色の鱗弾）
            addShot(base_angle + wave1 + offset, 2.8, img_enemyShotScale[4]);
            // 波2（マゼンタの鱗弾）
            addShot(base_angle + wave2 + offset, 2.8, img_enemyShotScale[5]);
        }
    }

    // =========================================================
    // 【粒】の境界：直進的で不規則なばら撒き（ランダムパーティクル）
    // =========================================================
    // 20フレームに1回、全方位にランダム速度で発射
    if (t % 20 == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 15; i++) {
            double rand_angle = (GetRand(360) / 180.0) * DX_PI;
            double rand_speed = (100 + GetRand(250)) / 100.0; // 速度1.0〜3.5
            // 粒を表現する白い菱形弾
            addShot(rand_angle, rand_speed, img_enemyShotDiamond[6]);
        }
    }

    // =========================================================
    // 既存の弾の移動処理
    // =========================================================
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShotSet->count++;
}


// 敵本体のパターン（メインループからこれを呼ぶようにする）
void EnemyPat_Namistubu_Gemini()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;  // 画面上部中央付近に陣取る
        enemy.maxHp = enemy.hp = 200; // ボス級のHP
        muki = 1;
    }
    else {
        // スペルカード展開中は、優雅にゆっくりと左右に揺れる
        enemy.x += 0.4 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // カウント60で発狂開始（弾幕セットを1回だけ生成して登録）
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWaveAndParticle; // 作成した波と粒の関数をセット
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // マネージャーのリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}