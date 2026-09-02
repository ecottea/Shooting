// enemyPat_Tmp.cpp
// 「審判の宣告 - 無効な抵抗」実装パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ばら撒き
static void ShotScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 9; i++) {
            pEnemyShot = new sEnemyShot;

            // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意！
            // リプレイファイルにseed値を保存するので再現性あり。
            pEnemyShot->x = pEnemyShotSet->x + GetRand(480) - 240;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            pEnemyShot->speed = (200 + GetRand(200)) / 100.0;

            // 弾の種類一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            // 実際には全て使うのではなく必要なものを抜粋して使用すること。
            switch (pEnemyShotSet->kind % 8) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[i];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[i];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotLargeBall[i];
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotBullet[i];
                break;
            case 4:
                pEnemyShot->kind = img_enemyShotScale[i];
                break;
            case 5:
                pEnemyShot->kind = img_enemyShotDiamond[i];
                break;
            case 6:
                pEnemyShot->kind = img_enemyShotMediumOval[i];
                break;
            case 7:
                pEnemyShot->kind = img_enemyShotLaser[i];
                break;
            }

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

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 補助関数：自機弾を自機方向へ強制移動させる（ハッキング演出）
// ---------------------------------------------------------
static void HackPlayerShots()
{
    sPlayerShot* pPShot = playerShotHead.next;
    while (pPShot != &playerShotHead) {
        double dx = player.x - pPShot->x;
        double dy = player.y - pPShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist > 1.0) {
            // 自機に向かって高速で戻ってくる
            pPShot->x += (dx / dist) * 4.5;
            pPShot->y += (dy / dist) * 4.5;
        }
        pPShot = pPShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕：審判の放射（自機中心から自機弾風の色・形状で放射）
// ---------------------------------------------------------
static void ShotJudgment(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ：警告音と弾生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 360度放射状に「自機のショットを模した白弾」を展開
        int shotNum = 36;
        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 自機の現在座標から発射
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 放射状の向き
            pEnemyShot->muki = (DX_PI * 2.0 / shotNum) * i;

            // 徐々に加速する演出のため、初期速度は抑えめにし、param_d[0]に加速度を記録
            pEnemyShot->speed = 1.5;
            pEnemyShot->param_d[0] = 0.15; // 毎フレームの加速度

            // 自機弾を模すため「白(6)」の「銃弾」または「小玉」を使用
            // ここでは「白：銃弾」を使用して自機ショット感を演出
            pEnemyShot->kind = img_enemyShotBullet[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理（加速付き）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->speed += pShot->param_d[0]; // 加速
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕：警告の円環（反則発動前の予告）
// ---------------------------------------------------------
static void ShotWarning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 敵の周囲に赤い大玉を配置して警告を表現
        int shotNum = 16;
        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (DX_PI * 2.0 / shotNum) * i;
            pEnemyShot->speed = 0.0; // 最初は停止

            // 赤(0)の大玉
            pEnemyShot->kind = img_enemyShotLargeBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 警告弾をゆっくりと自機方向へ収縮させる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 自機方向へのベクトルを計算
        double dx = player.x - pShot->x;
        double dy = player.y - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist > 1.0) {
            pShot->x += (dx / dist) * 1.2;
            pShot->y += (dy / dist) * 1.2;
        }

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン：審判の宣告 - 無効な抵抗
// ---------------------------------------------------------
void EnemyPat_Violate_Qwen()
{
    static int phase = 0;          // 0:通常, 1:警告, 2:反則発動, 3:審判の弾幕, 4:解除
    static int phase_count = 0;    // 各フェーズ内のローカルカウンタ

    // パターン開始時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = 300;
        enemy.hp = 300;
        phase = 0;
        phase_count = 0;
    }

    phase_count++;

    // =========================================================
    // フェーズ管理
    // =========================================================

    // --- Phase 0: 通常攻撃 (0 ~ 120フレーム) ---
    if (phase == 0) {
        // 敵の移動：左右に揺れる
        enemy.x = 240.0 + sin(phase_count * 0.05) * 120.0;

        // 通常のばら撒き弾
        if (phase_count % 15 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotScatter; // enemyPat_sampleForAI.cpp内の関数を流用
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = phase_count; // 色などを少しずつ変える

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        if (phase_count >= 120) {
            phase = 1;
            phase_count = 0;
        }
    }
    // --- Phase 1: 警告 (120 ~ 180フレーム, 約1秒) ---
    else if (phase == 1) {
        // 敵は停止
        enemy.x += (240.0 - enemy.x) * 0.1;
        enemy.y += (40.0 - enemy.y) * 0.1;

        // 警告エフェクト弾の生成（初回のみ）
        if (phase_count == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotWarning;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        if (phase_count >= 60) {
            phase = 2;
            phase_count = 0;
        }
    }
    // --- Phase 2: 反則発動・ハッキング (180 ~ 300フレーム) ---
    else if (phase == 2) {
        // 【反則演出 1】自機を画面中央に強制移動（速度低下・操作不能感）
        player.x += (240.0 - player.x) * 0.15;
        player.y += (240.0 - player.y) * 0.15;

        // 【反則演出 2】自機ショットを自機方向へUターンさせる
        HackPlayerShots();

        // 【反則演出 3】敵機を無敵状態にする（HP減少を無効化）
        enemy.hp = enemy.maxHp;

        if (phase_count >= 120) {
            phase = 3;
            phase_count = 0;
        }
    }
    // --- Phase 3: 審判の弾幕 (300 ~ 480フレーム) ---
    else if (phase == 3) {
        // 自機の強制移動とハッキングは継続
        player.x += (240.0 - player.x) * 0.2;
        player.y += (240.0 - player.y) * 0.2;
        HackPlayerShots();
        enemy.hp = enemy.maxHp;

        // 自機座標を中心とした「審判の弾幕」を定期的に生成
        if (phase_count % 20 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotJudgment;
            // 自機の現在座標を発射源にする（自機の弾が返ってきている演出）
            pEnemyShotSet->x = player.x;
            pEnemyShotSet->y = player.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 6; // 白

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        if (phase_count >= 180) {
            phase = 4;
            phase_count = 0;
        }
    }
    // --- Phase 4: 解除・通常復帰 (480フレーム以降) ---
    else if (phase == 4) {
        // 自機の強制移動を解除（プレイヤーの通常入力に任せるため、ここでは何もしない）
        // 敵の無敵を解除
        // (メインルーチンでのダメージ計算が通常通り行われるようになる)

        // フェーズ0に戻ってループ、またはパターン終了へ
        if (phase_count >= 60) {
            phase = 0;
            phase_count = 0;
            // count をリセットするかはメインルーチンの仕様に依存するが、
            // ここではパターンを再始動させるため phase だけリセットする
        }
    }
}