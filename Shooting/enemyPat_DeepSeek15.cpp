// enemyPat_Tmp.cpp
// 洗濯機（脱水～泡すすぎ）をイメージした複合弾幕
//   - 敵は画面内をゆらゆらと複雑に動く（ドラムの振動）
//   - 一定間隔で「脱水回転＋飛沫＋泡」を重ねて放つ
//   - 弾幕セット内でタイミングをずらしながら複数種類の弾を生成し、
//     見た目にも変化に富んだ攻撃になる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------
//  弾幕パターン：脱水スピン＋泡飛沫
//     敵の位置を中心に、フェーズごとに異なる弾を生成する。
//     カウント（フレーム経過）によって以下の攻撃を追加する：
//       0   : 強めの効果音を再生
//       0   : 回転しながら広がる水のリング（小玉・青） 16発
//       12  : ドラムの飛沫（中玉・シアン）ランダム方向 8発
//       24  : 高速の脱水リング（小玉・白） 32発
//       36  : 大きな泡（大玉・マゼンタ）ゆっくり 6発
//       48  : 接線方向へ飛ぶ回転水滴（小玉・青） 12発
//       60  : プレイヤーを狙う泡弾（中玉・黄） 4発（やや誘導）
// --------------------------------------------------
static void ShotWashingMachine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = nullptr;

    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;
    const int    cnt = pEnemyShotSet->count;   // このセットが生成されてからのフレーム数

    // --- 効果音（初回のみ） ---
    if (cnt == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---------- 各フェーズで弾を生成 ----------
    if (cnt == 0) {
        // 【リング１：回転しながら広がる水】
        const int    num = 16;
        const double baseAngle = cnt * 0.1; // 少し位相をずらす（cnt==0では0）
        const double outward = 2.2;
        const double tangent = 1.8;
        for (int i = 0; i < num; ++i) {
            double theta = 2.0 * DX_PI * i / num + baseAngle;
            // 方向：放射方向 + 接線方向を合成
            double vx = outward * cos(theta) - tangent * sin(theta);
            double vy = outward * sin(theta) + tangent * cos(theta);
            double spd = sqrt(vx * vx + vy * vy);
            double ang = atan2(vy, vx);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + cos(theta) * 14.0;
            pEnemyShot->y = cy + sin(theta) * 14.0;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青（水）
            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (cnt == 12) {
        // 【飛沫：ランダム方向に中玉（シアン）】
        const int num = 8;
        for (int i = 0; i < num; ++i) {
            double theta = GetRand(360) / 180.0 * DX_PI; // 0～2π ランダム
            double spd = (180.0 + GetRand(120)) / 100.0; // 1.8～3.0
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = theta;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (cnt == 24) {
        // 【高速リング２：白い小玉・多数】
        const int num = 32;
        const double outward = 3.2;
        const double tangent = 0.6; // 弱い接線成分
        for (int i = 0; i < num; ++i) {
            double theta = 2.0 * DX_PI * i / num;
            double vx = outward * cos(theta) - tangent * sin(theta);
            double vy = outward * sin(theta) + tangent * cos(theta);
            double spd = sqrt(vx * vx + vy * vy);
            double ang = atan2(vy, vx);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + cos(theta) * 10.0;
            pEnemyShot->y = cy + sin(theta) * 10.0;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (cnt == 36) {
        // 【大きな泡：マゼンタの大玉、ゆっくり広がる】
        const int num = 6;
        for (int i = 0; i < num; ++i) {
            double theta = 2.0 * DX_PI * i / num + 0.3; // 位相ずらし
            double spd = 1.2;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = theta;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotLargeBall[5]; // マゼンタ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (cnt == 48) {
        // 【接線方向の水滴：外側へ勢いよく飛ぶ】
        const int num = 12;
        const double outward = 0.5;  // わずかな外向き
        const double tangent = 2.8;
        for (int i = 0; i < num; ++i) {
            double theta = 2.0 * DX_PI * i / num + DX_PI / num; // 少しずらす
            double vx = outward * cos(theta) - tangent * sin(theta);
            double vy = outward * sin(theta) + tangent * cos(theta);
            double spd = sqrt(vx * vx + vy * vy);
            double ang = atan2(vy, vx);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + cos(theta) * 20.0;
            pEnemyShot->y = cy + sin(theta) * 20.0;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (cnt == 60) {
        // 【プレイヤー狙いの泡弾（やや誘導）】
        const int num = 4;
        double baseAim = atan2(player.y - cy, player.x - cx);
        for (int i = 0; i < num; ++i) {
            double offset = (GetRand(60) - 30) / 180.0 * DX_PI; // ±30度
            double ang = baseAim + offset;
            double spd = 2.0;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄色
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---------- 全弾の移動（毎フレーム） ----------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------
//  敵本体パターン（洗濯機）
//     ゆらゆらと不規則に動きつつ、一定間隔で
//     上記の弾幕セットを生成し続ける。
// --------------------------------------------------
void EnemyPat_WashingMachine_DeepSeek()
{
    // 敵の揺れ方を制御するための静的変数
    static double phaseX = 0.0;
    static double phaseY = 1.2; // 位相をずらす

    // 初回のみ初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        phaseX = 0.0;
        phaseY = 1.2;
    }
    else {
        // 複雑な振動（リサージュ風の揺れ）
        enemy.x = 240.0 + 90.0 * sin(phaseX);
        enemy.y = 130.0 + 50.0 * cos(phaseY * 1.3); // 上下動も加える
        phaseX += 0.045;
        phaseY += 0.055;
        // 画面端で跳ね返る必要はないが、念のためクランプ
        if (enemy.x < 40.0)  enemy.x = 40.0;
        if (enemy.x > 440.0) enemy.x = 440.0;
        if (enemy.y < 30.0)  enemy.y = 30.0;
        if (enemy.y > 250.0) enemy.y = 250.0;
    }

    // 新しい弾幕セットを定期的に生成（45フレームごと）
    // 連続して重ねることで、常に複数のパターンが画面に存在する
    if (count % 45 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;            // このセットの経過フレーム
        pEnemyShotSet->patternFunc = ShotWashingMachine;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // muki にはプレイヤー方向を保存（今回のパターンでは未使用だが参考として）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // ダミーヘッド付き弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体の弾幕セット二重連結リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}