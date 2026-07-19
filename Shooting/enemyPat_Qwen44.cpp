// enemyPat_arrow.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 使用素材の選定理由
// ============================================================
// 1. img_enemyShotBullet (5.0 x 2.0)
//    細長い形状を活かし、矢の「軸」やV字の「翼」を表現するために使用。
//    色は赤(0)・橙(8)でグラデーションをつけ、白(6)・黄(1)でUI的な標識を表現。
// 2. img_enemyShotLaser (64.0 x 4.0)
//    矢軸を貫く「光線」として使用。シアン(3)や赤(0)で視認性を確保。
// 3. img_enemyShotScale (4.0 x 3.0)
//    軽やかな形状を活かし、矢の尾部「羽根」の扇状弾として使用。青(4)・緑(2)で彩りを添える。
// ============================================================

// ----------------------------------------------------------
// フェーズ1：V字の矢尻（Chevron）
// V字の形状を保ったまま、全体がプレイヤーをロックオンして誘導されてくる。
// ----------------------------------------------------------
static void ShotArrow_V(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音

        double base_muki = pEnemyShotSet->muki;
        pEnemyShotSet->param_d[0] = base_muki; // 基準角度を保存
        pEnemyShotSet->param_d[2] = pEnemyShotSet->x; // 中心座標X
        pEnemyShotSet->param_d[3] = pEnemyShotSet->y; // 中心座標Y

        // V字の翼を形成する弾を生成
        for (int i = 0; i < 6; i++) {
            double dist = 10.0 + i * 12.0;

            // 右翼
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + cos(base_muki + DX_PI / 6) * dist;
            pShot->y = pEnemyShotSet->y + sin(base_muki + DX_PI / 6) * dist;
            pShot->muki = base_muki + DX_PI / 6;
            pShot->speed = 2.2;
            pShot->kind = img_enemyShotBullet[0]; // 赤
            pShot->margin = 100;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            // 左翼
            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + cos(base_muki - DX_PI / 6) * dist;
            pShot->y = pEnemyShotSet->y + sin(base_muki - DX_PI / 6) * dist;
            pShot->muki = base_muki - DX_PI / 6;
            pShot->speed = 2.2;
            pShot->kind = img_enemyShotBullet[8]; // 橙
            pShot->margin = 100;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレームの誘導処理
    double cx = pEnemyShotSet->param_d[2];
    double cy = pEnemyShotSet->param_d[3];
    double target_muki = atan2(player.y - cy, player.x - cx);

    // 角度差分を計算し、-PI ～ PI に補正
    double diff = target_muki - pEnemyShotSet->param_d[0];
    while (diff > DX_PI) diff -= 2 * DX_PI;
    while (diff < -DX_PI) diff += 2 * DX_PI;

    // 基準角度をプレイヤーへゆっくり誘導
    pEnemyShotSet->param_d[0] += diff * 0.03;

    // 中心座標を移動
    pEnemyShotSet->param_d[2] += cos(pEnemyShotSet->param_d[0]) * 2.0;
    pEnemyShotSet->param_d[3] += sin(pEnemyShotSet->param_d[0]) * 2.0;

    // 各弾の位置と角度を再計算（V字の形状を維持）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    int i = 0;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double base_m = pEnemyShotSet->param_d[0] + DX_PI;
        double offset = (i % 2 == 0) ? DX_PI / 6 : -DX_PI / 6;
        double dist = 10.0 + (i / 2) * 12.0;

        pShot->x = pEnemyShotSet->param_d[2] + cos(base_m + offset) * dist;
        pShot->y = pEnemyShotSet->param_d[3] + sin(base_m + offset) * dist;
        pShot->muki = base_m + offset;

        pShot = pShot->next;
        i++;
    }
}

// ----------------------------------------------------------
// フェーズ2：矢軸（Shaft）を貫く光線
// V字の中心を貫くように、高速のレーザーが飛ぶ。
// ----------------------------------------------------------
static void ShotArrow_Shaft(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki; // プレイヤー狙い
        pShot->speed = 9.0; // 高速
        pShot->kind = img_enemyShotLaser[3]; // シアン

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------
// フェーズ3：矢羽（Fletching）の扇状弾
// 扇状に広がる鱗弾が、画面を埋め尽くしながら迫る。
// ----------------------------------------------------------
static void ShotArrow_Fletching(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        double base_muki = pEnemyShotSet->muki;

        for (int i = -4; i <= 4; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = base_muki + i * 0.12; // 扇状に展開
            pShot->speed = 3.2;
            pShot->kind = img_enemyShotScale[4]; // 青

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------
// フェーズ4：標識（Signpost）のポップアップ
// プレイヤーの近くに矢印アイコンが出現し、その方向から攻撃が飛ぶ。
// ----------------------------------------------------------
static void ShotSignpost(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 矢印の向きをランダムに決定
        double sign_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + DX_PI;
        pEnemyShotSet->param_d[0] = sign_muki;

        // 矢印の「軸」を配置 (速度0で静止)
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + cos(sign_muki) * (i * 12.0);
            pShot->y = pEnemyShotSet->y + sin(sign_muki) * (i * 12.0);
            pShot->muki = sign_muki;
            pShot->speed = 0.0; // 静止させる
            pShot->kind = img_enemyShotBullet[6]; // 白
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 矢印の「先端」を配置 (V字)
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + cos(sign_muki + DX_PI / 4) * (i * 12.0);
            pShot->y = pEnemyShotSet->y + sin(sign_muki + DX_PI / 4) * (i * 12.0);
            pShot->muki = sign_muki + DX_PI / 4;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotBullet[1]; // 黄
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + cos(sign_muki - DX_PI / 4) * (i * 12.0);
            pShot->y = pEnemyShotSet->y + sin(sign_muki - DX_PI / 4) * (i * 12.0);
            pShot->muki = sign_muki - DX_PI / 4;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotBullet[1]; // 黄
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 60フレーム経過したら、その方向へレーザーを放つ
    if (pEnemyShotSet->count == 60) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        double sign_muki = pEnemyShotSet->param_d[0];

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = sign_muki + DX_PI;
        pShot->speed = 12.0; // 超高速
        pShot->kind = img_enemyShotLaser[0]; // 赤

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 静止している弾の移動処理（speed=0なので実質何もしない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------
// 敵本体のパターン
// ----------------------------------------------------------
void EnemyPat_Arrow_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // フェーズが4つあるので耐久力を少し上げる
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 0) muki *= -1;
    }

    // 120フレームごとにフェーズが変化し、480フレームで1サイクル
    int current_phase = (count / 120) % 4;

    // 30フレームごとに弾幕を生成
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        switch (current_phase) {
        case 0:
            pEnemyShotSet->patternFunc = ShotArrow_V;
            break;
        case 1:
            pEnemyShotSet->patternFunc = ShotArrow_Shaft;
            break;
        case 2:
            pEnemyShotSet->patternFunc = ShotArrow_Fletching;
            break;
        case 3:
            pEnemyShotSet->patternFunc = ShotSignpost;
            while (true) {
                // 標識はプレイヤーの近くに表示させる
                pEnemyShotSet->x = player.x + GetRand(100) - 50;
                pEnemyShotSet->y = player.y + GetRand(100) - 50;
                // 画面内に収める
                if (pEnemyShotSet->x < 40) pEnemyShotSet->x = 40;
                if (pEnemyShotSet->x > 440) pEnemyShotSet->x = 440;
                if (pEnemyShotSet->y < 40) pEnemyShotSet->y = 40;
                if (pEnemyShotSet->y > 440) pEnemyShotSet->y = 440;

                double dx = pEnemyShotSet->x - player.x;
                double dy = pEnemyShotSet->y - player.y;
                if (hypot(dx, dy) > 10) break;
            }
            break;
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}