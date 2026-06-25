// enemyPat_sierpinski.cpp
// シェルピンスキーのギャスケットをモチーフにした弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>
#include <functional>

// ------------------------------------------------------------
// 弾幕パターン：シェルピンスキー・ギャスケット
//   再帰的に三角メッシュを生成し、弾を頂点(重心)に配置。
//   各弾は放射状に拡散する。
// ------------------------------------------------------------
static void ShotSierpinski(sEnemyShotSet* pEnemyShotSet)
{
    const double SideLength = 250.0;   // 大三角形の一辺の長さ
    const int    MaxDepth = 6;       // 再帰深度（弾数は 3^4 = 81 個）

    if (pEnemyShotSet->count == 0) {
        // 発射音
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = pEnemyShotSet->muki;   // 照準方向

        // 中点計算用
        auto mid = [](double x1, double y1, double x2, double y2) {
            return std::make_pair((x1 + x2) * 0.5, (y1 + y2) * 0.5);
        };

        // 重心のリスト
        std::vector<std::pair<double, double>> centroids;

        // 再帰関数：三角形を3分割し、末端の重心を記録
        std::function<void(double, double, double, double, double, double, int)> recurse;
        recurse = [&](double ax, double ay, double bx, double by, double cx, double cy, int depth) {
            if (depth == 0) {
                double gx = (ax + bx + cx) / 3.0;
                double gy = (ay + by + cy) / 3.0;
                centroids.emplace_back(gx, gy);
            }
            else {
                auto [abx, aby] = mid(ax, ay, bx, by);
                auto [acx, acy] = mid(ax, ay, cx, cy);
                auto [bcx, bcy] = mid(bx, by, cx, cy);
                recurse(ax, ay, abx, aby, acx, acy, depth - 1);
                recurse(abx, aby, bx, by, bcx, bcy, depth - 1);
                recurse(acx, acy, bcx, bcy, cx, cy, depth - 1);
            }
        };

        // 大三角形の頂点（ローカル座標）
        // 頂点 A = (0,0),  B, C は baseAngle ± 30° の方向に SideLength だけ離れた点
        double ax = 0.0, ay = 0.0;
        double rad30 = 30.0 * DX_PI / 180.0;
        double bx = SideLength * cos(baseAngle + rad30);
        double by = SideLength * sin(baseAngle + rad30);
        double cx = SideLength * cos(baseAngle - rad30);
        double cy = SideLength * sin(baseAngle - rad30);

        recurse(ax, ay, bx, by, cx, cy, MaxDepth);

        // 弾を生成
        for (size_t i = 0; i < centroids.size(); ++i) {
            double lx = centroids[i].first;
            double ly = centroids[i].second;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + lx;
            pShot->y = pEnemyShotSet->y + ly;
            pShot->muki = atan2(ly, lx);               // 外向き放射状
            pShot->speed = 2.0 + (i % 3) * 0.3;         // 速度にわずかな変化
            pShot->kind = img_enemyShotSmallBall[i % 6]; // 色変化

            // リンクリスト末尾に追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレームの移動処理（生成済みの全弾を動かす）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Sierpinski_DeepSeek()
{
    static int moveDir;
    static int shotCycle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
        shotCycle = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.7 * moveDir;
        if (enemy.x < 60.0 || enemy.x > 420.0) moveDir *= -1;

        // たまに上下位置を変える
        if (count % 240 == 120) enemy.y = 50.0 + GetRand(40);
    }

    // 180フレームごとにシェルピンスキー弾幕を発射
    if (count % 120 == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSierpinski;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pSet->kind = shotCycle++;

        // 空の循環リスト
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体リストへ追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}