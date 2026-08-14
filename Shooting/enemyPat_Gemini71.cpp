// enemyPat_EndFilament.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------------
// 弾幕パターン①：スパーク弾（接触不良による不規則なばら撒き）
// --------------------------------------------------------
static void ShotSpark(sEnemyShotSet* pEnemyShotSet)
{
    // 生成時のみ弾をセットして終わり
    if (pEnemyShotSet->count == 0) {
        // ジリジリとしたノイズ音をイメージ（軽めの発射音）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 2〜4発のスパーク弾
        int shotNum = 2 + GetRand(2);
        for (int i = 0; i < shotNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボス中心から少しズレた位置から発生
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;

            // 下方向(DX_PI/2)をベースにランダムに散らす
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(180) - 90) / 180.0 * DX_PI;

            // 速度はバラバラ
            pEnemyShot->speed = (200 + GetRand(400)) / 100.0;

            // 短レーザー(7)。色は 黄(1) または 橙(8)
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotLaser[1] : img_enemyShotLaser[8];

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

// --------------------------------------------------------
// 弾幕パターン②：ブラックアウトとフラッシュ
// --------------------------------------------------------
static void ShotShadowToFlash(sEnemyShotSet* pEnemyShotSet)
{
    // === [ブラックアウトフェーズ] 影弾の設置 ===
    // カウント 0, 20, 40 のタイミングで黒い弾を円形に設置
    if (pEnemyShotSet->count % 20 == 0 && pEnemyShotSet->count <= 40) {

        if (pEnemyShotSet->count == 0) {
            // 最初の一回だけ「ブツン」という消灯から充電への予告音
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 残りHPが少ないほど設置する弾数を増やす(16way 〜 28way)
        double hpRatio = (double)enemy.hp / enemy.maxHp;
        int way = 16 + (int)(12 * (1.0 - hpRatio));

        // 設置する円の半径（少しずつ外側に設置する）
        double radius = 40.0 + pEnemyShotSet->count * 1.5;
        double base_angle = (GetRand(360) / 180.0) * DX_PI;

        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = base_angle + (DX_PI * 2.0 / way) * i;

            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);

            // フラッシュ時の放射方向として param_d[0] に角度を保存しておく
            pEnemyShot->param_d[0] = angle;

            pEnemyShot->speed = 0.0; // 設置時は停止
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒(7)の中玉（影弾）

            // 何波目の設置かを param_i[0] に記録し、フラッシュ時の変化に利用
            pEnemyShot->param_i[0] = pEnemyShotSet->count / 20;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // === [フラッシュフェーズ] 一斉点灯・発射 ===
    // カウント80で影弾を一斉に黄・白の弾へ変化させる
    if (pEnemyShotSet->count == 80) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 自機への角度を取得
        double aim_angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int waveType = pShot->param_i[0]; // 0, 1, 2

            if (waveType % 2 == 0) {
                // 0波目と2波目は「黄(1)の大玉」になり、元の放射方向へ飛ぶ
                pShot->kind = img_enemyShotLargeBall[1];
                pShot->speed = 2.0 + GetRand(100) / 100.0;
                pShot->muki = pShot->param_d[0];
            }
            else {
                // 1波目は「白(6)の中玉」になり、自機方向周辺に殺到する
                pShot->kind = img_enemyShotMediumBall[6];
                pShot->speed = 3.5 + GetRand(100) / 100.0;
                // 自機狙いベースに少しバラけさせる
                pShot->muki = aim_angle + (GetRand(60) - 30) / 180.0 * DX_PI;
            }
            pShot = pShot->next;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 発射後、徐々に加速させて「一気に拡散する」印象を強める
        if (pEnemyShotSet->count > 80 && pShot->speed < 7.0) {
            pShot->speed += 0.05;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// --------------------------------------------------------
// 敵本体のパターン関数（メイン呼び出し用）
// --------------------------------------------------------
void EnemyPat_FlickeringLight_Gemini()
{
    // ループ周期（240フレームで1サイクル）
    const int CYCLE = 240;
    int phase_time = count % CYCLE;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    double hpRatio = (double)enemy.hp / enemy.maxHp;

    // --------------------------------------------------------
    // フェーズ1: スパークフェーズ (0 ～ 89)
    // --------------------------------------------------------
    if (phase_time < 90) {
        // 接触不良を表現するため、小刻みに揺らす（ジリジリ感）
        enemy.x += (GetRand(20) - 10) / 10.0;
        enemy.y += (GetRand(20) - 10) / 10.0;

        // 中心(240, 80)に戻ろうとする力をかけることで画面外へ逃げないようにする
        enemy.x += (240.0 - enemy.x) * 0.1;
        enemy.y += (80.0 - enemy.y) * 0.1;

        // HPが減るほど高確率でスパーク弾を散らす（緊迫感の演出）
        int sparkProb = 30 + (int)(60 * (1.0 - hpRatio)); // 30% ～ 90%
        if (phase_time % 5 == 0 && GetRand(100) < sparkProb) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSpark;
            pSet->x = enemy.x;
            pSet->y = enemy.y;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // --------------------------------------------------------
    // フェーズ2: ブラックアウト・影弾設置開始 (90)
    // --------------------------------------------------------
    else if (phase_time == 90) {
        // 揺れを止めて定位置に戻す
        enemy.x = 240.0;
        enemy.y = 80.0;

        // 影弾制御用の ShotSet を1つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShadowToFlash;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // --------------------------------------------------------
    // フェーズ3: 消灯中 〜 クールダウン (91 ～ 239)
    // --------------------------------------------------------
    else {
        // 消灯中（91〜170）は完全に静止させて「沈黙」を演出。
        // フラッシュ後（170以降）は次回スパークへの予兆として僅かにフワフワ上下させる。
        if (phase_time > 170) {
            enemy.y += sin(count * 0.05) * 0.3;
        }
    }
}