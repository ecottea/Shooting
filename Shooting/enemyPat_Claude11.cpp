// enemyPat_tmp.cpp
// テーマ：はかいこうせん（破壊光線）
//
// 弾幕構成
//   ShotBeamCharge   収束チャージ粒子（ビーム警告演出）
//   ShotBeamLaser    破壊光線メインビーム
//   ShotWave         幕間うねり拡散弾
//
// 敵パターン EnemyPat_Hakaikousen_Claude（200フレーム1サイクル）
//   cyc=  0  ShotWave（幕間①）
//   cyc= 50  ShotWave（幕間②）
//   cyc=100  ShotBeamCharge（チャージ演出、48フレーム）
//   cyc=160  ShotBeamLaser（破壊光線本体、60フレーム＋余波バースト）
//
// 使用素材
//   img_enemyShotSmallBall[3]   シアン小玉  ← チャージ粒子
//   img_enemyShotLargeBall[6]   白大玉      ← ビームコア
//   img_enemyShotMediumBall[4]  青中玉      ← ビームサイド
//   img_enemyShotDiamond[3]     シアン菱形  ← エネルギー散乱
//   img_enemyShotScale[5]       マゼンタ鱗弾← 余波バースト
//   img_enemyShotBullet[4]      青銃弾      ← 幕間ウェーブ

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  収束チャージ粒子
//
//  シアン小玉が 8方向から「らせん回転しながら中心へ収束」する。
//  中心を通り抜けてそのまま画面外へ飛び去るため、
//  プレイヤーへの警告と同時に実際の脅威にもなる。
//
//  count 0〜47 の間、4フレームに1度 8方向から粒子を放出。
//  各ウェーブは少しずつ回転角をずらして螺旋状に見せる。
// ============================================================
static void ShotBeamCharge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0)
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

    if (pEnemyShotSet->count < 48 && pEnemyShotSet->count % 4 == 0) {
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            // ウェーブごとに少しずつ回転 → 螺旋収束に見える
            double angle = 2.0 * DX_PI * i / 8.0
                + pEnemyShotSet->count * 0.08;
            pEnemyShot->x = pEnemyShotSet->x + 100.0 * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + 100.0 * sin(angle);
            pEnemyShot->muki = atan2(pEnemyShotSet->y - pEnemyShot->y,
                pEnemyShotSet->x - pEnemyShot->x);
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotSmallBall[3];  // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  破壊光線メインビーム
//
//  count 0〜59  毎フレーム:
//    ・白大玉  ×1  （コアビーム、±3°ブレ、speed 7.5〜8.5）
//    ・青中玉  ×2  （サイドビーム、コアの左右±12°、speed 5.5〜7.5）
//    5フレーム毎:
//    ・シアン菱形 ×3（エネルギー散乱、±40°、speed 1.5〜4.5）
//
//  count == 60（ビーム終端）:
//    ・マゼンタ鱗弾 ×16（余波リングバースト、全方向均等）
//
//  弾数合計（1発動あたり）:
//    コア 60 ＋ サイド 120 ＋ 散乱 36 ＋ バースト 16 ＝ 232発
// ============================================================
static void ShotBeamLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0)
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

    if (pEnemyShotSet->count < 60) {

        // ── コアビーム（白大玉、極狭スプレッド） ──────────────────
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + GetRand(4) - 2;
        pEnemyShot->y = pEnemyShotSet->y + GetRand(4) - 2;
        pEnemyShot->muki = pEnemyShotSet->muki
            + (GetRand(6) - 3) / 180.0 * DX_PI;   // ±3°
        pEnemyShot->speed = 7.5 + GetRand(10) / 10.0;            // 7.5〜8.5
        pEnemyShot->kind = img_enemyShotLargeBall[6];            // 白

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // ── サイドビーム（青中玉、コア左右±12°） ──────────────────
        for (int side = -1; side <= 1; side += 2) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki
                + side * 12.0 / 180.0 * DX_PI
                + (GetRand(8) - 4) / 180.0 * DX_PI; // ±4°ゆらぎ
            pEnemyShot->speed = 5.5 + GetRand(20) / 10.0;          // 5.5〜7.5
            pEnemyShot->kind = img_enemyShotMediumBall[4];          // 青

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ── エネルギー散乱（シアン菱形、5フレーム毎、広角スプレッド） ──
        if (pEnemyShotSet->count % 3 == 0) {
            for (int i = 0; i < 3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + GetRand(16) - 8;
                pEnemyShot->y = pEnemyShotSet->y + GetRand(16) - 8;
                pEnemyShot->muki = pEnemyShotSet->muki
                    + (GetRand(80) - 40) / 180.0 * DX_PI; // ±40°
                pEnemyShot->speed = 1.5 + GetRand(30) / 10.0;             // 1.5〜4.5
                pEnemyShot->kind = img_enemyShotDiamond[3];               // シアン

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ── 余波リングバースト（マゼンタ鱗弾 ×16、全方向均等） ──────────
    if (pEnemyShotSet->count == 60) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int N = 32;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 2.0 * DX_PI * i / N;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotScale[5];  // マゼンタ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  幕間うねり拡散弾
//
//  プレイヤー方向へ 5-way 扇状に発射した青銃弾を、
//  sin 波で左右にゆらしながら進ませる。
//  ShotSet 作成時（count == 0）に全弾をスポーンし、
//  以降は移動のみ行う。
// ============================================================
static void ShotWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        const int N = 5;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - N / 2) * 0.18; // ±2段の扇
            pEnemyShot->speed = 2.5 + GetRand(20) / 10.0;              // 2.5〜4.5
            pEnemyShot->kind = img_enemyShotBullet[4];                 // 青銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // sin 波で向きをゆらしながら移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->muki += 0.04 * cos(pEnemyShotSet->count * 0.12);
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体：はかいこうせん（破壊光線）ボス
//
//  移動: 左右往来（x: 70〜410、speed 0.5）
//
//  攻撃サイクル（200フレーム周期）:
//    cyc=  0  ShotWave（幕間①）
//    cyc= 50  ShotWave（幕間②）
//    cyc=100  ShotBeamCharge（チャージ演出開始）
//    cyc=160  ShotBeamLaser（破壊光線発射）
//
//  タイムライン（60fps 基準）:
//    0.0s  ウェーブ①
//    0.8s  ウェーブ②
//    1.7s  チャージ開始 → 0.8s で収束完了
//    2.7s  破壊光線発射 → 1.0s でビーム終了＋余波バースト
//    3.3s  次サイクル（ウェーブ①）
// ============================================================
void EnemyPat_Hakaikousen_Claude()
{
    static double velX;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        velX = 0.5;
        return;
    }

    // 左右往来
    enemy.x += velX;
    if (enemy.x > 410.0 || enemy.x < 70.0) velX *= -1.0;

    const double ey = enemy.y + 10.0;
    const int    cyc = (count - 2) % 200;  // count==2 から cyc==0 スタート

    // ShotSet を生成してリストへ追加するラムダ
    auto spawn = [&](void(*func)(sEnemyShotSet*), double muki) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = func;
        pSet->x = enemy.x;
        pSet->y = ey;
        pSet->muki = muki;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    };

    const double playerMuki = atan2(player.y - ey, player.x - enemy.x);

    if (cyc == 0) spawn(ShotWave, playerMuki);  // 幕間①
    if (cyc == 50) spawn(ShotWave, playerMuki);  // 幕間②
    if (cyc == 100) spawn(ShotBeamCharge, 0.0);          // チャージ演出
    if (cyc == 160) spawn(ShotBeamLaser, playerMuki);  // 破壊光線
}