// enemyPat_Tmp.cpp
// スイカゲームモチーフ「果実融合弾幕（Fruit Merge Barrage）」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾のステージ定義
// 0: 種弾   (小玉・緑)
// 1: 幼果   (小玉・赤 or マゼンタ)
// 2: 中果   (中玉・橙 or 黄)
// 3: スイカ (大玉・緑) → 一定時間後に爆発
// ------------------------------------------------------------
static const int STAGE_SEED = 0;
static const int STAGE_SMALL = 1;
static const int STAGE_MEDIUM = 2;
static const int STAGE_LARGE = 3;

// おおよその半径（衝突判定用）
static const double RADIUS[4] = { 1.3, 1.3, 3.6, 10.5 };

// 色インデックス
// 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
static int GetKindByStage(int stage, int variant)
{
    switch (stage) {
    case STAGE_SEED:
        return img_enemyShotSmallBall[2];          // 緑・小
    case STAGE_SMALL:
        return (variant % 2 == 0) ? img_enemyShotSmallBall[0]   // 赤・小
            : img_enemyShotSmallBall[5];  // マゼンタ・小
    case STAGE_MEDIUM:
        return (variant % 2 == 0) ? img_enemyShotMediumBall[8]  // 橙・中
            : img_enemyShotMediumBall[1]; // 黄・中
    case STAGE_LARGE:
        return img_enemyShotLargeBall[2];          // 緑・大
    default:
        return img_enemyShotSmallBall[2];
    }
}

// 新しい弾をリストに追加する共通処理
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int stage)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = GetKindByStage(stage, GetRand(1));
    p->param_i[0] = stage;          // 現在のステージ
    p->param_i[1] = 0;              // 融合クールダウン
    p->param_i[2] = GetRand(1);     // 見た目のバリエーション
    p->param_d[0] = 0.0;            // 予備
    p->margin = 100;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾をリストから安全に削除
static void RemoveShot(sEnemyShot* p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
}

// ------------------------------------------------------------
// メイン弾幕パターン
// ------------------------------------------------------------
static void ShotFruitMerge(sEnemyShotSet* pEnemyShotSet)
{
    // ---- 種弾の定期出現 ----
    // 序盤は少なめ、時間経過で少し増やす
    int spawnInterval = 28;
    if (pEnemyShotSet->count > 600) spawnInterval = 20;
    if (pEnemyShotSet->count > 1100) spawnInterval = 5;

    if (pEnemyShotSet->count % spawnInterval == 0) {
        int num = 20 + GetRand(20); // 2〜4個
        for (int i = 0; i < num; i++) {
            double sx = 40.0 + GetRand(400);          // 画面上部付近
            double sy = -10.0 - GetRand(30);
            double smuki = DX_PI * 0.5 + (GetRand(60) - 30) / 180.0 * DX_PI; // ほぼ真下
            double sspeed = 1.2 + GetRand(80) / 100.0;

            AddShot(pEnemyShotSet, sx, sy, smuki, sspeed, STAGE_SEED);
        }
        // 軽めの発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ---- 移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // わずかに下方向へ加速（落ちてくる感じ）
        if (pShot->param_i[0] <= STAGE_MEDIUM) {
            pShot->speed += 0.008;
            if (pShot->speed > 3.5) pShot->speed = 3.5;
        }

        // 融合クールダウン減少
        if (pShot->param_i[1] > 0) pShot->param_i[1]--;

        pShot = pShot->next;
    }

    // ---- 融合判定 ----
    // 同じステージ同士が近づいたら融合して1段階大きくする
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] > 0) { // クールダウン中はスキップ
            pShot = pShot->next;
            continue;
        }

        sEnemyShot* pOther = pShot->next;
        bool merged = false;

        while (pOther != pEnemyShotSet->pEnemyShotHead) {
            if (pOther->param_i[1] > 0 || pOther->param_i[0] != pShot->param_i[0]) {
                pOther = pOther->next;
                continue;
            }

            double dx = pOther->x - pShot->x;
            double dy = pOther->y - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            double rsum = RADIUS[pShot->param_i[0]] + RADIUS[pOther->param_i[0]];

            if (dist < rsum * 0.95) {
                // 融合成功
                int nextStage = pShot->param_i[0] + 1;
                if (nextStage > STAGE_LARGE) nextStage = STAGE_LARGE;

                // pShot を成長させる
                pShot->param_i[0] = nextStage;
                pShot->param_i[1] = 12; // 少しクールダウン
                pShot->param_i[2] = GetRand(1);
                pShot->kind = GetKindByStage(nextStage, pShot->param_i[2]);

                // 少し速度を落とす（重くなった感じ）
                pShot->speed *= 0.75;
                if (pShot->speed < 0.8) pShot->speed = 0.8;

                // 中心を少し寄せる
                pShot->x = (pShot->x + pOther->x) * 0.5;
                pShot->y = (pShot->y + pOther->y) * 0.5;

                // 効果音
                if (nextStage >= STAGE_LARGE) {
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
                else {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                // pOther を削除
                sEnemyShot* toDelete = pOther;
                pOther = pOther->next;
                RemoveShot(toDelete);
                merged = true;
                break;
            }
            pOther = pOther->next;
        }

        if (!merged) pShot = pShot->next;
        // merged の場合は pShot をそのまま次へ（成長した弾を再チェックしない）
        else pShot = pShot->next;
    }

    // ---- 巨大スイカの爆発処理 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == STAGE_LARGE) {
            // 出現から一定フレーム経過で爆発
            // param_i[3] を爆発タイマーとして使用
            if (pShot->param_i[3] == 0) {
                pShot->param_i[3] = 1; // 初期化フラグ
                pShot->param_i[4] = 90 + GetRand(40); // 爆発までの残りフレーム
            }

            pShot->param_i[4]--;

            // 予兆：残り少ないと少し揺れる
            if (pShot->param_i[4] < 30) {
                pShot->x += (GetRand(4) - 2) * 0.4;
                pShot->y += (GetRand(4) - 2) * 0.4;
            }

            if (pShot->param_i[4] <= 0) {
                // 爆発！
                double cx = pShot->x;
                double cy = pShot->y;

                // 大量の種弾 + いくつかの中果を放射
                int seedNum = 14 + GetRand(8);
                for (int i = 0; i < seedNum; i++) {
                    double angle = (i * 360.0 / seedNum + GetRand(20) - 10) / 180.0 * DX_PI;
                    double spd = 1.8 + GetRand(120) / 100.0;
                    AddShot(pEnemyShotSet, cx, cy, angle, spd, STAGE_SEED);
                }

                int midNum = 3 + GetRand(3);
                for (int i = 0; i < midNum; i++) {
                    double angle = GetRand(360) / 180.0 * DX_PI;
                    double spd = 1.0 + GetRand(80) / 100.0;
                    AddShot(pEnemyShotSet, cx, cy, angle, spd, STAGE_MEDIUM);
                }

                // 爆発音
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                // 自分を消す
                sEnemyShot* toDelete = pShot;
                pShot = pShot->next;
                RemoveShot(toDelete);
                continue;
            }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_SuikaGame_Grok()
{
    static int muki;
    static bool shotSetCreated;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotSetCreated = false;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 最初の1回だけ融合弾幕セットを生成（永続的に管理）
    if (!shotSetCreated && count >= 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFruitMerge;
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

        shotSetCreated = true;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}