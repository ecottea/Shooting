// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 通常の直線移動を行う関数（各弾セットの更新用）
static void ShotMoveNormal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 各弾に設定された現在の向きと速度で愚直に前進
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 大砂嵐モチーフ弾幕：『絶望の砂塵嵐（ハブーブ・ボルテックス）』
void EnemyPat_SandStorm_Gemini()
{
    static int muki;

    // 1. 敵本体の初期化と移動処理
    if (count == 1) {
        // ゲーム画面 480x480 の上部中央に配置
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // HPは200固定
        muki = 1;
    }
    else {
        // 砂嵐の発生源として、ボス本体は画面上部をゆっくりと左右に往復
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 1サイクル 400フレーム（約6.6秒）のタイムラインで弾幕を管理
    int cycle = count % 400;

    // --------------------------------------------------------
    // 【第1段階：視界の遮蔽（砂粒弾の降下）】
    // --------------------------------------------------------
    // サイクル開始から280フレームまで、8フレームに1回画面上部から低速の砂粒を降らせる
    if (cycle >= 1 && cycle <= 280 && cycle % 2 == 0) {
        // 演出：一定間隔で砂が擦れるような軽いショット音を再生
        if (cycle % 32 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMoveNormal;

        // 画面上部（Y = -10）のランダムなX座標から発生させる
        pEnemyShotSet->x = (double)GetRand(480);
        pEnemyShotSet->y = -10.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下方向
        pEnemyShotSet->kind = 100;         // 後から突風で操作するため、砂粒弾の識別IDを100とする

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾を1つ生成
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        // 真下からわずかに角度をばらつかせる（GetRand(30)-15 で -15度〜+15度）
        pEnemyShot->muki = pEnemyShotSet->muki + ((GetRand(30) - 15) / 180.0 * DX_PI);
        pEnemyShot->speed = (80 + GetRand(60)) / 100.0 * 2; // 速度 0.8 〜 1.4 の超低速
        pEnemyShot->kind = img_enemyShotSmallBall[8];  // 砂をイメージして「橙色」の「小玉」

        // セットのリストへ接続
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // メインの全体リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --------------------------------------------------------
    // 【第2段階：砂嵐の発生（蛇行する竜巻弾幕）】
    // --------------------------------------------------------
    // サイクル 100〜280フレームの間、ボス位置から左右に激しくうねる竜巻を放出
    if (cycle >= 100 && cycle <= 280 && cycle % 6 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 放出角度をサイン波で左右に大きく揺らす（最大±50度）
        double angleOffset = sin((cycle - 100) / 15.0) * (50.0 / 180.0 * DX_PI);

        // 左右に展開する一対の竜巻流を作るため2回ループ
        for (int i = 0; i < 2; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotMoveNormal;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->kind = 200; // 竜巻弾の識別IDを200とする

            // i==0なら正方向、i==1なら負方向に角度をオフセット
            double finalMuki = DX_PI / 2.0 + (i == 0 ? angleOffset : -angleOffset);
            pEnemyShotSet->muki = finalMuki;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 弾を1つ生成
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 2.2; // プレイヤーが引きつけて避けるための中速

            if (i == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[1]; // 左ストリーム：黄色の中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotLargeBall[0];  // 右ストリーム：赤色の大玉（激しい核のイメージ）
            }

            // セットのリストへ接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // メインの全体リストへ追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // 突風の20フレーム前に予告音（チャージ音）を鳴らす
    if (cycle == 280) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 【第3段階：砂塵の乱気流（一斉突風ギミック）】
    // --------------------------------------------------------
    // サイクル 320フレーム目で、画面上にたまっている砂粒弾を一斉に真横へ吹き飛ばす
    if (cycle == 320) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 突風の方向を、サイクル（count/400）ごとに「右（0.0）」と「左（DX_PI）」で交互に切り替える
        double windMuki = ((count / 400) % 2 == 0) ? 0.0 : DX_PI;

        // グローバルリストから現在存在するすべての弾セットを走査
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            // 砂粒弾（kind == 100）のセットを見つけたら、その中のすべての弾を書き換える
            if (pSet->kind == 100) {
                sEnemyShot* pEnemyShot = pSet->pEnemyShotHead->next;
                while (pEnemyShot != pSet->pEnemyShotHead) {
                    // ベクトルの急変化：真横へ向けて超高速化
                    // わずかに上下にばらつき（GetRand(10)-5 で -5度〜+5度）を持たせて、吹き荒れるリアルな突風感を演出
                    pEnemyShot->muki = windMuki + ((GetRand(10) - 5) / 180.0 * DX_PI);
                    pEnemyShot->speed = 5.2 + (GetRand(150) / 100.0); // 5.2 〜 6.7 の高速へ変化

                    // 形状を「銃弾」に変更し、風で引き裂かれるような鋭い見た目に変貌させる
                    pEnemyShot->kind = img_enemyShotBullet[8]; // 橙色の銃弾

                    pEnemyShot = pEnemyShot->next;
                }
            }
            pSet = pSet->next;
        }
    }
}