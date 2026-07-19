// enemyPat_bubble.cpp
// ============================================================
// シャボン玉弾幕パターン「虹色シャボン」
//
// 使用素材:
//   img_enemyShotLargeBall[0-5]  : 親泡（虹色: 赤/黄/緑/シアン/青/マゼンタ）
//   img_enemyShotSmallBall[0-5]  : 破裂・外リング（虹色）
//   img_enemyShotSmallBall[6]    : 破裂・狙い弾（白）
//   sound_enemyShot_light        : 泡生成SE
//   sound_enemyShot_medium       : 破裂SE
//
// 構造:
//   EnemyPat_SoapBubbles_Claude
//     └─ ShotBubbleFloat（親泡グループ）× 定期生成
//           └─ SpawnBurstShotSet → ShotBubbleBurst（飛沫）× 各泡が時間差で破裂
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ─────────────────────────────────────────────────
// 前方宣言
// ─────────────────────────────────────────────────
static void SpawnBurstShotSet(double x, double y);

// ============================================================
// 弾幕：破裂（飛沫）
//   外リング 8 発（虹色・小玉） ＋ 狙い 4 発（白・小玉）
// ============================================================
static void ShotBubbleBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 外リング：8 発 全方位（虹色・小玉）
        for (int i = 0; i < 8; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->kind = img_enemyShotSmallBall[i % 6]; // 虹色サイクル
            p->muki = 2.0 * DX_PI * i / 8.0;
            p->speed = 1.8;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 狙い弾：プレイヤー方向 4 発（白・小玉）/ 15°刻みで扇状
        double aim = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        for (int i = 0; i < 4; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->kind = img_enemyShotSmallBall[6]; // 白
            p->muki = aim + (i - 1.5) * (DX_PI / 12.0);
            p->speed = 3.0;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 飛沫弾を直進
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
// ヘルパー：破裂 sEnemyShotSet を生成してリストに追加
// ============================================================
static void SpawnBurstShotSet(double x, double y)
{
    sEnemyShotSet* pNew = new sEnemyShotSet;
    pNew->count = 0;
    pNew->patternFunc = ShotBubbleBurst;
    pNew->x = x;
    pNew->y = y;

    pNew->pEnemyShotHead = new sEnemyShot;
    pNew->pEnemyShotHead->prev = pNew->pEnemyShotHead;
    pNew->pEnemyShotHead->next = pNew->pEnemyShotHead;

    pNew->prev = enemyShotSetHead.prev;
    pNew->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pNew;
    enemyShotSetHead.prev = pNew;
}

// ============================================================
// 弾幕：親泡（漂い → タイマーで破裂）
//
// 各 sEnemyShot の param_d:
//   [0] : 破裂タイマー（フレーム数、カウントダウン）
//   [1] : サイン波位相オフセット（泡ごとに均等ずれ）
//   [2] : フェーズ（0.0=漂い中 / 1.0=破裂済み・以降未使用）
//   [3] : 突風による横速度蓄積（毎フレーム 0.97 倍で減衰）
// ============================================================
static void ShotBubbleFloat(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM_BUBBLES = 6;

    // ── 生成（count==0 の 1 回のみ） ──
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM_BUBBLES; i++) {
            sEnemyShot* p = new sEnemyShot;
            // 敵位置から横に最大 ±40px ばらけて出現
            p->x = pEnemyShotSet->x + (GetRand(80) - 40);
            p->y = pEnemyShotSet->y;
            // 虹色（赤[0]〜マゼンタ[5]を 1 個ずつ）大玉
            p->kind = img_enemyShotLargeBall[i % 6];
            // 横動きはサイン波と突風が担うので muki は真下のみ
            p->muki = DX_PI / 2.0;                        // 真下
            p->speed = (3 + GetRand(5)) / 10.0;            // 0.3〜0.8 px/f

            p->param_d[0] = 80.0 + GetRand(60);            // 破裂タイマー 80〜140f
            p->param_d[1] = 2.0 * DX_PI * i / NUM_BUBBLES; // サイン位相（均等分割）
            p->param_d[2] = 0.0;                            // 漂い中
            p->param_d[3] = 0.0;                            // 突風速度初期値

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ── 突風キック（このShotSetの 60f ごとに全泡へ同時に加える）──
    double gustVx = 0.0;
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 60 == 0) {
        gustVx = (GetRand(1) == 0 ? 1.0 : -1.0) * 2.5;
    }

    // ── 全泡の更新 ──
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next; // 削除に備えて先に保存

        // 突風速度を蓄積し、毎フレーム減衰させる
        p->param_d[3] += gustVx;
        p->param_d[3] *= 0.97;

        // サイン波による横揺れ
        // ※ グローバルの count でなく pEnemyShotSet->count を使い
        //   ワープ（不連続ジャンプ）を防止する
        double sineVx = 1.0 * sin(pEnemyShotSet->count * 0.05 + p->param_d[1]);

        // 位置更新
        p->x += sineVx + p->param_d[3]; // 横：サイン波 ＋ 突風
        p->y += p->speed;               // 縦：真下にゆっくり漂う

        // 破裂タイマー
        p->param_d[0]--;
        if (p->param_d[0] <= 0.0) {
            SpawnBurstShotSet(p->x, p->y); // 破裂：飛沫セットを生成

            // リストから切り離してプールに返す
            p->prev->next = pNext;
            pNext->prev = p->prev;
            delete p;
        }

        p = pNext;
    }
}

// ============================================================
// 敵本体  EnemyPat_SoapBubbles_Claude
//
// フェーズ:
//   count    1〜300 : 80f 間隔で親泡セット生成
//   count 301〜     : 50f 間隔（泡の密度 UP）
// ============================================================
void EnemyPat_SoapBubbles_Claude()
{
    static int moveDir;
    static int spawnInterval;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
        spawnInterval = 60;
    }
    else {
        // 左右往復（150f ごとに反転）
        enemy.x += 0.7 * moveDir;
        if (count % 150 == 75) moveDir *= -1;
    }

    // Phase 3：300f 以降は間隔短縮で泡の密度を上げる
    spawnInterval = max(60 - count / 10, 7);

    // 親泡セットを定期生成（count % spawnInterval == 1 は count=1 にも合致）
    if (count % spawnInterval == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBubbleFloat;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}