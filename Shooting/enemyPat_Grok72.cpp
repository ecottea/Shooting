// enemyPat_Tmp.cpp
// 氷符「エターナル・フォース・ブリザード」モチーフ弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------
// 使用素材の選定（氷テーマ）
// 音 : sound_enemyShot_medium / heavy / extreme / sound_enemyCharge
// 弾 : SmallBall, MediumBall, LargeBall, Diamond
// 色 : 3=シアン, 4=青, 6=白 （必要に応じて他色も少数使用）
// ---------------------------------------------------------------

// 第1段階：まばらな落下氷柱＋軽い散弾
static void ShotIceFall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 縦方向に落ちる氷柱風（小玉・中玉）
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 3) * 28.0 + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y - 20.0 - GetRand(30);
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI; // ほぼ真下
            pEnemyShot->speed = 1.6 + GetRand(80) / 100.0;
            // シアン or 青の小玉/中玉
            if (GetRand(1) == 0)
                pEnemyShot->kind = img_enemyShotSmallBall[3 + GetRand(1)]; // 3 or 4
            else
                pEnemyShot->kind = img_enemyShotMediumBall[3 + GetRand(1)];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 左右から弧を描く破片（菱形）
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            int side = (i % 2 == 0) ? -1 : 1;
            pEnemyShot->x = pEnemyShotSet->x + side * (80 + GetRand(40));
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x)
                + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotDiamond[3 + GetRand(1)]; // シアン/青の菱形
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 第2段階：放射状結晶 → 一定時間後に分裂
static void ShotIceCrystal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 中心から放射状に中玉を射出
        int num = 12 + GetRand(4);
        double baseMuki = pEnemyShotSet->muki;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseMuki + (i * 2.0 * DX_PI / num) + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.2 + GetRand(60) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3 + (i % 2)]; // シアン/青交互
            // 分裂用タイマーを param_i[0] に保存
            pEnemyShot->param_i[0] = 40 + GetRand(20); // 分裂までのフレーム
            pEnemyShot->param_i[1] = 0; // 分裂済みフラグ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動＋分裂処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // まだ分裂していない場合のみ移動
        if (pShot->param_i[1] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 分裂タイミング
            if (pShot->count >= pShot->param_i[0]) {
                pShot->param_i[1] = 1; // 分裂済み

                // 破片を6方向に生成
                for (int k = 0; k < 6; k++) {
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pShot->x;
                    pFrag->y = pShot->y;
                    pFrag->muki = pShot->muki + (k * DX_PI / 3.0) + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pFrag->speed = 1.8 + GetRand(80) / 100.0;
                    pFrag->kind = img_enemyShotSmallBall[3 + GetRand(1)]; // 小玉
                    pFrag->param_i[1] = 1; // これ以上分裂しない
                    pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFrag->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                    pEnemyShotSet->pEnemyShotHead->prev = pFrag;
                }
                // 親弾は画面外へ飛ばして消えるのを待つ（速度を上げて外へ）
                pShot->speed = 8.0;
                pShot->muki = -DX_PI / 2.0; // 上方向へ逃がす
            }
        }
        else {
            // 分裂後の親弾もそのまま動かす
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 第3段階：吹雪の波（横から押し寄せる低速高密度）
static void ShotIceBlizzardWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int side = (pEnemyShotSet->kind % 2 == 0) ? -1 : 1; // 左右交互
        int num = 18 + GetRand(6);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = (side < 0) ? -30.0 : 510.0;
            pEnemyShot->y = 30.0 + i * (400.0 / num) + (GetRand(16) - 8);
            pEnemyShot->muki = (side < 0) ? 0.0 : DX_PI; // 右向き or 左向き
            // 少し上下に揺れる成分を加える
            pEnemyShot->muki += (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.4 + GetRand(50) / 100.0;
            pEnemyShot->margin = 40;
            // 白・シアン・青の小玉をランダム
            int col = 3 + GetRand(3); // 3,4,6
            if (col == 5) col = 6;
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // わずかに上下に揺らす（自然な吹雪感）
        if (pShot->count % 8 == 0) {
            pShot->muki += (GetRand(6) - 3) / 180.0 * DX_PI;
        }
        pShot = pShot->next;
    }
}

// クライマックス：巨大氷塊がゆっくり追尾 → 着弾で円形拡散
static void ShotIceChunk(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 巨大氷塊（大玉）を1〜2個
        int num = 1 + GetRand(1);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(120) - 60);
            pEnemyShot->y = pEnemyShotSet->y - 10.0;
            // プレイヤー方向へ緩やかに追尾する初期角度
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 1.1 + GetRand(40) / 100.0;
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青い大玉
            pEnemyShot->param_i[0] = 0; // 着弾フラグ
            pEnemyShot->param_d[0] = player.x; // 目標位置を記録（簡易追尾用）
            pEnemyShot->param_d[1] = player.y;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 簡易追尾（徐々にプレイヤー方向へ曲がる）
            double targetMuki = atan2(pShot->param_d[1] - pShot->y, pShot->param_d[0] - pShot->x);
            double diff = targetMuki - pShot->muki;
            // 角度差を -PI〜PI に正規化
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.03; // 緩やかに曲がる

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ある程度下まで来たら「着弾」扱い
            if (pShot->y > 380.0 || pShot->count > 180) {
                pShot->param_i[0] = 1;

                // 円形に破片を拡散
                int fragNum = 16 + GetRand(8);
                for (int k = 0; k < fragNum; k++) {
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pShot->x;
                    pFrag->y = pShot->y;
                    pFrag->muki = (k * 2.0 * DX_PI / fragNum) + (GetRand(15) - 7) / 180.0 * DX_PI;
                    pFrag->speed = 2.0 + GetRand(120) / 100.0;
                    int col = 3 + GetRand(3);
                    if (col == 5) col = 6;
                    pFrag->kind = (GetRand(2) == 0) ? img_enemyShotSmallBall[col] : img_enemyShotDiamond[col];
                    pFrag->param_i[0] = 1;
                    pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFrag->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                    pEnemyShotSet->pEnemyShotHead->prev = pFrag;
                }
                // 親の大玉は画面外へ
                pShot->speed = 6.0;
                pShot->muki = -DX_PI / 2.0;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// 敵本体パターン
// ---------------------------------------------------------------
void EnemyPat_EternalForceBlizzard_Grok()
{
    static int muki;
    static int phase;
    static int shotTimer;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        phase = 0;
        shotTimer = 0;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0) { enemy.x = 80.0; muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
        if (count % 150 == 75) muki *= -1;
    }

    // フェーズ進行（時間ベース）
    // 0〜240F : 第1段階（落下氷）
    // 241〜480F : 第2段階（結晶分裂）
    // 481〜720F : 第3段階（吹雪の波）
    // 721F〜    : クライマックス（巨大氷塊＋継続的な吹雪）
    if (count < 240) phase = 0;
    else if (count < 480) phase = 1;
    else if (count < 720) phase = 2;
    else phase = 3;

    shotTimer++;

    // ---- 弾幕セットの生成 ----
    // 第1段階：定期的な落下氷
    if (phase == 0 && shotTimer % 28 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIceFall;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 第2段階：放射結晶
    if (phase == 1 && shotTimer % 45 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIceCrystal;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 1;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 第3段階：吹雪の波（左右交互）
    if (phase >= 2 && shotTimer % 35 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIceBlizzardWave;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = shotTimer; // 偶奇で左右を決める
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // クライマックス：巨大氷塊
    if (phase == 3 && shotTimer % 90 == 1) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIceChunk;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 5.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 3;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // クライマックス中も軽い落下氷を継続して密度を維持
    if (phase == 3 && shotTimer % 22 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotIceFall;
        pSet->x = enemy.x + (GetRand(60) - 30);
        pSet->y = enemy.y + 15.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}