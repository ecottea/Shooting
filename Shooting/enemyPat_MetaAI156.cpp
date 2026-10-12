#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 定幅転弾「ルーロー・ローラー」
// ルーローの三角形を3つの円弧の集合として生成し、塊として転がす
static void ShotReuleaux(sEnemyShotSet* pEnemyShotSet)
{
    const double PI = DX_PI;
    const double SQRT3 = 1.7320508075688772;

    // pEnemyShotSet->param_d の割り当て
    // [0] R : ルーローの幅 = 正三角形の一辺
    // [1] rot : 全体の回転角
    // [2] cy : 三角形の重心 y
    // [3] cx : 重心 x
    // [4] vy : 縦方向の速さ
    // [5] rotSpeed : 回転速度
    // [6] driftX : 左への流れる速さ
    // [8] dirY : 上下方向 1 or -1
    // param_i[0] phase 0:生成直後 1:転がり 2:破裂

    if (pEnemyShotSet->count == 0) {
        // 出現演出
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double R = 155.0 + (double)GetRand(30); // 155-185 GetRandは0～x inclusive
        double d = R / SQRT3; // 重心から頂点までの距離

        pEnemyShotSet->param_d[0] = R;
        pEnemyShotSet->param_d[1] = 0.0;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[3] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[4] = 1.4 + (double)GetRand(60) / 100.0; // 1.4-2.0
        pEnemyShotSet->param_d[5] = (GetRand(1) == 0 ? 1.0 : -1.0) * (0.010 + (double)GetRand(10) / 1000.0);
        pEnemyShotSet->param_d[6] = 0.25 + (double)GetRand(20) / 100.0; // 0.25-0.45
        pEnemyShotSet->param_d[8] = (GetRand(1) == 0 ? 1.0 : -1.0);
        pEnemyShotSet->param_i[0] = 0;

        // 正三角形の3頂点 V0,V1,V2 を重心原点で定義
        // V0: 上 (-90度), V1: 左下 150度, V2: 右下 30度
        double Vx[3], Vy[3];
        Vx[0] = 0.0; Vy[0] = -d;
        Vx[1] = -d * 0.8660254037844386; Vy[1] = d * 0.5;
        Vx[2] = d * 0.8660254037844386; Vy[2] = d * 0.5;

        auto AddShot = [&](double ox, double oy, int kind, double margin, int typeFlag) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->param_d[3] + ox;
            p->y = pEnemyShotSet->param_d[2] + oy;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = kind;
            p->count = 0;
            p->margin = margin;
            p->param_d[0] = ox;
            p->param_d[1] = oy;
            p->param_i[0] = typeFlag; // 0:輪郭 1:核 2:内部
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        };

        // 1. 輪郭: 各頂点を中心とする60度の円弧
        const int OUTLINE_PER_ARC = 18; // 18*3=54発
        for (int c = 0; c < 3; c++) {
            int j = (c + 1) % 3;
            int k = (c + 2) % 3;
            double startA = atan2(Vy[j] - Vy[c], Vx[j] - Vx[c]);
            double endA = atan2(Vy[k] - Vy[c], Vx[k] - Vx[c]);
            double diff = endA - startA;
            while (diff <= -PI) diff += 2 * PI;
            while (diff > PI) diff -= 2 * PI;
            for (int i = 0; i < OUTLINE_PER_ARC; i++) {
                double t = (double)i / (double)(OUTLINE_PER_ARC - 1);
                double ang = startA + diff * t;
                double px = Vx[c] + R * cos(ang);
                double py = Vy[c] + R * sin(ang);
                // 中玉 シアン(3)で輪郭をくっきり
                AddShot(px, py, img_enemyShotMediumBall[3], 50.0, 0);
            }
        }

        // 2. 核: 3頂点に大玉 橙(8)
        for (int c = 0; c < 3; c++) {
            AddShot(Vx[c], Vy[c], img_enemyShotLargeBall[8], 70.0, 1);
        }

        // 3. 内部塗り: 縮小したルーローを2層
        const double scales[2] = { 0.35, 0.70 };
        const int INNER_PER_ARC = 12;
        for (int s = 0; s < 2; s++) {
            double Rs = R * scales[s];
            double ds = d * scales[s];
            double SVx[3], SVy[3];
            SVx[0] = 0.0; SVy[0] = -ds;
            SVx[1] = -ds * 0.8660254037844386; SVy[1] = ds * 0.5;
            SVx[2] = ds * 0.8660254037844386; SVy[2] = ds * 0.5;
            for (int c = 0; c < 3; c++) {
                int j = (c + 1) % 3;
                int k = (c + 2) % 3;
                double startA = atan2(SVy[j] - SVy[c], SVx[j] - SVx[c]);
                double endA = atan2(SVy[k] - SVy[c], SVx[k] - SVx[c]);
                double diff = endA - startA;
                while (diff <= -PI) diff += 2 * PI;
                while (diff > PI) diff -= 2 * PI;
                for (int i = 0; i < INNER_PER_ARC; i++) {
                    double t = (double)i / (double)(INNER_PER_ARC - 1);
                    double ang = startA + diff * t;
                    double px = SVx[c] + Rs * cos(ang);
                    double py = SVy[c] + Rs * sin(ang);
                    // 小玉 白(6)で面を埋める
                    AddShot(px, py, img_enemyShotSmallBall[6], 45.0, 2);
                }
            }
        }
    }

    // フェーズ管理
    if (pEnemyShotSet->count == 20) {
        pEnemyShotSet->param_i[0] = 1; // 転がり開始
    }
    if (pEnemyShotSet->count == 280) {
        // 破裂
        pEnemyShotSet->param_i[0] = 2;
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            double dx = p->x - pEnemyShotSet->param_d[3];
            double dy = p->y - pEnemyShotSet->param_d[2];
            p->muki = atan2(dy, dx);
            // 輪郭は少し速く飛ばす
            double base = (p->param_i[0] == 0) ? 2.2 : 1.3;
            p->speed = base + (double)GetRand(80) / 100.0; // GetRand注意: 0-80 inclusive
            p = p->next;
        }
    }

    // 毎フレームの更新
    if (pEnemyShotSet->param_i[0] == 1) {
        // 転がり中
        double R = pEnemyShotSet->param_d[0];
        double cy = pEnemyShotSet->param_d[2] + pEnemyShotSet->param_d[8] * pEnemyShotSet->param_d[4];
        double cx = pEnemyShotSet->param_d[3] - pEnemyShotSet->param_d[6];
        double rot = pEnemyShotSet->param_d[1] + pEnemyShotSet->param_d[5];

        // 上下の壁でバウンド (幅一定なので cy の可動範囲は R に依存)
        double top = 70.0 + R * 0.55;
        double bottom = 410.0 - R * 0.55;
        if (cy < top) {
            cy = top;
            pEnemyShotSet->param_d[8] = 1.0;
        }
        if (cy > bottom) {
            cy = bottom;
            pEnemyShotSet->param_d[8] = -1.0;
        }

        pEnemyShotSet->param_d[2] = cy;
        pEnemyShotSet->param_d[3] = cx;
        pEnemyShotSet->param_d[1] = rot;

        double cosR = cos(rot);
        double sinR = sin(rot);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            double ox = p->param_d[0];
            double oy = p->param_d[1];
            // 重心周りに回転
            double rx = ox * cosR - oy * sinR;
            double ry = ox * sinR + oy * cosR;
            p->x = cx + rx;
            p->y = cy + ry;
            p = p->next;
        }
    }
    else if (pEnemyShotSet->param_i[0] == 2) {
        // 破裂後は直進
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p = p->next;
        }
    }
    else {
        // phase 0: 静止 (生成直後の見せ)
        // 何もしない - 初期位置のまま
    }
}

void EnemyPat_ReuleauxTriangle_MetaAI()
{
    static int dir;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
        shot_count = 0;
    }
    else {
        // 敵本体はゆっくり左右往復
        enemy.x += 0.55 * (double)dir;
        if (enemy.x < 140.0) dir = 1;
        if (enemy.x > 340.0) dir = -1;
    }

    // 110フレーム毎にルーローを1つ生成
    if (count % 110 == 10) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotReuleaux;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}