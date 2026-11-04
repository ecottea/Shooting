#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  飛鉢「伝説の飛空円盤」風弾幕
//  (『東方星蓮船 〜 Undefined Fantastic Object』6面ボス
//   聖白蓮の最終スペルカード。Hard/Lunatic時の名称で、
//   Easy/Normalでは「飛鉢「フライングファンタスティカ」」)
// ------------------------------------------------------------
//  【原作の弾幕の仕組み(調査に基づく)】
//   1. 白蓮が背後に魔法陣(花のような形)を展開する
//   2. その「花のような部分」から、お札型の弾が円状(リング状)に
//      発射される
//   3. お札弾同士は青色の線(小さな弾の連なり)で繋がっている
//   4. 弾幕全体はゆっくりと膨張と収縮を繰り返しながら、
//      次々と自機に向かって降ってくる
//   5. 白蓮の体力に応じてお札の色が 青→水色→黄→紫→赤 の順に変化する
//   6. 戦闘が後半になるほど弾の量が増える
//
//  【実装方針】
//   ・「お札+連結弾で描く1つのリング(花の輪)」= sEnemyShotSet 1つ
//     (お札12枚 + それを繋ぐ小玉60個 = 1リング72弾)
//   ・リングの状態(中心・降下速度・回転・膨張収縮)は sEnemyShotSet の
//     param_d / param_i に保持し、patternFunc 内でリング上の全弾の
//     位置を毎フレーム直接計算して配置し直す
//   ・リングは生成時に自機方向を向き、以後は直進して画面外へ抜ける。
//     画面外の弾の消去・空になったセットの後始末はメインルーチンが
//     行う仕様なので、ここでは何も削除しない
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント
//     もメインルーチンが行う仕様を前提とする
//
//  【使用した素材の選定】
//   ・お札型の弾 → 中楕円弾 img_enemyShotMediumOval (10.5x7.0)
//       素材一覧にお札画像がないため、縦長で大きく視認性の良い
//       中楕円弾でお札を代用する
//       色はHPに応じて 青(4)→水色(3)→黄(1)→紫(5)→赤(0) と変化
//   ・繋ぐ「線」→ 小玉 img_enemyShotSmallBall (2.5x2.5)、常に青(4)
//       お札の間を密に埋めて「青色の線で繋がっている」見た目を作る
//   ・音 → sound_enemyCharge   (符卡発動の予告音)
//          sound_enemyShot_medium (リングの放出音)
//   (大玉/銃弾/鱗弾/菱形弾/短レーザー他はこの符卡では使用しない)
//
//  【sEnemyShotSet のパラメータの割り当て】
//   param_d[0] : リング中心のx
//   param_d[1] : リング中心のy
//   param_d[2] : リング中心の移動方向(生成時に自機狙いで固定)
//   param_d[3] : リング中心の降下速度
//   param_d[4] : リングの回転角速度(ラジアン/フレーム)
//   param_d[5] : リングの現在の回転角
//   param_d[6] : 膨張収縮の角速度(ラジアン/フレーム)
//   param_i[0] : お札の数
//   param_i[1] : 隣接するお札の間の連結弾の数
//   param_i[2] : お札の色(色番号 0〜8)
//   param_i[3] : 基準半径
//   param_i[4] : 膨張収縮の振幅
//  【sEnemyShot のパラメータの割り当て】
//   param_d[0] : リング上でのこの弾の基準角度
//   param_i[0] : 0=お札 / 1=連結弾
// ============================================================

// HP(割合)に応じたお札の色番号を返す
// 原作同様、体力に応じて 青→水色→黄→紫→赤 の順に変化する
static int BowlColorByHp()
{
    double ratio = (double)enemy.hp / (double)enemy.maxHp;
    if (ratio > 0.8) return 4; // 青
    if (ratio > 0.6) return 3; // 水色(シアン)
    if (ratio > 0.4) return 1; // 黄
    if (ratio > 0.2) return 5; // 紫(マゼンタ)
    return 0;                  // 赤
}

// 弾幕：飛鉢(お札と連結弾で描くリング)の放出と降下
static void ShotFlyingBowl(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // セット生成フレーム：リングを一気に組み上げる
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium,
        //                   sound_enemyShot_heavy, sound_enemyShot_extreme,
        //                   sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = pEnemyShotSet->param_i[0]; // お札の数
        int m = pEnemyShotSet->param_i[1]; // お札の間の連結弾の数
        for (int i = 0; i < n; i++) {
            double base = (double)i * (DX_PI * 2.0 / (double)n);
            // k==0 がお札、k>=1 がお札の間を埋める連結弾
            for (int k = 0; k <= m; k++) {
                pEnemyShot = new sEnemyShot;
                // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、
                //   銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
                // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
                if (k == 0) {
                    pEnemyShot->param_i[0] = 0; // 0:お札
                    pEnemyShot->kind = img_enemyShotMediumOval[pEnemyShotSet->param_i[2]];
                }
                else {
                    pEnemyShot->param_i[0] = 1; // 1:連結弾(常に青)
                    pEnemyShot->kind = img_enemyShotSmallBall[4];
                }
                pEnemyShot->param_d[0] = base + (double)k * (DX_PI * 2.0 / (double)n) / (double)(m + 1);
                pEnemyShot->x = pEnemyShotSet->param_d[0];
                pEnemyShot->y = pEnemyShotSet->param_d[1];
                pEnemyShot->margin = 240;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // リングをゆっくり回転させる(花弁が回るように見える)
    pEnemyShotSet->param_d[5] += pEnemyShotSet->param_d[4];
    // リングの中心を自機方向へ降下させる
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[3] * cos(pEnemyShotSet->param_d[2]);
    pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[3] * sin(pEnemyShotSet->param_d[2]);

    // 現在のリング半径：基準半径がゆっくりと膨張と収縮を繰り返す
    double r = (double)pEnemyShotSet->param_i[3]
        + (double)pEnemyShotSet->param_i[4] * sin(pEnemyShotSet->param_d[6] * (double)pEnemyShotSet->count);

    // 全ての弾をリング上の正しい位置へ配置し直す
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double theta = pShot->param_d[0] + pEnemyShotSet->param_d[5];
        pShot->x = pEnemyShotSet->param_d[0] + r * cos(theta);
        pShot->y = pEnemyShotSet->param_d[1] + r * sin(theta);
        pShot->muki = theta; // お札が放射状を向き、花弁っぽく見える
        pShot = pShot->next;
    }
}

// リング(お札+連結弾の円陣)を1つ生成する
static void SpawnRing(int n) // n: 何個目のリングか
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotFlyingBowl;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->kind = n;
    // リングの中心はボスの位置からスタートし、自機へ向かって降りてくる
    pEnemyShotSet->param_d[0] = enemy.x;
    pEnemyShotSet->param_d[1] = enemy.y;
    pEnemyShotSet->param_d[2] = atan2(player.y - enemy.y, player.x - enemy.x);
    // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意！
    // リプレイファイルにseed値を保存するので再現性あり。
    pEnemyShotSet->param_d[3] = 0.85 + GetRand(40) / 100.0;            // 降下速度 0.85〜1.25
    pEnemyShotSet->param_d[4] = ((GetRand(1) == 0) ? 0.008 : -0.008);  // 回転方向は毎回ランダム
    pEnemyShotSet->param_d[5] = (GetRand(360) - 180) / 180.0 * DX_PI;  // 回転の初期角 -180〜180度
    pEnemyShotSet->param_d[6] = 0.024; // 膨張収縮の角速度(周期 約4.4秒)
    pEnemyShotSet->param_i[0] = 12;    // お札12枚(12角形の花)
    pEnemyShotSet->param_i[1] = 5;     // お札の間を小玉5個で繋ぐ
    pEnemyShotSet->param_i[2] = BowlColorByHp(); // お札の色は現在のHPで決める
    pEnemyShotSet->param_i[3] = 50*5;    // 基準半径
    pEnemyShotSet->param_i[4] = 25;    // 膨張収縮の振幅(半径25〜75)
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_HikouEnban_Zai()
{
    static int ringCount; // 生成したリングの数
    static int nextRing;  // 次にリングを生成するcountの値

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        ringCount = 0;
        nextRing = 50;
        // 符卡発動の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 白蓮は画面上部をゆったりと漂う
        enemy.x = 240.0 + 60.0 * sin((count - 1) * 0.008);
        enemy.y = 60.0 + 10.0 * sin((count - 1) * 0.015);
    }

    // 魔法陣の花からリングを次々と放出する。
    // HPが減るほど間隔が短くなり、後半になるほど弾の量が増える(原作準拠)
    if (count >= nextRing) {
        SpawnRing(ringCount++);
        double hpRatio = (double)enemy.hp / (double)enemy.maxHp;
        int interval;
        if (hpRatio > 0.8)      interval = 170; // 青
        else if (hpRatio > 0.6) interval = 140; // 水色
        else if (hpRatio > 0.4) interval = 115; // 黄
        else if (hpRatio > 0.2) interval = 95;  // 紫
        else                    interval = 80;  // 赤
        nextRing = count + interval;
    }
}
