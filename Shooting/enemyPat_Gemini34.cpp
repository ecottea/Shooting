// enemyPat_FrictionHeat.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：導火の擦過（フリクション・ヒート）
static void Pattern_FrictionHeat(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    // 1. マッチの投下（未着火状態の軸をパラパラと落とす）
    // --------------------------------------------------------
    if (pEnemyShotSet->count % 8 == 0) {
        sEnemyShot* match = new sEnemyShot;
        match->x = pEnemyShotSet->x + GetRand(400) - 200; // ボス周辺の広い範囲から投下
        match->y = pEnemyShotSet->y;
        match->muki = DX_PI / 2.0;                        // 真下へ落下
        match->speed = (150 + GetRand(100)) / 100.0;      // 速度 1.5 ～ 2.5
        match->kind = img_enemyShotScale[8];              // 橙色の鱗弾（マッチの軸）
        match->param_i[0] = 0;                            // 状態管理（0:未着火, 1:炎上, 2:灰, 3:炎弾）

        // リストの末尾（headのprev）に追加
        match->prev = pEnemyShotSet->pEnemyShotHead->prev;
        match->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = match;
        pEnemyShotSet->pEnemyShotHead->prev = match;
    }

    bool playedIgniteSound = false; // 1フレーム内で何度も着火音が鳴るのを防ぐフラグ

    // --------------------------------------------------------
    // 2. 画面上の全弾（マッチ・炎弾・灰）の更新
    // --------------------------------------------------------
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // param_i[0] に保存した状態ごとのギミック処理
        switch (pEnemyShot->param_i[0]) {
        case 0: // 【状態0：未着火】
            // 摩擦ライン（Y=220）を通過したら発火
            if (pEnemyShot->y >= 220.0) {
                pEnemyShot->param_i[0] = 1;                   // 状態を「炎上」へ
                pEnemyShot->param_i[1] = 0;                   // 炎上タイマーを初期化
                pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉（火柱）に変化

                if (!playedIgniteSound) {
                    if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                    playedIgniteSound = true;
                }
            }
            break;

        case 1: // 【状態1：炎上中】
            pEnemyShot->param_i[1]++;  // 炎上時間をカウント
            pEnemyShot->speed += 0.03; // 燃えながら少しずつ加速して落ちる

            // 5フレームに1回、周囲に炎弾（火の粉）をばら撒く
            if (pEnemyShot->param_i[1] % 5 == 0) {
                // 自機方向を基準に少しバラけさせる
                double angle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);

                sEnemyShot* fire = new sEnemyShot;
                fire->x = pEnemyShot->x;
                fire->y = pEnemyShot->y;
                fire->muki = angle + (GetRand(30) - 15) / 180.0 * DX_PI; // 自機狙い＋ランダムブレ
                fire->speed = (300 + GetRand(200)) / 100.0;              // 速度 3.0 ～ 5.0

                // 炎弾の色を赤・黄・橙からランダムで選ぶ
                int c = GetRand(2);
                if (c == 0) fire->kind = img_enemyShotSmallBall[0];
                else if (c == 1) fire->kind = img_enemyShotSmallBall[1];
                else fire->kind = img_enemyShotSmallBall[8];

                fire->param_i[0] = 3; // 状態を「ただの炎弾」に設定

                // 新しい炎弾をリストに追加
                fire->prev = pEnemyShotSet->pEnemyShotHead->prev;
                fire->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = fire;
                pEnemyShotSet->pEnemyShotHead->prev = fire;
            }

            // 一定時間（約1秒）燃え続けると灰になる
            if (pEnemyShot->param_i[1] >= 30) {
                pEnemyShot->param_i[0] = 2;                   // 状態を「灰」へ
                pEnemyShot->kind = img_enemyShotMediumBall[7];// 黒の中玉に変化
            }
            break;

        case 2: // 【状態2：灰（燃え尽き後）】
            // 空気抵抗を受けているように急減速し、フワフワ落ちる
            if (pEnemyShot->speed > 0.8) {
                pEnemyShot->speed *= 0.92;
            }
            break;

        case 3: // 【状態3：炎弾（ばら撒かれた火の粉）】
            // 特殊な挙動はせず、設定された向きと速度でまっすぐ飛ぶだけ
            break;
        }

        // 座標の更新（全状態共通）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（メインから呼ばれる関数）
void EnemyPat_Match_Gemini()
{
    static int muki;

    if (count == 1) {
        // 初期位置とHPの設定
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 弾幕（フリクション・ヒート）の管理セットを1つだけ登録
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Pattern_FrictionHeat;
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
    else {
        // ボス本体は画面上部を左右にゆったり移動し続ける
        enemy.x += 1.2 * (double)muki;
        // 画面端の手前で折り返す (画面幅480)
        if (enemy.x > 380.0) muki = -1;
        if (enemy.x < 100.0) muki = 1;

        // 弾幕の発射起点をボスの位置に追従させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        if (pSet != &enemyShotSetHead) {
            pSet->x = enemy.x;
            pSet->y = enemy.y;
        }
    }
}