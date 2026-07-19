// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// へにょりレーザー（1本の線を構成するセグメント群の更新）
static void ShotWigglyLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // セグメントの生成
    // param_i[0]: 生成間隔フレーム
    // param_i[1]: 最大セグメント数
    int interval = pEnemyShotSet->param_i[0];
    int maxSeg = pEnemyShotSet->param_i[1];

    if (interval > 0 && maxSeg > 0) {
        int currentSegIndex = pEnemyShotSet->count / interval;
        if (pEnemyShotSet->count % interval == 0 && currentSegIndex <= maxSeg) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->speed = 3.5; // 基本の進行速度
            // muki は後のループで正確な移動ベクトルに上書きされるため、ここでは仮置き
            pEnemyShot->muki = pEnemyShotSet->muki;

            // 何番目のセグメントかを記録（波の位相計算に使う）
            pEnemyShot->param_i[0] = currentSegIndex;

            // 画像：銃弾(5.0x2.0)を採用。隙間なく繋がってレーザーのように見える。
            // pEnemyShotSet->kind に色のインデックス(0〜8)を入れている前提
            pEnemyShot->kind = img_enemyShotBullet[pEnemyShotSet->kind];

            // 画面外へ少しはみ出してから消えるようにマージンを拡張
            pEnemyShot->margin = 100.0;

            // リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全セグメントの座標更新（へにょり計算）
    double baseMuki = pEnemyShotSet->muki;
    double amp = pEnemyShotSet->param_d[0];      // 振幅
    double freq = pEnemyShotSet->param_d[1];     // 空間の波の周波数
    double waveSpeed = pEnemyShotSet->param_d[2]; // 時間の波の進行速度
    double originX = pEnemyShotSet->param_d[3];  // 発射元X
    double originY = pEnemyShotSet->param_d[4];  // 発射元Y

    // 進行方向に対する法線ベクトル（直角方向）
    double nx = cos(baseMuki + DX_PI / 2.0);
    double ny = sin(baseMuki + DX_PI / 2.0);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本軌道上の距離
        double dist = pShot->speed * pShot->count;

        // 基本軌道の座標（直進した場合の場所）
        double bx = originX + dist * cos(baseMuki);
        double by = originY + dist * sin(baseMuki);

        // サイン波によるオフセット量の計算
        // セグメント番号で空間的な波を形成し、countで時間経過とともに波が流れるようにする
        double phase = freq * pShot->param_i[0] - waveSpeed * pShot->count;
        double offset = amp * sin(phase);

        // 最終的な座標（基本軌道 ＋ 法線方向への振動）
        pShot->x = bx + offset * nx;
        pShot->y = by + offset * ny;

        // 描画・当たり判定用の向きを、実際の移動ベクトルから計算して設定
        if (pShot->count > 0) {
            double prev_dist = pShot->speed * (pShot->count - 1);
            double prev_bx = originX + prev_dist * cos(baseMuki);
            double prev_by = originY + prev_dist * sin(baseMuki);

            double prev_phase = freq * pShot->param_i[0] - waveSpeed * (pShot->count - 1);
            double prev_offset = amp * sin(prev_phase);

            double prev_x = prev_bx + prev_offset * nx;
            double prev_y = prev_by + prev_offset * ny;

            // 現在の座標と1フレーム前の座標から移動方向を算出
            pShot->muki = atan2(pShot->y - prev_y, pShot->x - prev_x);
        }

        pShot = pShot->next;
    }
}

// レーザー1本分のセットを生成するヘルパー関数
static void SpawnWigglyLaserSet(double x, double y, double muki, double amp, double freq, double waveSpeed, int interval, int maxSeg, int colorIdx)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotWigglyLaser;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = colorIdx; // 色情報を一時的にkindに保持

    // パラメータ格納
    pSet->param_d[0] = amp;
    pSet->param_d[1] = freq;
    pSet->param_d[2] = waveSpeed;
    pSet->param_d[3] = x; // 発射元X
    pSet->param_d[4] = y; // 発射元Y
    pSet->param_i[0] = interval;
    pSet->param_i[1] = maxSeg;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 敵本体のパターン
void EnemyPat_HenyoriLaser_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 敵の移動（左右に小さく揺れる）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 弾幕の発射タイミング
    // 120フレーム周期で、縦レーザーと横レーザーを交互に発射
    int cycle = count % 120;

    if (cycle == 1) {
        // 予告音を再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 縦方向（下向き）のへにょりレーザー x 3本
        // 色はシアン(3)
        // muki = PI/2 (下向き), 振幅40, 空間周波数0.25, 時間波速0.08
        for (int i = -1; i <= 1; i++) {
            SpawnWigglyLaserSet(
                enemy.x + i * 120.0, enemy.y + 10.0,
                DX_PI / 2.0,
                40.0, 0.25, 0.08,
                2, 60, 3
            );
        }
    }
    else if (cycle == 61) {
        // 予告音を再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 横方向（右向きと左向き）のへにょりレーザー x 2セット
        // 色はマゼンタ(5) ※縦と色を変えることで交差点が視認しやすくなる
        // 右向き (muki = 0)
        SpawnWigglyLaserSet(
            -10.0, enemy.y + 30.0,
            0.0,
            40.0, 0.25, 0.08,
            2, 80, 5
        );
        // 左向き (muki = PI)
        SpawnWigglyLaserSet(
            490.0, enemy.y + 30.0,
            DX_PI,
            40.0, 0.25, 0.08,
            2, 80, 5
        );
    }
}