// enemyPat_TomatoSplash.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 「完熟トマティーナ・スプラッシュ」用弾幕パターン
static void ShotTomatoSplash(sEnemyShotSet* pEnemyShotSet)
{
    // 大弾(トマト)の発射
    if (pEnemyShotSet->count == 0) {
        // 発射音（ドスッという感じの重めの音）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pTomato = new sEnemyShot;
        pTomato->x = pEnemyShotSet->x;
        pTomato->y = pEnemyShotSet->y;
        pTomato->muki = pEnemyShotSet->muki;

        // 初速を少しランダムに（2.5 〜 3.5）
        // GetRand(10) は 0〜10 を返す
        pTomato->speed = (25 + GetRand(10)) / 10.0 + 3;

        // 色と種類：赤の大弾
        pTomato->kind = img_enemyShotLargeBall[0];

        // 状態管理用パラメータ
        // param_i[0] -> 0: トマト本体, 1: 果汁(小弾)
        pTomato->param_i[0] = 0;
        // param_i[1] -> 破裂するまでのフレーム数(50〜70)
        pTomato->param_i[1] = 50 + GetRand(20);
        pTomato->margin = 240;

        // リストに追加
        pTomato->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pTomato->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pTomato;
        pEnemyShotSet->pEnemyShotHead->prev = pTomato;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // nextポインタを保存しておく
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) {
            // ----- トマト本体の挙動 -----
            // 空気抵抗のように徐々に減速させる
            if (pShot->speed > 0.0) {
                pShot->speed -= 0.05;
                if (pShot->speed < 0.0) pShot->speed = 0.0;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 破裂タイミングの判定
            if (pShot->count >= pShot->param_i[1]) {
                // 破裂音（激しい音）
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                // 果汁（小弾）を全方位にばら撒く
                // GetRand(8)は0〜8なので16〜24方向
                int way = 16 + GetRand(8);
                for (int i = 0; i < way; i++) {
                    // 各方向に2〜3個
                    int drops = 2 + GetRand(1);
                    for (int j = 0; j < drops; j++) {
                        sEnemyShot* pJuice = new sEnemyShot;
                        pJuice->x = pShot->x;
                        pJuice->y = pShot->y;

                        // 基準の角度 ＋ 少しのランダムなブレ (-10度〜+10度)
                        double baseAngle = (DX_PI * 2.0 / way) * i;
                        double offsetAngle = (GetRand(20) - 10) / 180.0 * DX_PI;
                        pJuice->muki = baseAngle + offsetAngle;

                        // 速度をバラバラに（ドロッとした不規則な飛散を表現）
                        // GetRand(35)は0〜35。0.5 〜 4.0
                        pJuice->speed = (5 + GetRand(35)) / 10.0;

                        // 色（赤、黄、橙）をランダムに選ぶ
                        int colorRand = GetRand(2); // 0, 1, 2
                        int colorIdx = 0;
                        if (colorRand == 0) colorIdx = 0; // 赤
                        else if (colorRand == 1) colorIdx = 1; // 黄
                        else colorIdx = 8; // 橙

                        pJuice->kind = img_enemyShotSmallBall[colorIdx];
                        pJuice->param_i[0] = 1; // 果汁(小弾)フラグ
                        pJuice->margin = 240;

                        // リストの末尾に追加
                        pJuice->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pJuice->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pJuice;
                        pEnemyShotSet->pEnemyShotHead->prev = pJuice;
                    }
                }

                // トマト本体を画面外へ退避し、メインルーチンの自動消去に任せる
                pShot->x = -9999.0;
                pShot->y = -9999.0;
                pShot->speed = 0.0;
                pShot->param_i[0] = -1; // 処理済みフラグ（以降無効）
            }
        }
        else if (pShot->param_i[0] == 1) {
            // ----- 果汁(小弾)の挙動 -----
            // シンプルな等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Tomatina_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480 (中央は x=240)
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;
    }
    else {
        // 左右にゆらゆら移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 120フレーム周期で、最初の30フレーム間で10フレーム間隔でトマトを投擲（計4個）
    int cycle = count % 120;
    if (cycle >= 0 && cycle <= 30 && cycle % 10 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTomatoSplash;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // プレイヤーの方向を狙いつつ、少しだけ軌道をブレさせる (-10度〜+10度)
        double targetAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double blur = (GetRand(20) - 10) / 180.0 * DX_PI;
        pEnemyShotSet->muki = targetAngle + blur;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}