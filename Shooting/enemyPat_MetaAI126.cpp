#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#define PI2 (DX_PI * 2.0)

// このセット内で使う弾の識別用
enum {
    TYPE_BOTTLE = 0,
    TYPE_MENTOS = 1,
    TYPE_JET_UP = 2,
    TYPE_JET_RAIN = 3,
    TYPE_FOAM = 4
};

static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int type)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->param_i[0] = type;
    p->margin = 480;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// メントスコーラ噴出
static void ShotMentosGeyser(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->param_i[0] : 0=振り 1=噴出中
    // pEnemyShotSet->param_i[1] : 落としたメントス数
    // pEnemyShotSet->param_d[0] : ボトル中心xの揺れ基準

    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 1. ボトルの輪郭を SmallBall[7]黒 で作る
        const double bodyW = 30.0*2;
        const double bodyH = 45.0*2;
        const double neckW = 12.0*2;
        const double neckH = 18.0*2;
        const double baseY = 15.0*2;

        double outline[][2] = {
            {-bodyW / 2, baseY},
            {-bodyW / 2, baseY - bodyH},
            {-bodyW / 3, baseY - bodyH - 4},
            {-neckW / 2, baseY - bodyH - 8},
            {-neckW / 2, baseY - bodyH - 8 - neckH},
            { neckW / 2, baseY - bodyH - 8 - neckH},
            { neckW / 2, baseY - bodyH - 8},
            { bodyW / 3, baseY - bodyH - 4},
            { bodyW / 2, baseY - bodyH},
            { bodyW / 2, baseY}
        };
        const int outlineNum = 10;
        const int div = 8; // 1辺を8分割 -> 約80発で輪郭を補間

        for (int s = 0; s < outlineNum; s++) {
            int e = (s + 1) % outlineNum;
            // 底辺(10->1)は閉じない
            if (s == outlineNum - 1) continue;
            for (int k = 0; k < div; k++) {
                double t = (double)k / (double)div;
                // なめらかに見えるよう smoothstep
                double st = t * t * (3.0 - 2.0 * t);
                double ox = outline[s][0] * (1.0 - st) + outline[e][0] * st;
                double oy = outline[s][1] * (1.0 - st) + outline[e][1] * st;

                sEnemyShot* p = new sEnemyShot;
                p->x = pEnemyShotSet->x + ox;
                p->y = pEnemyShotSet->y + oy;
                p->muki = 0;
                p->speed = 0;
                p->kind = img_enemyShotSmallBall[7];
                p->count = 0;
                p->param_i[0] = TYPE_BOTTLE;
                p->param_d[0] = ox;
                p->param_d[1] = oy;
                p->param_d[2] = GetRand(100) / 100.0 * PI2;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // --- 既存弾の移動 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case TYPE_BOTTLE:
            if (pEnemyShotSet->param_i[0] == 0) {
                // 振るフェーズ: ブルブル揺れる
                double shake = sin(pEnemyShotSet->count * 0.45 + pShot->param_d[2]) * 3.0;
                pShot->x = pEnemyShotSet->x + pShot->param_d[0] + shake;
                pShot->y = pEnemyShotSet->y + pShot->param_d[1] + cos(pEnemyShotSet->count * 0.38) * 1.0;
            }
            else {
                // 噴出後は弾けて消えるように外へ飛ばす
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;

        case TYPE_MENTOS:
            // 自由落下
            pShot->param_d[1] += pShot->param_d[2]; // vy += gravity
            pShot->y += pShot->param_d[1];
            // 底でバウンドして止まる
            if (pShot->y > pEnemyShotSet->y + 15.0) {
                pShot->y = pEnemyShotSet->y + 15.0;
                pShot->param_d[1] *= -0.25;
                if (fabs(pShot->param_d[1]) < 0.3) pShot->param_d[1] = 0;
            }
            if (pEnemyShotSet->count == 120) pShot->margin = -9999;
            break;

        case TYPE_JET_UP:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.985; // 空気抵抗で減速
            // 頂点に達したら雨に変化
            if (pShot->speed < 0.8) {
                pShot->param_i[0] = TYPE_JET_RAIN;
                pShot->kind = img_enemyShotMediumOval[8]; // 橙の中楕円でしぶき
                pShot->muki = DX_PI * 0.5 + (GetRand(60) - 30) / 180.0 * DX_PI; // 真下+ばらけ
                pShot->speed = 1.8 + GetRand(150) / 100.0;
                pShot->param_d[0] = 0.12; // 雨の加速度
            }
            break;

        case TYPE_JET_RAIN:
            pShot->param_d[0] += 0.02; // 重力加速
            pShot->y += pShot->speed + pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki) * 0.3;
            break;

        case TYPE_FOAM:
            // 減速して滞留、わずかに自機方向へ漂う
            pShot->speed *= 0.96;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 0.5%だけ自機へカーブ
            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = toPlayer - pShot->muki;
            while (diff > DX_PI) diff -= PI2;
            while (diff < -DX_PI) diff += PI2;
            pShot->muki += diff * 0.008;
            if (pShot->speed < 0.01) pShot->margin = -9999;
            break;
        }
        pShot = pShot->next;
    }

    // --- 生成ロジック ---
    // 2. メントス投入: 60, 80, 100Fで1個ずつ
    if (pEnemyShotSet->count == 60 || pEnemyShotSet->count == 80 || pEnemyShotSet->count == 100) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pEnemyShotSet->x + (GetRand(10) - 5);
        p->y = pEnemyShotSet->y - 40.0; // 口の上から落とす
        p->muki = DX_PI * 0.5;
        p->speed = 0;
        p->kind = img_enemyShotLargeBall[6]; // 白の大玉=メントス
        p->count = 0;
        p->param_i[0] = TYPE_MENTOS;
        p->param_d[1] = 0.5 + GetRand(50) / 100.0; // 初速vy
        p->param_d[2] = 0.18; // 重力
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_i[1]++;
    }

    // 3. 噴出トリガー
    if (pEnemyShotSet->count == 120) {
        pEnemyShotSet->param_i[0] = 1;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ボトルを弾けさせる
        sEnemyShot* ps = pEnemyShotSet->pEnemyShotHead->next;
        while (ps != pEnemyShotSet->pEnemyShotHead) {
            if (ps->param_i[0] == TYPE_BOTTLE) {
                ps->muki = atan2(ps->y - pEnemyShotSet->y, ps->x - pEnemyShotSet->x) + (GetRand(40) - 20) / 180.0 * DX_PI;
                ps->speed = 4.0 + GetRand(200) / 100.0;
            }
            ps = ps->next;
        }

        // 初期泡バースト 32発
        for (int i = 0; i < 32; i++) {
            double mk = PI2 * i / 32.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            double sp = 2.5 + GetRand(200) / 100.0;
            int col = (i % 3 == 0) ? img_enemyShotMediumBall[6] : img_enemyShotSmallBall[6]; // 白で泡
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y - 38.0, mk, sp, col, TYPE_FOAM);
        }
    }

    // 4. メインジェット: 120Fから240Fまで毎フレーム3発、橙の小玉
    if (pEnemyShotSet->count >= 120 && pEnemyShotSet->count < 260) {
        for (int j = 0; j < 2; j++) {
            double mk = -DX_PI * 0.5 + (GetRand(80) - 40) / 180.0 * DX_PI; // 真上 ±40度
            double sp = 7.5 + GetRand(200) / 100.0;
            // GetRand注意: 0〜x を返す
            AddShot(pEnemyShotSet, pEnemyShotSet->x + (GetRand(10) - 5), pEnemyShotSet->y - 38.0, mk, sp, img_enemyShotSmallBall[8], TYPE_JET_UP);
        }
        // 泡の追加膨張: 8Fごとにリング状に
        if (pEnemyShotSet->count % 8 == 0) {
            for (int i = 0; i < 6; i++) {
                double mk = PI2 * i / 6.0 + pEnemyShotSet->count * 0.05;
                AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y - 30.0, mk, 1.2 + GetRand(80) / 100.0, img_enemyShotSmallBall[6], TYPE_FOAM);
            }
        }
    }
}

// 敵本体
void EnemyPat_MentosCola_MetaAI()
{
    static int muki;
    static int nextGeyserCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0+250;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextGeyserCount = 30;
    }
    else {
        // 振りフェーズ中は小刻みに揺れる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        bool isShaking = false;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotMentosGeyser && pSet->count < 120) { isShaking = true; break; }
            pSet = pSet->next;
        }
        if (isShaking) {
            enemy.x += sin(count * 0.6) * 1.8;
        }
        else {
            enemy.x += 0.4 * muki;
            if (count % 240 == 0) muki *= -1;
        }
    }

    // 360F周期で噴出セットを生成
    if (count == nextGeyserCount) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMentosGeyser;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        nextGeyserCount = count + 360;
    }
}