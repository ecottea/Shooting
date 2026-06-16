// enemyPat_LeafCutter.cpp
// はっぱカッターをモチーフにした凝った弾幕パターン
// 二重の回転刃、間欠的な小さな弾の放出、脈動半径、うねるような移動

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -------------------------------------------------------
// 補助：直進弾の移動（扇状弾など簡易な弾に使用）
static void SimpleLinearShot(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// -------------------------------------------------------
// 補助：中心位置からプレイヤー方向へ扇状弾を発射する（新しいShotSetを作成）
static void EmitFanShot(const sEnemyShotSet* srcSet, int num, double angleSpan, int kind, double speed)
{
    // 新しいShotSetを作成
    sEnemyShotSet* pNewSet = new sEnemyShotSet;
    pNewSet->count = 0;
    pNewSet->patternFunc = SimpleLinearShot;
    pNewSet->x = srcSet->x;
    pNewSet->y = srcSet->y;
    pNewSet->muki = 0.0;  // 未使用

    // ダミーヘッド
    pNewSet->pEnemyShotHead = new sEnemyShot;
    pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
    pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;

    // プレイヤー方向を基準に扇状に弾を生成
    double baseMuki = atan2(player.y - pNewSet->y, player.x - pNewSet->x);
    for (int i = 0; i < num; ++i) {
        sEnemyShot* p = new sEnemyShot;
        double offset = (i - (num - 1) / 2.0) * angleSpan;
        p->muki = baseMuki + offset;
        p->speed = speed;
        p->x = pNewSet->x;
        p->y = pNewSet->y;
        p->kind = kind;
        //p->margin = 9999;   // 画面外で即消えないようにする

        // 双方向リストへ追加
        p->prev = pNewSet->pEnemyShotHead->prev;
        p->next = pNewSet->pEnemyShotHead;
        pNewSet->pEnemyShotHead->prev->next = p;
        pNewSet->pEnemyShotHead->prev = p;
    }

    // グローバルリストに接続
    pNewSet->prev = enemyShotSetHead.prev;
    pNewSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pNewSet;
    enemyShotSetHead.prev = pNewSet;
}

// -------------------------------------------------------
// 弾幕：はっぱカッター（回転する二重の刃 ＋ 断続的な扇状弾）
static void LeafCutterShot(sEnemyShotSet* pEnemyShotSet)
{
    const int    INNER_NUM = 6;
    const int    OUTER_NUM = 12;
    const double BASE_RADIUS_INNER = 22.0;
    const double BASE_RADIUS_OUTER = 38.0;
    const double ROT_INNER = 0.12;   // [rad/frame] 内側の回転速度
    const double ROT_OUTER = -0.09;   // 外側は逆回転
    const double CENTER_SPEED = 1.5;    // 中心の並進速度（やや抑えめ）
    const double OSC_AMP = 15.0;    // 中心がうねる振幅（横方向）
    const double OSC_FREQ = 0.05;    // うねり周波数

    // ----- 初回だけ二重の刃を生成 -----
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 内側の刃（緑の鱗弾、葉っぱイメージ）
        for (int i = 0; i < INNER_NUM; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = (2.0 * DX_PI) * i / INNER_NUM;   // 初期角度を保存
            p->speed = 0.0;                             // 個別の速度は使わない
            p->kind = img_enemyShotScale[2];           // 緑の鱗弾
            p->margin = 9999;   // 画面外に出てもすぐ消えない

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 外側の刃（黄緑の菱形弾、より軽快な印象）
        for (int i = 0; i < OUTER_NUM; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = (2.0 * DX_PI) * i / OUTER_NUM;
            p->speed = 0.0;
            p->kind = img_enemyShotDiamond[2];         // シアンの菱形弾
            p->margin = 9999;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ----- 中心の移動（プレイヤーへ向かいつつ、うねる） -----
    double age = (double)pEnemyShotSet->count;
    double baseMuki = pEnemyShotSet->muki;   // 生成時のプレイヤー方向
    double lateralOffset = OSC_AMP * sin(age * OSC_FREQ);
    double perpMuki = baseMuki + DX_PI / 2.0;
    pEnemyShotSet->x += CENTER_SPEED * cos(baseMuki) + lateralOffset * cos(perpMuki);
    pEnemyShotSet->y += CENTER_SPEED * sin(baseMuki) + lateralOffset * sin(perpMuki);

    // ----- 半径を脈動させる -----
    double radiusInner = BASE_RADIUS_INNER + 10.0 * sin(age * 0.08);
    double radiusOuter = BASE_RADIUS_OUTER + 14.0 * cos(age * 0.11);

    // ----- 各刃の位置更新 -----
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    int bulletIdx = 0;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double angle = p->muki;   // 初期角度オフセット
        if (bulletIdx < INNER_NUM) {
            angle += ROT_INNER * age;
            p->x = pEnemyShotSet->x + radiusInner * cos(angle);
            p->y = pEnemyShotSet->y + radiusInner * sin(angle);
        }
        else {
            angle += ROT_OUTER * age;
            p->x = pEnemyShotSet->x + radiusOuter * cos(angle);
            p->y = pEnemyShotSet->y + radiusOuter * sin(angle);
        }
        ++bulletIdx;
        p = p->next;
    }

    // ----- 30フレームごとに小さな扇状弾を放つ（独立したShotSetで） -----
    if (age > 0 && (int)age % 30 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        // 赤い小玉を7方向へ速度1.8で扇状に発射（プレイヤー方向中心）
        EmitFanShot(pEnemyShotSet, 5, (10.0 / 180.0 * DX_PI), img_enemyShotSmallBall[0], 1.8);
    }
}

// ============================================================
// 敵本体のパターン（はっぱカッター使い）
// ============================================================
void EnemyPat_RazorLeaf_DeepSeek2()
{
    static int moveMuki;
    static double phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        moveMuki = 1;
        phase = 0.0;
    }
    else {
        enemy.x += 0.9 * moveMuki;
        if (enemy.x < 100.0 || enemy.x > 380.0)
            moveMuki *= -1;

        phase += 0.04;
        enemy.y = 60.0 + 40.0 * sin(phase);
    }

    if (count % 50 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = LeafCutterShot;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 5.0;
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