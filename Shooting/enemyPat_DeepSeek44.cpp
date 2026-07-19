// enemyPat_Arrow.cpp
// 矢印型弾幕：「鏃穿つ方陣」 - 編隊で形作られた矢が壁で跳ね返る

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 矢の編隊を更新するパターン関数
// 矢尻（菱形）＋矢柄＋矢羽（小玉）で１つの矢を表現
// ------------------------------------------------------------
static void ArrowPattern(sEnemyShotSet* pSet)
{
    // 初回のみ矢を構成する弾を一括生成
    if (pSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 各部の定義 { dist, angle(rad) }
        struct Part { double dist; double angle; };
        // 矢尻（先端）
        const Part tip = { 0.0, 0.0 };
        // 矢柄（左右２列）
        const Part shaftL[] = { {8.0, DX_PI + 0.1745}, {16.0, DX_PI + 0.1745}, {24.0, DX_PI + 0.1745}, {32.0, DX_PI + 0.1745} };
        const Part shaftR[] = { {8.0, DX_PI - 0.1745}, {16.0, DX_PI - 0.1745}, {24.0, DX_PI - 0.1745}, {32.0, DX_PI - 0.1745} };
        // 矢羽（尾部で左右に開く）
        const Part featherL = { 36.0, DX_PI + 0.5236 };
        const Part featherR = { 36.0, DX_PI - 0.5236 };

        // 生成ヘルパー
        auto createPart = [&](const Part& p, int kind) {
            sEnemyShot* shot = new sEnemyShot;
            shot->kind = kind;
            shot->speed = 0.0;                     // 手動移動
            shot->x = pSet->x + p.dist * cos(pSet->muki + p.angle);
            shot->y = pSet->y + p.dist * sin(pSet->muki + p.angle);
            shot->muki = pSet->muki;
            // param_d[0] = dist, param_d[1] = angle
            shot->param_d[0] = p.dist;
            shot->param_d[1] = p.angle;
            shot->margin = 480;
            // リストに接続
            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        };

        // 矢尻：赤菱形
        createPart(tip, img_enemyShotDiamond[0]);
        // 矢柄・矢羽：白小玉
        for (auto& s : shaftL) createPart(s, img_enemyShotSmallBall[6]);
        for (auto& s : shaftR) createPart(s, img_enemyShotSmallBall[6]);
        createPart(featherL, img_enemyShotSmallBall[6]);
        createPart(featherR, img_enemyShotSmallBall[6]);

        return;
    }

    // 編隊の移動（count >= 1 以降毎フレーム）
    double speed = pSet->param_d[0];   // 進行速度
    pSet->x += speed * cos(pSet->muki);
    pSet->y += speed * sin(pSet->muki);

    // 画面端（480x480）での反射
    const double W = 480.0, H = 480.0;
    if (pSet->x < 0.0) {
        pSet->x = 0.0;
        pSet->muki = DX_PI - pSet->muki;
    }
    else if (pSet->x > W) {
        pSet->x = W;
        pSet->muki = DX_PI - pSet->muki;
    }
    if (pSet->y < 0.0) {
        pSet->y = 0.0;
        pSet->muki = -pSet->muki;
    }
    else if (pSet->y > H) {
        pSet->y = H;
        pSet->muki = -pSet->muki;
    }

    // 全弾の位置を編隊中心からの相対座標で更新
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        double dist = shot->param_d[0];
        double angle = shot->param_d[1];
        shot->x = pSet->x + dist * cos(pSet->muki + angle);
        shot->y = pSet->y + dist * sin(pSet->muki + angle);
        shot->muki = pSet->muki;   // 向きも揃える（見た目用）
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Arrow_DeepSeek()
{
    static int muki;               // 左右移動の向き
    static int shot_kind_counter;  // 未使用だがサンプル互換用

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_kind_counter = 0;
    }
    else {
        enemy.x += 1.18 * (double)muki;
        if (count % 180 == 90)
            muki *= -1;
    }

    // 80フレームごとに画面四方向から矢の編隊を発射
    if (count % 80 == 0) {
        const double speed = 2.5;

        // 射出位置：上下左右の画面外
        const double startPos[4][2] = {
            { player.x, -20.0   },
            { player.x, 500.0   },
            { -20.0,    player.y },
            { 500.0,    player.y }
        };

        for (int i = 0; i < 4; ++i) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ArrowPattern;
            pSet->x = startPos[i][0];
            pSet->y = startPos[i][1];
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = shot_kind_counter++;
            pSet->param_d[0] = speed;   // 進行速度を格納

            // 弾リストのヘッダ
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            // グローバルリストに追加
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}