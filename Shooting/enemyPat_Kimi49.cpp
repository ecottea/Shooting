// enemyPat_lifegame.cpp
// ライフゲームモチーフ弾幕「世代交代の連鎖（Generational Chain）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数定義
// ============================================================

// 弾の色インデックス
// 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙

// パラメータインデックス定義
static const int PARAM_GENERATION = 0;   // 世代数 (0=母細胞, 1=第1世代, 2=第2世代, 3=第3世代)
static const int PARAM_STATE      = 1;   // 状態 (0=通常, 1=過密死, 2=過疎死, 3=生存, 4=誕生)
static const int PARAM_SPLIT_TIMER = 2;  // 分裂までの残りフレーム
static const int PARAM_HAS_SPLIT  = 3;   // 分裂済みフラグ (0=未分裂, 1=分裂済み)
static const int PARAM_COUNT_NEIGHBORS = 4; // 近傍カウント結果

// 設定値
static const int SPLIT_INTERVAL = 120;   // 分裂間隔 (2秒 = 120フレーム)
static const int MAX_GENERATION = 3;     // 最大世代数
static const double NEIGHBOR_RADIUS = 150.0; // 近傍判定半径
static const double DENSE_SCATTER_SPEED = 3.0; // 過密死散弾速度
static const double SPARSE_RUSH_SPEED = 4.0;   // 過疎死突進速度
static const double SURVIVE_SPEED_MULT = 0.5;  // 生存弾速度倍率
static const double BIRTH_SPEED_MULT = 2.0;    // 誕生弾速度倍率
static const double BIRTH_ANGLE_JITTER = 15.0; // 誕生弾方向ブレ(度)

// ============================================================
//  ユーティリティ関数
// ============================================================

// 2点間の距離を計算
static double Distance(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

// 近傍の弾の数をカウント（同じsEnemyShotSet内の弾のみ）
static int CountNeighbors(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pTarget)
{
    int neighborCount = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot != pTarget) {
            double dist = Distance(pTarget->x, pTarget->y, pShot->x, pShot->y);
            if (dist <= NEIGHBOR_RADIUS) {
                neighborCount++;
            }
        }
        pShot = pShot->next;
    }
    return neighborCount;
}

// ============================================================
//  弾生成ヘルパー
// ============================================================

// 新しい弾を生成してリストに追加
static sEnemyShot* CreateShot(sEnemyShotSet* pEnemyShotSet, double x, double y, 
                               double muki, double speed, int kind, int generation)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->count = 0;
    pShot->margin = 200.0;

    // パラメータ初期化
    pShot->param_i[PARAM_GENERATION] = generation;
    pShot->param_i[PARAM_STATE] = 0;
    pShot->param_i[PARAM_SPLIT_TIMER] = SPLIT_INTERVAL;
    pShot->param_i[PARAM_HAS_SPLIT] = 0;
    pShot->param_i[PARAM_COUNT_NEIGHBORS] = 0;

    for (int i = 5; i < 8; i++) pShot->param_i[i] = 0;
    for (int i = 0; i < 8; i++) pShot->param_d[i] = 0.0;

    // リストに追加
    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;

    return pShot;
}

// 過密死: 8方向に散弾を爆散
static void ExplodeDense(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pParent)
{
    for (int i = 0; i < 8; i++) {
        double angle = (DX_PI / 4.0) * i; // 45度間隔
        // 銃弾(5.0x2.0) 白(6)
        CreateShot(pEnemyShotSet, pParent->x, pParent->y, 
                   angle, DENSE_SCATTER_SPEED, img_enemyShotBullet[6], pParent->param_i[PARAM_GENERATION]+1);
    }
    // 効果音
    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
}

// 過疎死: 自機方向へ突進弾を生成
static void RushSparse(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pParent)
{
    double angleToPlayer = atan2(player.y - pParent->y, player.x - pParent->x);
    // 鱗弾(4.0x3.0) 青(4)
    sEnemyShot* pShot = CreateShot(pEnemyShotSet, pParent->x, pParent->y,
                                    angleToPlayer, SPARSE_RUSH_SPEED, img_enemyShotScale[4], 
                                    pParent->param_i[PARAM_GENERATION]);
    pShot->param_i[PARAM_STATE] = 2; // 過疎死状態
}

// 通常分裂: 8方向に次世代を生成
static void SplitNormal(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pParent, int nextGen)
{
    int kind;
    switch (nextGen) {
        case 1: kind = img_enemyShotMediumBall[1]; break; // 中玉(7.0x7.0) 黄(1)
        case 2: kind = img_enemyShotSmallBall[2]; break;  // 小玉(2.5x2.5) 緑(2)
        case 3: kind = img_enemyShotSmallBall[3]; break;  // 小玉(2.5x2.5) シアン(3)
        default: kind = img_enemyShotMediumBall[1]; break;
    }

    for (int i = 0; i < 8; i++) {
        double angle = (DX_PI / 4.0) * i + (DX_PI / 8.0); // 22.5度ずらして配置
        double speed = 1.5;
        CreateShot(pEnemyShotSet, pParent->x, pParent->y,
                   angle, speed, kind, nextGen);
    }
}

// 生存分裂: 速度半減で8方向に次世代を生成
static void SplitSurvive(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pParent, int nextGen)
{
    // 菱形弾(4.5x2.5) マゼンタ(5)
    int kind = img_enemyShotDiamond[5];

    for (int i = 0; i < 8; i++) {
        double angle = (DX_PI / 4.0) * i + (DX_PI / 8.0);
        double speed = 1.5 * SURVIVE_SPEED_MULT; // 速度半減
        sEnemyShot* pShot = CreateShot(pEnemyShotSet, pParent->x, pParent->y,
                                       angle, speed, kind, nextGen);
        pShot->param_i[PARAM_STATE] = 3; // 生存状態
    }
}

// 誕生分裂: 2倍速度で方向がブレる8方向に次世代を生成
static void SplitBirth(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pParent, int nextGen)
{
    // 中楕円弾(10.5x7.0) 橙(8)
    int kind = img_enemyShotMediumOval[8];

    for (int i = 0; i < 8; i++) {
        double baseAngle = (DX_PI / 4.0) * i + (DX_PI / 8.0);
        // 方向ブレ: ±BIRTH_ANGLE_JITTER度
        // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す
        double jitter = (GetRand((int)(BIRTH_ANGLE_JITTER * 2.0)) - BIRTH_ANGLE_JITTER) / 180.0 * DX_PI;
        double angle = baseAngle + jitter;
        double speed = 1.5 * BIRTH_SPEED_MULT; // 2倍速度
        sEnemyShot* pShot = CreateShot(pEnemyShotSet, pParent->x, pParent->y,
                                       angle, speed, kind, nextGen);
        pShot->param_i[PARAM_STATE] = 4; // 誕生状態
    }
}

// ============================================================
//  ライフゲーム法則に基づく分裂処理
// ============================================================

static void ProcessLifeGameSplit(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pShot)
{
    int generation = pShot->param_i[PARAM_GENERATION];

    // 最大世代を超えたら自然消滅（分裂しない）
    if (generation >= MAX_GENERATION) {
        pShot->param_i[PARAM_HAS_SPLIT] = 1; // 分裂済みとしてマーク（自然消滅）
        return;
    }

    // 近傍の弾の数をカウント
    int neighborCount = CountNeighbors(pEnemyShotSet, pShot);
    pShot->param_i[PARAM_COUNT_NEIGHBORS] = neighborCount;

    int nextGen = generation + 1;

    // ライフゲーム法則適用
    if (neighborCount >= 4) {
        // 過密死: 分裂せず、8方向に散弾を爆散
        pShot->param_i[PARAM_STATE] = 1;
        ExplodeDense(pEnemyShotSet, pShot);
        pShot->param_i[PARAM_HAS_SPLIT] = 1;
    }
    else if (neighborCount <= 1) {
        // 過疎死: 分裂せず、自機方向へ突進
        pShot->param_i[PARAM_STATE] = 2;
        RushSparse(pEnemyShotSet, pShot);
        pShot->param_i[PARAM_HAS_SPLIT] = 1;
    }
    else if (neighborCount == 2) {
        // 生存: 分裂完了、速度半減で滞留
        pShot->param_i[PARAM_STATE] = 3;
        SplitSurvive(pEnemyShotSet, pShot, nextGen);
        pShot->param_i[PARAM_HAS_SPLIT] = 1;
    }
    else if (neighborCount == 3) {
        // 誕生: 2倍速度で分裂、方向ブレ
        pShot->param_i[PARAM_STATE] = 4;
        SplitBirth(pEnemyShotSet, pShot, nextGen);
        pShot->param_i[PARAM_HAS_SPLIT] = 1;
    }
}

// ============================================================
//  弾幕パターン: 世代交代の連鎖
// ============================================================

static void ShotLifeGameChain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // === 初期化: 母細胞の生成 ===
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 母細胞（大玉(20.0x20.0) 赤(0)）を1つ生成
        // 母細胞は移動せず、一定間隔で周囲8方向に分裂生成する
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0; // 母細胞は移動しない
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 大玉 赤
        pEnemyShot->count = 0;
        pEnemyShot->margin = 20.0;

        // パラメータ初期化
        pEnemyShot->param_i[PARAM_GENERATION] = 0;      // 母細胞 = 世代0
        pEnemyShot->param_i[PARAM_STATE] = 0;           // 通常状態
        pEnemyShot->param_i[PARAM_SPLIT_TIMER] = SPLIT_INTERVAL; // 分裂タイマー
        pEnemyShot->param_i[PARAM_HAS_SPLIT] = 0;       // 未分裂
        pEnemyShot->param_i[PARAM_COUNT_NEIGHBORS] = 0;

        for (int i = 5; i < 8; i++) pEnemyShot->param_i[i] = 0;
        for (int i = 0; i < 8; i++) pEnemyShot->param_d[i] = 0.0;

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // === 弾の更新処理 ===
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除対応のため次を保存

        int generation = pShot->param_i[PARAM_GENERATION];
        int state = pShot->param_i[PARAM_STATE];
        int hasSplit = pShot->param_i[PARAM_HAS_SPLIT];

        // --- 移動処理 ---
        // 母細胞は移動しない
        if (generation > 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // --- 分裂タイマー処理 ---
        // 分裂済みでない弾のみタイマーを減らす
        if (hasSplit == 0 && generation < MAX_GENERATION) {
            pShot->param_i[PARAM_SPLIT_TIMER]--;

            // タイマーが0になったらライフゲーム法則に基づいて分裂
            if (pShot->param_i[PARAM_SPLIT_TIMER] <= 0) {
                ProcessLifeGameSplit(pEnemyShotSet, pShot);
            }
        }

        // --- 3世代目の自然消滅演出 ---
        // 最大世代の弾が分裂済み（自然消滅マーク）の場合、
        // 一定時間後にキラキラ輝いて消滅する演出
        if (generation >= MAX_GENERATION && hasSplit == 1) {
            // 自然消滅: 分裂後60フレームで消滅
            // countは毎フレーム自動で+1されるので、誕生時のcountを基準にする
            // ここでは param_d[0] に誕生時のcountを記録
            if (pShot->param_d[0] == 0.0) {
                pShot->param_d[0] = (double)pShot->count; // 消滅開始カウントを記録
            }

            int elapsed = pShot->count - (int)pShot->param_d[0];
            if (elapsed > 60) {
                // リストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }

        pShot = pNext;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================

void EnemyPat_LifeGame_Kimi()
{
    static int muki;
    static int shot_count;
    static int phase;      // フェーズ管理
    static int waitTimer; // 待機タイマー

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
        phase = 0;
        waitTimer = 0;
    }
    else {
        // 敵本体の移動: 左右にゆっくり揺れる
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        // 画面端での clamp
        if (enemy.x < 40.0) { enemy.x = 40.0; muki = 1; }
        if (enemy.x > 440.0) { enemy.x = 440.0; muki = -1; }
    }

    // === 弾幕セットの生成 ===
    // フェーズ0: 母細胞投下
    // フェーズ1: 連鎖が収束するまで待機
    // フェーズ2: 次の母細胞投下まで待機

    if (phase == 0) {
        // 母細胞を投下
        if (count % 140 == 1) { // 5秒間隔で母細胞投下
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotLifeGameChain;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = shot_count++;

            // パラメータ初期化
            for (int i = 0; i < 8; i++) {
                pEnemyShotSet->param_i[i] = 0;
                pEnemyShotSet->param_d[i] = 0.0;
            }

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            //phase = 1;
            phase = 0;
            waitTimer = 0;
        }
    }
    else if (phase == 1) {
        // 連鎖が収束するまで待機（母細胞投下から一定時間）
        waitTimer++;
        if (waitTimer >= 480) { // 8秒後に次のフェーズへ
            phase = 2;
            waitTimer = 0;
        }
    }
    else if (phase == 2) {
        // 次の母細胞投下までの待機
        waitTimer++;
        if (waitTimer >= 120) { // 2秒後にphase0へ戻る
            phase = 0;
            waitTimer = 0;
        }
    }
}