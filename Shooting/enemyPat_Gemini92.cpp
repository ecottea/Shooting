// enemyPat_meteorBreak.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：空中破砕（メテオ・ブレイク）
static void ShotMeteorBreak(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化：隕石（核と尾）の生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0; // フェーズ: 0=降下中, 1=破砕済み

        double startMuki = pEnemyShotSet->muki; // 自機狙い角度
        double meteorSpeed = 4.5; // 隕石の落下速度

        // 1. 核の生成 (赤色の大弾)
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = startMuki;
        pEnemyShot->speed = meteorSpeed;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉
        pEnemyShot->param_i[0] = 0; // 役割0: 核

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 2. 尾の生成 (黄色の小弾をV字に配置して追従)
        double backMuki = startMuki + DX_PI; // 進行方向の真後ろ
        for (int i = 1; i <= 6; i++) {
            for (int lr = -1; lr <= 1; lr++) {
                if (lr == 0 && i > 3) continue; // 中心はやや短めにする

                pEnemyShot = new sEnemyShot;
                // 後方に i*14.0、左右に i*8.0 ずらしてV字を形成
                double distBack = i * 14.0;
                double distSide = i * 8.0;
                double dx = cos(backMuki) * distBack + cos(backMuki + DX_PI / 2.0) * distSide * lr;
                double dy = sin(backMuki) * distBack + sin(backMuki + DX_PI / 2.0) * distSide * lr;

                pEnemyShot->x = pEnemyShotSet->x + dx;
                pEnemyShot->y = pEnemyShotSet->y + dy;
                pEnemyShot->muki = startMuki;
                pEnemyShot->speed = meteorSpeed; // 核と完全に同じ速度で同期移動
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄の小玉
                pEnemyShot->param_i[0] = 1; // 役割1: 尾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --- 毎フレームの全弾更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    sEnemyShot* pNuclear = nullptr; // 核となる大弾へのポインタを探す

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾の基本移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 役割ごとの特殊処理
        if (pShot->param_i[0] == 0) {
            pNuclear = pShot; // 核のポインタを確保
        }
        else if (pShot->param_i[0] == 2) {
            // 火花: 空気抵抗のように徐々に減速させる
            if (pShot->speed > 0.3) pShot->speed -= 0.03;
        }

        pShot = pShot->next;
    }

    // --- 降下フェーズ（火花の発生と破砕判定） ---
    if (pEnemyShotSet->param_i[0] == 0) {
        if (pNuclear != nullptr) {
            // 落下中の摩耗 (火花の飛散)
            // 毎フレーム、核の左右から菱形弾を少しずつ吐き出す
            if (pEnemyShotSet->count % 2 == 0) {
                for (int lr = -1; lr <= 1; lr += 2) {
                    pEnemyShot = new sEnemyShot;
                    // 大玉の端付近(中心から約15px)から発生
                    double angleSide = pNuclear->muki + DX_PI / 2.0 * lr;
                    pEnemyShot->x = pNuclear->x + cos(angleSide) * 15.0;
                    pEnemyShot->y = pNuclear->y + sin(angleSide) * 15.0;

                    // 進行方向の逆向きへ不規則な角度で散らす
                    double backMuki = pNuclear->muki + DX_PI;
                    // GetRand(40)は 0~40 なので、-20~20の範囲になる
                    pEnemyShot->muki = backMuki + (GetRand(40) - 20) / 180.0 * DX_PI;
                    pEnemyShot->speed = (80 + GetRand(70)) / 100.0; // 0.8 ~ 1.5の低速
                    pEnemyShot->kind = img_enemyShotDiamond[8]; // 橙の菱形弾 (米粒弾)
                    pEnemyShot->param_i[0] = 2; // 役割2: 火花

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }

            // 空中破砕 (画面中央付近: 480x480なので Y座標が240を超えたら破砕)
            if (pNuclear->y > 240.0 || pEnemyShotSet->count > 150) {
                pEnemyShotSet->param_i[0] = 1; // 破砕済みフェーズへ移行

                double breakX = pNuclear->x;
                double breakY = pNuclear->y;
                double nucMuki = pNuclear->muki;

                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                // --- 核と尾の弾を消滅させる ---
                pShot = pEnemyShotSet->pEnemyShotHead->next;
                while (pShot != pEnemyShotSet->pEnemyShotHead) {
                    sEnemyShot* nextShot = pShot->next;
                    // 役割0(核) または 1(尾) の弾だけリストから外して消去
                    if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                        pShot->prev->next = pShot->next;
                        pShot->next->prev = pShot->prev;
                        delete pShot;
                    }
                    pShot = nextShot;
                }

                // --- 衝撃波の発生 (黄色の小弾が正円を描いて全方位へ展開) ---
                int way = 36 * 2;
                for (int i = 0; i < way; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = breakX;
                    pEnemyShot->y = breakY;
                    pEnemyShot->muki = (DX_PI * 2.0 / way) * i;
                    pEnemyShot->speed = 5.5; // 高速展開
                    pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄の小玉
                    pEnemyShot->param_i[0] = 3; // 役割3: 衝撃波

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }

                // --- 岩片の飛散 (青・黒の中弾が退路を塞ぐように散開) ---
                for (int i = 0; i < 12 * 5; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = breakX;
                    pEnemyShot->y = breakY;
                    // 前方に広くランダムに散らす
                    pEnemyShot->muki = nucMuki + (GetRand(160) - 80) / 180.0 * DX_PI;
                    pEnemyShot->speed = (120 + GetRand(130)) / 100.0; // 1.2 ~ 2.5の低速
                    // 青と黒をランダムに混ぜる
                    pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[4] : img_enemyShotMediumBall[7];
                    pEnemyShot->param_i[0] = 4; // 役割4: 岩片

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
        else {
            // 何らかの理由(画面外など)で核が消えた場合は安全のためフェーズを進める
            pEnemyShotSet->param_i[0] = 1;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Meteor_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右へゆっくり移動
        enemy.x += 1.0 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 160フレーム周期でメテオ・ブレイクを発動
    if (count % 160 == 60) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 160 == 120) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMeteorBreak;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0; // 敵の少し下から発射

        // 自機を狙う角度を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // エミッタをリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}