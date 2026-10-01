#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// Peano曲線弾幕 - 充填する聖域
// 27x27のスネーク型ペアノ曲線で画面を埋めていく
// ============================================================
static void ShotPeano(sEnemyShotSet* pEnemyShotSet)
{
    const int GRID = 27; // 3^3 = 27 -> 729マス
    const double AREA_SIZE = 400.0; // 弾幕エリア 400x400
    const double AREA_LEFT = 40.0; // 左 40
    const double AREA_TOP = 80.0; // 上 80 (敵の下から)
    const double CELL = AREA_SIZE / GRID; // 約14.8px
    const int WALL_LIFETIME = 300-100; // 壁が残るフレーム (5秒)
    const int STEP_INTERVAL = 4-3; // 4Fごとに壁を1つ進める
    const int ATTACK_INTERVAL = 12; // 12Fごとに先端から攻撃

    // 初回初期化
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0; // 現在のpath index (0..GRID*GRID-1)
        pEnemyShotSet->param_d[0] = AREA_LEFT;
        pEnemyShotSet->param_d[1] = AREA_TOP;
        pEnemyShotSet->param_d[2] = CELL;

        pEnemyShotSet->x = AREA_LEFT + CELL * 0.5;
        pEnemyShotSet->y = AREA_TOP + CELL * 0.5;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // index -> world座標変換 (スネーク型 = 最も単純なPeano曲線)
    auto GridToWorld = [&](int idx, double& wx, double& wy) {
        int gy = idx / GRID;
        int gx = idx % GRID;
        if (gy % 2 == 1) gx = GRID - 1 - gx; // 奇数行は反転
        wx = AREA_LEFT + gx * CELL + CELL * 0.5;
        wy = AREA_TOP + gy * CELL + CELL * 0.5;
    };

    // ---- 壁生成 ----
    if (pEnemyShotSet->count % STEP_INTERVAL == 0) {
        int pathIdx = pEnemyShotSet->param_i[0];

        //if (pathIdx >= GRID * GRID) {
        //    pathIdx = 0; // ループして塗り直し
        //    pEnemyShotSet->param_i[0] = 0;
        //}

        double wx, wy;
        GridToWorld(pathIdx, wx, wy);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = wx;
        pShot->y = wy;
        pShot->muki = 0.0;
        pShot->speed = 0.0;
        pShot->count = 0;
        pShot->param_i[0] = 0; // 0=壁
        pShot->kind = img_enemyShotMediumBall[4]; // 中玉・青 7.0x7.0

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        if (pathIdx % 9 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        pEnemyShotSet->param_i[0]++; // 次へ
        pEnemyShotSet->x = wx; // 先端位置を更新
        pEnemyShotSet->y = wy;
    }

    // ---- 先端からの追跡弾 ----
    if (pEnemyShotSet->count % ATTACK_INTERVAL == 0) {
        double hx = pEnemyShotSet->x;
        double hy = pEnemyShotSet->y;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = hx;
        pShot->y = hy;
        double base = atan2(player.y - hy, player.x - hx);
        // GetRand(x)は0..xを返すので注意
        double spread = (GetRand(20) - 10) / 180.0 * DX_PI; // -10度〜+10度
        pShot->muki = base + spread;
        pShot->speed = 2.2 + GetRand(60) / 100.0; // 2.2〜2.8
        pShot->count = 0;
        pShot->param_i[0] = 1; // 1=攻撃弾

        if (GetRand(3) == 0) {
            pShot->kind = img_enemyShotDiamond[0]; // 赤菱形 アクセント
        }
        else {
            pShot->kind = img_enemyShotSmallBall[0]; // 赤小玉
        }

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // ---- 移動・寿命管理 ----
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        if (p->param_i[0] == 0) {
            // 壁: 寿命で自前削除 (画面外削除はメインに任せる仕様)
            if (p->count > WALL_LIFETIME) {
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
        }
        else {
            // 攻撃弾: 通常移動 + ペアノらしい揺らぎ
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p->muki += sin(p->count * 0.05) * 0.01;
        }
        p = next;
    }
}

// 敵本体
void EnemyPat_PeanoCurve_MetaAI()
{
    static int muki;
    static int phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 中央でゆっくり往復、弾幕の観察を邪魔しない程度に
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 弾幕セットは1つだけ生成し、内部でループさせる
    if (count % 500 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeano;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}