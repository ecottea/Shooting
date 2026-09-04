// enemyPat_Tmp.cpp
//
// 弾幕：超音速ソニックウェイブ（ドップラー効果モチーフ）
//
// 【概要】
//   ボスが画面上部を左右に往復しながら、リング状の弾を一定間隔で全方向に発射する。
//   ドップラー効果を以下の3要素で表現：
//     ・ボスの進行方向側の弾は、角度刻みが小さい（＝輪が詰まって濃くなる）
//     ・進行方向側の弾は速く、逆側は遅い
//     ・進行方向側は青、逆側は赤にグラデーション（ブルーシフト／レッドシフト）
//   結果として、画面上に「青い濃い壁」と「赤いまばらな帯」の縞模様が
//   ボスの移動に合わせてうねりながら迫ってくる。
//
// 【攻略ポイント】
//   赤い帯（ボスが遠ざかる側）は弾がまばらで遅いので安全地帯。
//   青い壁は高密度かつ高速だが、ボスの動きを見れば次にどちらへ来るか予測できる。
//
// 【備考】
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//     画面外の弾の消去はメインルーチン側で行われる仕様なので本ファイルでは記述しない。
//   ・SEのピッチ変化（本当のドップラー音）はハンドル共有のため不可能なので、
//     通常の輪＝light、周期的な太い「大波」＝heavy で擬似的に表現する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：超音速ソニックウェイブ
static void ShotSonicWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音：通常の輪は light、大波（太い輪）は heavy
        if (pEnemyShotSet->kind == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        double moveAng = pEnemyShotSet->param_d[0]; // ボスの進行方向（右:0 / 左:π）
        double offset = pEnemyShotSet->param_d[1]; // 輪の位相（発射ごとに回して縞をうねらせる）

        // ドップラーの強さ・輪の細かさ・基準速度
        // kind 0: 通常の輪（小玉・細かい） / kind 1: 大波（中玉・太く遅い）
        double k, baseStep, baseSpeed;
        if (pEnemyShotSet->kind == 0) {
            k = 0.60;                          // 刻みの詰まり具合（ドップラー強度）
            baseStep = 2.0 * DX_PI / 28.0;    // 平均28発/輪
            baseSpeed = 2.4;
        }
        else {
            k = 0.50;
            baseStep = 2.0 * DX_PI / 16.0;    // 平均16発/輪
            baseSpeed = 1.5;
        }

        // 輪を1周分、ドップラー補正付きの可変刻みでばら撒く
        double theta = offset;
        while (theta < offset + 2.0 * DX_PI) {
            double g = cos(theta - moveAng); // 進行方向との一致度（-1〜+1）

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = theta;

            // ブルーシフト側（g>0）は速く、レッドシフト側（g<0）は遅い
            pEnemyShot->speed = baseSpeed * (1.0 - 0.35 * g);

            // 色：進行方向側が青、逆側が赤へグラデーション
            // 0:赤 3:シアン 4:青 6:白 8:橙
            int color;
            if (g > 0.55) color = 4; // 青
            else if (g > 0.20) color = 3; // シアン
            else if (g > -0.20) color = 6; // 白
            else if (g > -0.55) color = 8; // 橙
            else                color = 0; // 赤

            if (pEnemyShotSet->kind == 0) pEnemyShot->kind = img_enemyShotSmallBall[color];
            else                          pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 進行方向側（g>0）は刻みが縮む＝弾が詰まる。逆側は刻みが広がる＝まばらになる。
            // （刻みの総和は常に2πなので、毎回の発射数はほぼ一定になる）
            theta += baseStep * (1.0 - k * g);
        }
    }

    // 弾の移動（等速直進のみ。countの加算と画面外消去はメインルーチン任せ）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Doppler_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
        // 弾幕開始の予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 左右往復（半周期150フレーム、振幅180px → xは60〜420の範囲を動く）
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 90) muki *= -1;
    }

    // 予告音から60フレーム猶予を置いて、5フレームごとに輪を発射
    if (count > 60 && count % 5 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSonicWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // 進行方向（右:0 / 左:π）を弾幕側へ渡す → ドップラーの基準方位
        pEnemyShotSet->param_d[0] = (muki > 0) ? 0.0 : DX_PI;
        // 輪の位相を発射ごとに回転させ、縞模様に「うねり」を出す
        pEnemyShotSet->param_d[1] = shot_count * 0.22;
        // 9回に1回は太く遅い「大波」の輪（リズムと緩急を作る）
        pEnemyShotSet->kind = (shot_count % 9 == 8) ? 1 : 0;
        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}