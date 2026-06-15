// enemyPat_HyperBeam.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：はかいこうせん（チャージ → 極太ビーム）
static void ShotHyperBeam(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // ----------------------------------------------------
    // [フェーズ1：チャージ] (0〜59フレーム)
    // 周囲の空間から中心(敵)に向かってエネルギーが集まる演出
    // ----------------------------------------------------
    if (c == 0)  PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    if (c >= 0 && c < 60) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 敵の周囲(半径80ピクセル)から発生
            double angle = (GetRand(360) / 180.0) * DX_PI;
            double dist = 80.0;
            pEnemyShot->x = pEnemyShotSet->x + dist * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(angle);

            // 中心に向かうように、進行角度はそのままで「マイナスの速度」を設定
            pEnemyShot->muki = angle;
            pEnemyShot->speed = -1.3; // 約60フレームで中心に到達

            pEnemyShot->kind = img_enemyShotDiamond[3]; // シアン(水色)の菱形弾でエネルギー感を演出

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----------------------------------------------------
    // [フェーズ2：ビーム発射] (60〜100フレーム)
    // 溜めたエネルギーを一気に極太の直線弾幕として放出
    // ----------------------------------------------------
    if (c >= 60 && c <= 100) {
        if (c == 60) {
            // 発射の瞬間にSE（※重い音がロードされていれば heavy の使用を推奨）
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        // 毎フレーム複数個の弾を進行方向の「垂直方向（横）」に並べて太い線を作る
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 進行方向に対して垂直な角度(法線)
            double normalAngle = pEnemyShotSet->muki + DX_PI / 2.0;

            // ビームの太さ（中心から -30 〜 +30 ピクセルずらす）
            int widthOffset = GetRand(60) - 30;

            pEnemyShot->x = pEnemyShotSet->x + widthOffset * cos(normalAngle);
            pEnemyShot->y = pEnemyShotSet->y + widthOffset * sin(normalAngle);
            pEnemyShot->muki = pEnemyShotSet->muki;

            // 圧倒的な速度と、微小なブレで破壊力とビームの荒々しさを表現
            pEnemyShot->speed = (1000 + GetRand(400)) / 100.0; // 10.0 〜 14.0

            // 弾の種類でグラデーションを作成（中心は白く大きく、外側は黄色や赤）
            if (widthOffset >= -10 && widthOffset <= 10) {
                pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白大玉(コア)
            }
            else if (widthOffset >= -20 && widthOffset <= 20) {
                pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[0]; // 赤鱗弾(外側に散るエネルギー)
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----------------------------------------------------
    // [フェーズ3：反動（クールダウン）] (100〜)
    // 弾の生成をストップし、しばらく撃たない（スキが生まれる）
    // ----------------------------------------------------

    // 敵が動いてもビームの起点がズレないように発射口を追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    // 全弾の座標更新処理
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 演出処理：発射フェーズに入ったら、中心に集まっていたチャージ弾も一緒に前方に撃ち出される
        if (pEnemyShot->speed < 0 && c >= 60) {
            pEnemyShot->muki = GetRand(359) / 180.0 * DX_PI;
            pEnemyShot->speed = (800 + GetRand(200)) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白小玉に変化して吹き飛ぶ
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hakaikousen_Gemini()
{
    static int muki;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 大技を使うので少し高めの耐久力に設定
        muki = 1;
    }
    else {
        // 反動フェーズの「スキ」を見せるために、ゆっくりと左右移動させる
        enemy.x += 0.5 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // 300フレーム（約5秒）周期で「はかいこうせん」のセットを生成
    if (count % 200 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperBeam;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 生成された瞬間にプレイヤーの位置を計算し、ビームの角度をロックオン
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // ダミーヘッドの初期化とリストへの追加
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}