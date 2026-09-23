#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// メントスコーラ弾幕「噴出泡沫（Fountain Foam）」
// 使える弾種:
//   小玉(2.5x2.5)  img_enemyShotSmallBall[i]
//   中玉(7.0x7.0)  img_enemyShotMediumBall[i]
//   大玉(20.0x20.0) img_enemyShotLargeBall[i]
//   銃弾(5.0x2.0)  img_enemyShotBullet[i]
//   鱗弾(4.0x3.0)  img_enemyShotScale[i]
//   菱形弾(4.5x2.5) img_enemyShotDiamond[i]
//   中楕円弾(10.5x7.0) img_enemyShotMediumOval[i]
//   短レーザー(64.0x4.0) img_enemyShotLaser[i]
// 色: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙
// 効果音: sound_enemyShot_light / medium / heavy / extreme, sound_enemyCharge
// ============================================================

// メントスコーラ噴出パターン本体
static void ShotMentosCola(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double PI = DX_PI;

    // -------------------------------------------------------
    // フェーズ1: チャージ（溜め） count 0〜39
    // 小さめの泡（小玉・白/シアン）を円状にほぼ静止させて配置
    // -------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 12発の小玉を円状に配置（速度0で溜め）
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (i / 12.0) * 2.0 * PI;
            pEnemyShot->x = pEnemyShotSet->x + 28.0 * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + 28.0 * sin(ang);
            pEnemyShot->muki = ang;          // 後で外側へ飛ばす用
            pEnemyShot->speed = 0.0;         // 静止
            // 白とシアンを交互
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[3];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // フェーズ2: 爆発の瞬間 count == 45
    // 溜め弾を外側へ高速で弾き飛ばし、中玉放射 + 上方向レーザー
    // -------------------------------------------------------
    if (pEnemyShotSet->count == 45) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 既存の溜め弾に速度を与えて外側へ弾き飛ばす
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->speed == 0.0) {
                pShot->speed = 4.5 + GetRand(15) / 10.0;  // 4.5〜6.0
            }
            pShot = pShot->next;
        }

        // 中玉を放射状に16方向高速発射（メイン衝撃波）
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (i / 16.0) * 2.0 * PI;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 5.5 + GetRand(20) / 10.0;  // 5.5〜7.5
            // 白・橙・シアンを混ぜる
            int col = (i % 3 == 0) ? 6 : (i % 3 == 1) ? 8 : 3;
            pEnemyShot->kind = img_enemyShotMediumBall[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 上方向に短レーザーを3本（泡の筋）
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 1) * 18.0;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = -PI / 2.0 + (GetRand(20) - 10) / 180.0 * PI;  // ほぼ真上
            pEnemyShot->speed = 7.0 + GetRand(20) / 10.0;
            pEnemyShot->kind = img_enemyShotLaser[6];  // 白
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // フェーズ3: 主噴出 count 50〜180
    // 上方向バイアスの小玉を連続噴出 + 時々中玉・中楕円
    // -------------------------------------------------------
    if (pEnemyShotSet->count >= 50 && pEnemyShotSet->count <= 180) {
        // 毎フレームではなく2フレームに1回程度で密度調整
        if (pEnemyShotSet->count % 2 == 0) {
            // 小玉をランダム角度で噴出（真上寄りにバイアス）
            int num = 2 + GetRand(2);  // 2〜4発
            for (int i = 0; i < num; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
                pEnemyShot->y = pEnemyShotSet->y + GetRand(10) - 5;
                // 角度: -150°〜-30° の範囲に寄せる（上方向中心）
                double base = -PI / 2.0;
                double spread = (GetRand(120) - 60) / 180.0 * PI;  // ±60°
                pEnemyShot->muki = base + spread;
                pEnemyShot->speed = 2.8 + GetRand(25) / 10.0;  // 2.8〜5.3
                // 白・シアン・黄をランダム
                int col = GetRand(2);  // 0,1,2 → 白/シアン/黄
                if (col == 0) pEnemyShot->kind = img_enemyShotSmallBall[6];
                else if (col == 1) pEnemyShot->kind = img_enemyShotSmallBall[3];
                else pEnemyShot->kind = img_enemyShotSmallBall[1];
                pEnemyShot->param_i[0] = 1;
                pEnemyShot->param_d[0] = 0.0;
                pEnemyShot->margin = 480;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 8フレームに1回、中玉を扇状に低速でばら撒く
        if (pEnemyShotSet->count % 8 == 0) {
            for (int i = 0; i < 3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                double ang = -PI / 2.0 + (i - 1) * 0.45 + (GetRand(20) - 10) / 180.0 * PI;
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.8 + GetRand(15) / 10.0;  // 低速
                pEnemyShot->kind = img_enemyShotMediumBall[6];  // 白
                pEnemyShot->param_i[0] = 1;
                pEnemyShot->param_d[0] = 0.0;
                pEnemyShot->margin = 480;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 20フレームに1回、中楕円弾をアクセントで追加
        if (pEnemyShotSet->count % 20 == 5) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(16) - 8;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = -PI / 2.0 + (GetRand(40) - 20) / 180.0 * PI;
            pEnemyShot->speed = 3.2 + GetRand(10) / 10.0;
            pEnemyShot->kind = img_enemyShotMediumOval[3];  // シアン
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // たまに細長い弾（鱗弾）を縦に飛ばして泡の筋を表現
        if (pEnemyShotSet->count % 15 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(30) - 15;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = -PI / 2.0 + (GetRand(30) - 15) / 180.0 * PI;
            pEnemyShot->speed = 5.0 + GetRand(20) / 10.0;
            pEnemyShot->kind = img_enemyShotScale[6];  // 白
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // フェーズ4: 余韻 count 181〜220
    // 残った小玉をゆっくり下方向へ落とす感じを追加
    // -------------------------------------------------------
    if (pEnemyShotSet->count >= 181 && pEnemyShotSet->count <= 220) {
        if (pEnemyShotSet->count % 6 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20);
            pEnemyShot->muki = PI / 2.0 + (GetRand(40) - 20) / 180.0 * PI;  // 下方向
            pEnemyShot->speed = 1.2 + GetRand(10) / 10.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // 全弾の移動（毎フレーム）
    // -------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        if (pShot->param_i[0] == 1) {
            pShot->y += pShot->param_d[0];
            pShot->param_d[0] += 0.015;
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_MentosCola_Grok()
{
    static int muki;
    static int shot_phase;  // 次の噴出までの間隔管理用

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;  // 200で固定
        muki = 1;
        shot_phase = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 約4秒に1回（240フレーム）メントスコーラ噴出を発動
    // 最初は count==60 で開始し、以降は間隔を空ける
    if (count == 60 || (count > 60 && (count - 60) % 240 == 0)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMentosCola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 460;
        pEnemyShotSet->muki = 0.0;  // このパターンではあまり使わない
        pEnemyShotSet->kind = shot_phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}