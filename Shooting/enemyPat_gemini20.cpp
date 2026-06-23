// enemyPat_hourglass.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：砂時計（時砂の回廊）
static void ShotHourglass(sEnemyShotSet* pEnemyShotSet)
{
    // 敵本体の座標に追従させる（ボスが動くと砂時計も揺らぐ）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    int c = pEnemyShotSet->count;
    int phase = c % 400; // 500フレーム周期 (0~249: 砂が落ちる, 250~499: 時間逆行)

    // 砂時計の形状パラメータ
    double glassWidth = 120.0;     // 枠の半幅
    double neckOffset = 150.0;     // くびれ（収束点）までのY軸距離

    double focalX = pEnemyShotSet->x;
    double focalY = pEnemyShotSet->y + neckOffset;

    // --------------------------------------------------------
    // 【フェーズ1】 時間が正方向に進み、砂が落ちる (0 ~ 249)
    // --------------------------------------------------------
    if (phase < 250) {

        // ① ガラス枠（シアン色のひし形弾）の生成: 4フレームに1回
        if (phase % 4 == 0) {
            // 軽い発射音を鳴らす
            //if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 2; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                // i==0なら左端、i==1なら右端
                double startX = pEnemyShotSet->x + (i == 0 ? -glassWidth : glassWidth);
                double startY = pEnemyShotSet->y;

                pShot->x = startX;
                pShot->y = startY;
                // 収束点（くびれ）へ向かう角度
                pShot->muki = atan2(focalY - startY, focalX - startX);
                pShot->speed = 2.0;
                pShot->kind = img_enemyShotDiamond[3]; // 3:シアン（ガラスの表現）

                // リストへの追加（末尾）
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // ② 砂（黄/白の小玉弾）の生成: 毎フレーム生成して密度の高い砂を表現
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // ガラス枠の内側からランダムな位置で発生 (-110 ~ +110)
            double offsetX = (GetRand(220) - 110.0);
            double startX = pEnemyShotSet->x + offsetX;
            double startY = pEnemyShotSet->y;

            pShot->x = startX;
            pShot->y = startY;

            double baseAngle = atan2(focalY - startY, focalX - startX);
            // 砂のサラサラ感を出すため、角度に微小なブレ（-0.02 ~ +0.02 rad）を加える
            pShot->muki = baseAngle + (GetRand(40) - 20) / 1000.0;
            // 落下速度にもばらつきを持たせる (1.5 ~ 2.5)
            pShot->speed = (150 + GetRand(100)) / 100.0;

            // 色は 1:黄 をベースに、時折 6:白（光る砂）を混ぜる
            int colors[] = { 1, 6 };
            pShot->kind = img_enemyShotSmallBall[colors[GetRand(1)]];

            // リストへの追加（末尾）
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // --------------------------------------------------------
    // 【フェーズ切り替え】 時間逆行の瞬間 (250)
    // --------------------------------------------------------
    if (phase == 250) {
        // 時が止まり逆行する合図として重い音を鳴らす
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 弾の移動と、時間逆行ギミックの適用処理
    // --------------------------------------------------------
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 逆行の瞬間に、画面内にある全弾のベクトルを180度反転させる
        if (phase == 250) {
            pEnemyShot->muki += DX_PI; // 角度を180度（πラジアン）回す

            // 砂の弾（黄・白）を、時間が戻っていることを示すマゼンタに変化させる
            if (pEnemyShot->kind == img_enemyShotSmallBall[1] || pEnemyShot->kind == img_enemyShotSmallBall[6]) {
                pEnemyShot->kind = img_enemyShotSmallBall[5]; // 5:マゼンタ
            }
        }

        // 実際の移動処理
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ペナルティ弾
static void ShotPenaltyMove(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 0.2;
            pEnemyShot->speed = 10.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（エントリポイント）
void EnemyPat_Hourglass_Gemini()
{
    // 初期設定
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面上部中央寄り
        enemy.maxHp = enemy.hp = 200;
    }

    // フローティング効果：ボスのゆっくりとした上下運動に合わせて、砂時計全体も呼吸するように揺らぐ
    enemy.y = 80.0 + sin(count * DX_PI / 180.0) * 10.0;

    // 弾幕マネージャ（ShotSet）の登録 (60フレーム目に1回のみ)
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHourglass;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0; // 追従型のため固定の向きは不使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (player.y < 320 && count % 3 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPenaltyMove;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}