// enemyPat_sunflower.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：陽華「サンフラワー・スプレッド」
static void ShotSunflowerSpread(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int c = pEnemyShotSet->count;

    // ----------------------------------------------------
    // フェーズ1: 開花 (0～180フレーム)
    // 茎のレーザーと、花びらを形成する黄色の弾を放射
    // ----------------------------------------------------
    if (c < 180) {
        // 茎 (緑の短レーザー) - 20フレームごと
        if (c % 20 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0; // 真下
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotLaser[2]; // 2:緑
            pEnemyShot->param_i[0] = 1; // 1:茎

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 花びら (黄色の菱形弾) - 3フレームごと
        if (c % 3 == 0) {
            // 音が重なりすぎないように間隔をあけて再生
            if (c % 15 == 0 && CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            if (c % 15 == 0) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int ways = 5; // 5方向のスパイラル
            double angle = c * 0.1; // 発射角度を少しずつ回転させる
            // 弾速に周期的な波(sin波)をつけることで、花びらのシルエットを形成
            double speed = 2.0 + 1.2 * sin(c * 0.1 * 4.0);

            for (int i = 0; i < ways; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle + (DX_PI * 2.0 / ways) * i;
                pEnemyShot->speed = speed;
                pEnemyShot->kind = img_enemyShotDiamond[1]; // 1:黄
                pEnemyShot->param_i[0] = 0; // 0:花びら

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // ----------------------------------------------------
    // フェーズ2: 成熟 (180～240フレーム)
    // 中心に黒い小玉(種)を高密度・低速で押し出す
    // ----------------------------------------------------
    else if (c >= 180 && c < 240) {
        if (c % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            int ways = 24; // 高密度
            double angle = (c % 20) * 0.1; // 少しずらしながら
            for (int i = 0; i < ways; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle + (DX_PI * 2.0 / ways) * i;
                // GetRand(50)は0～50を返すので、速度は0.5～1.0の極低速になる
                pEnemyShot->speed = 0.5 + GetRand(50) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[7]; // 7:黒
                pEnemyShot->param_i[0] = 2; // 2:種

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ----------------------------------------------------
    // ループ処理 (450フレームで最初に戻る)
    // ----------------------------------------------------
    if (c == 450) {
        pEnemyShotSet->count = 0;
        c = 0;
    }

    // ----------------------------------------------------
    // 既存の弾の挙動制御 (位置の更新とフェーズ移行)
    // ----------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    bool soundPlayed = false;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // フェーズ待機 (240～300フレーム): 全弾を減速してひまわりを一旦完成・停止させる
        if (c >= 240 && c < 300) {
            pShot->speed *= 0.95;
        }
        // フェーズ3開始 (300フレーム): 一斉飛散
        else if (c == 300) {
            if (pShot->param_i[0] == 2) {
                // 種(黒)は自機狙いの扇状弾として加速
                double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                // 自機方向を中心に ±30度のランダムなばらつき (GetRand(60)は0～60)
                pShot->muki = targetAngle + (GetRand(60) - 30) * DX_PI / 180.0;
                pShot->speed = 1.0 + GetRand(150) / 100.0; // 1.0～2.5
            }
            else if (pShot->param_i[0] == 0) {
                // 花びら(黄)は一定速度で外側に散る
                pShot->speed = 1.5;
            }

            // 飛散時の効果音は1度だけ鳴らす
            if (!soundPlayed) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                soundPlayed = true;
            }
        }
        // フェーズ3継続 (300フレーム以降): 種弾を徐々に加速させる
        else if (c > 300 && pShot->param_i[0] == 2) {
            if (pShot->speed < 5.0) {
                pShot->speed += 0.03;
            }
        }

        // 位置の更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Sunflower_Gemini()
{
    // 初期化
    if (count == 1) {
        // 画面中央上部付近で固定し、弾幕の造形を美しく見せる
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 弾幕セットの登録 (60フレーム目に開始)
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSunflowerSpread;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}