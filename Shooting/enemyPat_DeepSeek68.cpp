// enemyPat_firework.cpp
// 花火モチーフ弾幕「大輪牡丹花火・千輪咲き」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//--------------------------------------
// 前方宣言
//--------------------------------------
static void FireworkFirstBurst(sEnemyShotSet* pEnemyShotSet);
static void FireworkSecondBurst(sEnemyShotSet* pEnemyShotSet);
static void FireworkCenterLaser(sEnemyShotSet* pEnemyShotSet);
static void FireworkScatter(sEnemyShotSet* pEnemyShotSet);

//--------------------------------------
// 第一炸裂「開花」：32方向の大型楕円弾
// 一定距離で停止し、第二炸裂を発生させる
//--------------------------------------
static void FireworkFirstBurst(sEnemyShotSet* pEnemyShotSet)
{
    const double TARGET_RADIUS = 150.0;   // 停止距離
    const int    FINAL_COUNT = 240;     // 散華を開始するフレーム

    // 初回：32方向に大弾を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 32; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = (i / 32.0) * 2.0 * DX_PI;
            pShot->speed = 2.8;   // 低速
            // 牡丹花火の花弁をイメージした中楕円弾（色:橙 8）
            pShot->kind = img_enemyShotMediumOval[8];

            // 追跡用パラメータ
            pShot->param_i[0] = 0;               // 二次炸裂発生済みフラグ
            pShot->param_d[0] = pEnemyShotSet->x; // 発射原点X
            pShot->param_d[1] = pEnemyShotSet->y; // 発射原点Y

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動と制御
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;  // 削除に備えて次を保存

        // 原点からの距離を計算
        double dx = pShot->x - pShot->param_d[0];
        double dy = pShot->y - pShot->param_d[1];
        double dist = sqrt(dx * dx + dy * dy);

        if (dist >= TARGET_RADIUS) {
            // 停止
            pShot->speed = 0.0;

            // まだ二次炸裂を発生させていなければ生成
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1;

                // 第二炸裂用ショットセットを作成
                sEnemyShotSet* pNewSet = new sEnemyShotSet;
                pNewSet->count = 0;
                pNewSet->patternFunc = FireworkSecondBurst;
                pNewSet->x = pShot->x;
                pNewSet->y = pShot->y;
                pNewSet->muki = 0.0;
                pNewSet->kind = 0;

                // ダミーヘッド
                pNewSet->pEnemyShotHead = new sEnemyShot;
                pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
                pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;

                // グローバルリストに接続
                pNewSet->prev = enemyShotSetHead.prev;
                pNewSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pNewSet;
                enemyShotSetHead.prev = pNewSet;
            }
        }
        else {
            // 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }

    // 散華（フィナーレ）処理
    if (pEnemyShotSet->count == FINAL_COUNT) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;

            // 弾がまだ画面内なら散華弾を生成
            if (pShot->x > -50 && pShot->x < 530 && pShot->y > -50 && pShot->y < 530) {
                sEnemyShotSet* pScatterSet = new sEnemyShotSet;
                pScatterSet->count = 0;
                pScatterSet->patternFunc = FireworkScatter;
                pScatterSet->x = pShot->x;
                pScatterSet->y = pShot->y;
                pScatterSet->muki = 0.0;
                pScatterSet->kind = 0;

                pScatterSet->pEnemyShotHead = new sEnemyShot;
                pScatterSet->pEnemyShotHead->prev = pScatterSet->pEnemyShotHead;
                pScatterSet->pEnemyShotHead->next = pScatterSet->pEnemyShotHead;

                pScatterSet->prev = enemyShotSetHead.prev;
                pScatterSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pScatterSet;
                enemyShotSetHead.prev = pScatterSet;
            }

            // リストから外して削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;

            pShot = pNext;
        }
    }
}

//--------------------------------------
// 第二炸裂「千輪咲き」＋「垂れ柳」
// 8方向のレーザー弾を高速発射し、重力カーブさせる
//--------------------------------------
static void FireworkSecondBurst(sEnemyShotSet* pEnemyShotSet)
{
    const int STRAIGHT_FRAMES = 20;   // 直進フレーム数
    const double GRAVITY_ANGLE = DX_PI / 2.0; // 下方向（90°）

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            double base = (i / 8.0) * 2.0 * DX_PI + (GetRand(10) - 5) * (DX_PI / 180.0); // ±5°ランダム
            pShot->muki = base;
            pShot->speed = 4.5;   // 高速
            // 色とりどりの短レーザー (赤0,黄1,緑2,シアン3,マゼンタ5 からランダム)
            int cols[] = { 0,1,2,3,5 };
            int col = cols[GetRand(4)];
            pShot->kind = img_enemyShotLaser[col];
            pShot->margin = 64;

            pShot->param_i[0] = 0;   // 経過フレーム
            pShot->param_d[0] = base; // 初期角度

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 移動（直進 → 重力カーブ）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_i[0]++;
        if (pShot->param_i[0] > STRAIGHT_FRAMES) {
            // 徐々に下方向へカーブ
            double diff = GRAVITY_ANGLE - pShot->muki;
            // 角度を正規化
            while (diff > DX_PI)  diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.06;  // ゆるやかなカーブ
            pShot->speed *= 0.995;       // わずかな減速
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

//--------------------------------------
// 中心回転レーザー「花芯の渦」
// 6本のレーザーが回転し、先端から自機狙いの火花弾を発射
//--------------------------------------
static void FireworkCenterLaser(sEnemyShotSet* pEnemyShotSet)
{
    const double RADIUS = 100.0;        // 回転半径
    const double ROTATE_SPEED = 0.02;   // 回転速度（rad/frame）
    const int    FIRE_INTERVAL = 20;    // 火花弾発射間隔

    if (pEnemyShotSet->count == 0) {
        // 6本の回転レーザー弾
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + RADIUS * cos(i * DX_PI / 3.0);
            pShot->y = pEnemyShotSet->y + RADIUS * sin(i * DX_PI / 3.0);
            pShot->muki = i * DX_PI / 3.0;  // 外向き
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotLaser[0];  // 赤レーザー
            pShot->margin = 64;

            pShot->param_i[0] = i;   // インデックス
            pShot->param_i[1] = 0;   // 0=レーザー, 1=火花

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 基本角度（セットのカウントで回転）
    double baseAngle = ROTATE_SPEED * pEnemyShotSet->count;

    // 全弾更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 0) {
            // レーザー弾：回転位置を更新
            double angle = baseAngle + pShot->param_i[0] * DX_PI / 3.0;
            pShot->x = pEnemyShotSet->x + RADIUS * cos(angle);
            pShot->y = pEnemyShotSet->y + RADIUS * sin(angle);
            pShot->muki = angle;  // 向きも回転

            // 火花弾発射（一定間隔）
            if (pEnemyShotSet->count % FIRE_INTERVAL == 0 && pEnemyShotSet->count < 600) {
                // 自機狙い方向
                double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                sEnemyShot* pSpark = new sEnemyShot;
                pSpark->x = pShot->x;
                pSpark->y = pShot->y;
                pSpark->muki = aim;
                pSpark->speed = 2.5;
                pSpark->kind = img_enemyShotSmallBall[1]; // 黄色の小玉
                pSpark->param_i[1] = 1; // 火花弾マーク

                pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSpark->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                pEnemyShotSet->pEnemyShotHead->prev = pSpark;
            }
        }
        else {
            // 火花弾：直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//--------------------------------------
// フィナーレ「散華」：低速全方位の粉弾
//--------------------------------------
static void FireworkScatter(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 短い効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 12方向＋ランダムに低速弾
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            double angle = (i * 2.0 * DX_PI / 16) + (GetRand(20) - 10) * DX_PI / 180.0;
            pShot->muki = angle;
            pShot->speed = 1.0 + GetRand(100) / 100.0; // 1.0～2.0
            // 鱗弾や小玉をランダムカラーで
            int col = GetRand(5); // 0～5 (赤黄緑シアンマゼンタ)
            if (i % 2 == 0)
                pShot->kind = img_enemyShotSmallBall[col];
            else
                pShot->kind = img_enemyShotScale[col];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾移動（直線）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//--------------------------------------
// 敵本体のパターン
//--------------------------------------
void EnemyPat_Firework_DeepSeek()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // シーケンス管理：60フレームの予兆の後、第一炸裂＋回転レーザーを開始
    if (count % 600 == 61) {
        // --- 第一炸裂セット ---
        sEnemyShotSet* pFirst = new sEnemyShotSet;
        pFirst->count = 0;
        pFirst->patternFunc = FireworkFirstBurst;
        pFirst->x = enemy.x;
        pFirst->y = enemy.y;
        pFirst->muki = 0.0;
        pFirst->kind = 0;
        pFirst->pEnemyShotHead = new sEnemyShot;
        pFirst->pEnemyShotHead->prev = pFirst->pEnemyShotHead;
        pFirst->pEnemyShotHead->next = pFirst->pEnemyShotHead;

        pFirst->prev = enemyShotSetHead.prev;
        pFirst->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFirst;
        enemyShotSetHead.prev = pFirst;

        // --- 中心回転レーザーセット ---
        sEnemyShotSet* pCenter = new sEnemyShotSet;
        pCenter->count = 0;
        pCenter->patternFunc = FireworkCenterLaser;
        pCenter->x = enemy.x;
        pCenter->y = enemy.y;
        pCenter->muki = 0.0;
        pCenter->kind = 0;
        pCenter->pEnemyShotHead = new sEnemyShot;
        pCenter->pEnemyShotHead->prev = pCenter->pEnemyShotHead;
        pCenter->pEnemyShotHead->next = pCenter->pEnemyShotHead;

        pCenter->prev = enemyShotSetHead.prev;
        pCenter->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pCenter;
        enemyShotSetHead.prev = pCenter;
    }
}