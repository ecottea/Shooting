// ============================================================
// 弾幕：線香花火「西の花」
// 素材選定理由：
//   小玉(2.5x2.5) ... 火花本体。最も小さく火花っぽい
//   中玉(7.0x7.0) ... 火球本体。橙で線香の玉を表現
//   鱗弾(4.0x3.0) ... 米粒状で火の粉の形に一番近い
// 色選定：
//   8:橙 = 火球、1:黄 = パチパチ火花、6:白 = 細かい火の粉
// ============================================================

#include "gv.h"
#include <DxLib.h>
#include <math.h>

// DxLib外部参照（本体側で定義されている前提）
extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;
extern int sound_enemyShot_heavy;
extern int sound_enemyShot_extreme;
extern int sound_enemyCharge;

extern int img_enemyShotSmallBall[];
extern int img_enemyShotMediumBall[];
extern int img_enemyShotLargeBall[];
extern int img_enemyShotBullet[];
extern int img_enemyShotScale[];
extern int img_enemyShotDiamond[];
extern int img_enemyShotMediumOval[];
extern int img_enemyShotLaser[];

// --------------------------
// Phase2: パチパチ弾
// 中央から5wayで出て急減速 → 35F後に白い鱗弾3発に分裂
// --------------------------
static void ShotSenko_Pachipachi(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 軽いパチパチ音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int baseColor = (GetRand(1) == 0) ? 1 : 8; // 黄 or 橙ランダム
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機方向を中心に±60度バラける。GetRandは0..x inclusiveなので注意
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            // 初速は速く、すぐ減速させる
            pEnemyShot->speed = 3.5 + GetRand(150) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[baseColor];
            pEnemyShot->count = 0;
            pEnemyShot->param_i[0] = 0; // 分裂済みフラグ
            pEnemyShot->margin = 30.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 追加でリストが伸びても安全なように先に保存

        // 減速してその場でチカチカ滞空する動き
        if (pShot->count < 20) {
            pShot->speed *= 0.88;
        }
        else {
            pShot->speed *= 0.97;
            if (pShot->speed < 0.2*1.5) pShot->speed = 0.2*1.5;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // 少し重力で垂れる
        pShot->y += 0.02;

        // 35F後に火の粉（白鱗弾）に分裂
        if (pShot->count == 35 && pShot->param_i[0] == 0) {
            pShot->param_i[0] = 1;
            for (int j = 0; j < 3; j++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->muki = pShot->muki + (GetRand(80) - 40) / 180.0 * DX_PI;
                pNew->speed = 0.4 + GetRand(80) / 100.0;
                pNew->kind = img_enemyShotScale[6]; // 白・鱗弾 = 火の粉
                pNew->count = 0;
                pNew->margin = 30.0;
                pNew->param_i[0] = 1;

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }

        pShot = pNext;
    }
}

// --------------------------
// Phase3: 松葉弾（派手な割れ）
// 12方向に出て0.5秒後に4方向へ枝分かれ
// --------------------------
static void ShotSenko_Matsuba(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 12方向均等 + 少しランダム
            pEnemyShot->muki = (2.0 * DX_PI / 12.0) * i + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.8 + GetRand(50) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙小玉
            pEnemyShot->count = 0;
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->margin = 30.0;
            // 元の角度を保存（枝分かれの基準に）
            pEnemyShot->param_d[0] = pEnemyShot->muki;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 28Fで松葉状に4分裂
        if (pShot->count == 28 && pShot->param_i[0] == 0) {
            pShot->param_i[0] = 1;
            for (int k = 0; k < 4; k++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                // 元角度から90度ずつ + 少し散らす
                pNew->muki = pShot->param_d[0] + (DX_PI / 2.0) * k + (GetRand(30) - 15) / 180.0 * DX_PI;
                pNew->speed = 1.0 + GetRand(80) / 100.0;
                pNew->kind = img_enemyShotScale[6]; // 白鱗弾
                pNew->count = 0;
                pNew->margin = 30.0;
                pNew->param_i[0] = 1;

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }

        pShot = pNext;
    }
}

// --------------------------
// Phase4: 落下する火球
// 中玉橙が重力落下、軌跡に火の粉をこぼす
// --------------------------
static void ShotSenko_Fall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 真下
        pEnemyShot->speed = 0.0;
        pEnemyShot->param_d[0] = 0.0; // 現在の落下速度
        pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙中玉 = 最後の火球
        pEnemyShot->count = 0;
        pEnemyShot->margin = 60.0; // 大きいので画面外判定を緩めに

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // 重力加速
        if (pShot->kind == img_enemyShotMediumBall[8]) {
            pShot->param_d[0] += 0.035; // g
            if (pShot->param_d[0] > 2.6) pShot->param_d[0] = 2.6;
            pShot->speed = pShot->param_d[0];

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 4Fごとに軌跡に火の粉をこぼす
            if (pShot->count % 4 == 0) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x + GetRand(10) - 5;
                pNew->y = pShot->y;
                pNew->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
                pNew->speed = 0.3 + GetRand(60) / 100.0;
                pNew->kind = img_enemyShotSmallBall[6]; // 白小玉 = こぼれ粉
                pNew->count = 0;
                pNew->margin = 30.0;

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
        else {
            // こぼれた粉はゆっくり落ちる
            pShot->speed *= 0.98;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki) + 0.03;
        }

        pShot = pNext;
    }
}

// --------------------------
// 常時：ほたるのように漂う火の粉（難易度調整用）
// --------------------------
static void ShotSenko_Sprinkle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
        pEnemyShot->speed = 0.5 + GetRand(50) / 100.0;
        pEnemyShot->kind = img_enemyShotScale[6]; // 白鱗弾
        pEnemyShot->count = 0;
        pEnemyShot->margin = 20.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki) + (GetRand(10) - 5) / 100.0; // 横にフラフラ
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体：線香花火
// ============================================================
void EnemyPat_Sparkler_MetaAI()
{
    static int muki;
    static int shot_kind_counter;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_kind_counter = 0;
    }
    else {
        // ゆっくり左右に揺れる線香本体のイメージ
        enemy.x += 0.35 * (double)muki;
        if (enemy.x < 160.0) muki = 1;
        if (enemy.x > 320.0) muki = -1;
    }

    // ヘルパ：ShotSet生成
    auto CreateSet = [](double x, double y, double muki, sEnemyShotSet::PatternFunc func, int alive, int kind) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = func;
        pSet->x = x;
        pSet->y = y;
        pSet->muki = muki;
        pSet->kind = kind;
        pSet->alive = alive;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
        return pSet;
    };

    double toPlayer = atan2(player.y - enemy.y, player.x - enemy.x);

    // Phase1相当：常時漂う火の粉 7Fごと
    if (count % 7 == 0) {
        CreateSet(enemy.x, enemy.y + 8.0, toPlayer, ShotSenko_Sprinkle, 180, shot_kind_counter++);
    }

    // Phase2：パチパチ弾 10Fごと（メイン）
    if (count % 10 == 1) {
        // 自機方向中心に少しランダムを混ぜる
        double r = toPlayer + (GetRand(40) - 20) / 180.0 * DX_PI;
        CreateSet(enemy.x, enemy.y + 10.0, r, ShotSenko_Pachipachi, 120, shot_kind_counter++);
    }

    // Phase3：松葉弾 210Fごと
    if (count % 210 == 110) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 210 == 130) {
        CreateSet(enemy.x, enemy.y + 10.0, 0.0, ShotSenko_Matsuba, 180, shot_kind_counter++);
    }

    // Phase4：落下火球 400Fごと
    if (count % 400 == 350) {
        CreateSet(enemy.x, enemy.y + 10.0, 0.0, ShotSenko_Fall, 300, shot_kind_counter++);
    }
}