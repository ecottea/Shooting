#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：飛鉢「伝説の飛空円盤」のリング弾幕パターン
static void ShotFlyingSaucer(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 生成時（count == 0）に円環状の弾群を一斉に配置
    if (pEnemyShotSet->count == 0) {
        // 発射時の効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int numBullets = 24;      // 円周上の弾数
        double baseRadius = 12.0; // 発生時の初期半径
        int colorIdx = pEnemyShotSet->kind % 9; // ボスのHPに応じた色インデックス

        // プレイヤーが抜けられる「隙間（穴）」の角度位置をランダムに決定
        // GetRand(x) は 0 から x までの x+1 種類の整数を返す
        int gapIndex = GetRand(numBullets - 1);
        int gapWidth = 3; // 隙間の広さ（弾の個数分）

        for (int i = 0; i < numBullets; i++) {
            // 隙間となる角度の弾は生成しない
            bool isGap = false;
            for (int g = 0; g < gapWidth; g++) {
                if ((gapIndex + g) % numBullets == i) {
                    isGap = true;
                    break;
                }
            }
            if (isGap) continue;

            pEnemyShot = new sEnemyShot;
            double theta = (2.0 * DX_PI / numBullets) * i;

            // 初期位置（ボスの現在地を基準に円形配置）
            pEnemyShot->x = pEnemyShotSet->x + baseRadius * cos(theta);
            pEnemyShot->y = pEnemyShotSet->y + baseRadius * sin(theta);

            // 進行方向とスピード（外側へ広がる）
            pEnemyShot->muki = theta;
            pEnemyShot->speed = 1.4 + (pEnemyShotSet->param_i[0] * 0.2);

            // 弾の種類：お札のイメージに合う菱形弾（img_enemyShotDiamond）を採用
            pEnemyShot->kind = img_enemyShotDiamond[colorIdx];

            // プール（連結リスト）への登録
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの弾の移動処理（拡大しながらわずかに旋回する軌道）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 外側へ広がりつつ、少しずつカーブさせて円盤のうねりを表現
        pShot->speed += 0.008;
        pShot->muki += 0.004;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_HikouEnban_Gemini()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // ボスは上部でゆったりと移動
        enemy.x = 240.0 + 80.0 * sin(count * 0.02);
        enemy.y = 80.0 + 15.0 * cos(count * 0.03);
    }

    // ボスのHP減少に伴うフェーズ変化（原作の色彩変化と密度・速度の上昇を再現）
    // 色一覧：0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
    int colorIdx = 4;    // 初期：青(4)
    int interval = 80/3;   // 発射間隔
    int speedParam = 0;  // 速度パラメータ

    if (enemy.hp <= 150) {
        colorIdx = 3;    // シアン(3)
        interval = 70/3;
        speedParam = 1;
    }
    if (enemy.hp <= 100) {
        colorIdx = 1;    // 黄(1)
        interval = 60/3;
        speedParam = 2;
    }
    if (enemy.hp <= 50) {
        colorIdx = 0;    // 赤(0)
        interval = 45/3;
        speedParam = 3;
    }

    if (enemy.hp % 50 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 一定間隔で飛空円盤（リング弾幕）を展開
    if (count % interval == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlyingSaucer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = colorIdx;       // 色情報を渡す
        pEnemyShotSet->param_i[0] = speedParam; // 速度パラメータを渡す

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}