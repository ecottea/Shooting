#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// メントスコーラ弾幕：「臨界反応」
//
// 使用素材:
//   ・銃弾(橙)   ：コーラ内で蛇行しながら立ち上る気泡
//   ・中楕円弾(白)：投下されるメントス本体
//   ・小玉(シアン/白)：メントス着弾時に噴き出す泡
//   ・短レーザー(黄)：噴出の瞬間に飛び散る飛沫の閃光
//
// 構成:
//   気泡(ShotBubble)は常時発生し続ける背景密度担当。
//   一定間隔でメントス(ShotMentos)を複数個、時間差で投下。
//   メントスが一定の高さまで落下すると、その場で泡の爆発(ShotFoamBurst)
//   を新規ショットセットとして生成し、メントス自身は役目を終えて消える。
// ============================================================

// 前方宣言
static void ShotFoamBurst(sEnemyShotSet* pEnemyShotSet);

// 新しいショットセットを生成してリストに繋ぐ共通処理
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// 弾幕：泡の噴出(メントス着弾の瞬間に発生)
static void ShotFoamBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 全方位に泡(小玉)を放射
        const int shotNum = 72;
        for (int i = 0; i < shotNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double muki = (DX_PI * 2.0 / shotNum) * i + (GetRand(20) - 10) / 180.0 * DX_PI;
            double speed = 3.0 + GetRand(150) / 100.0;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki; // 発生直後は放射方向
            pEnemyShot->speed = speed;
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[6]; // シアン/白
            pEnemyShot->param_d[0] = speed * cos(muki); // vx
            pEnemyShot->param_d[1] = speed * sin(muki); // vy
            pEnemyShot->param_i[0] = 0; // 0:泡本体

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 弾ける飛沫の閃光(短レーザー、上方向〜斜め上方向に一瞬だけ)
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double muki = -DX_PI / 2.0 + (GetRand(200) - 100) / 180.0 * DX_PI;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki; // 飛び散る方向がそのまま向き
            pEnemyShot->speed = 6.0;
            pEnemyShot->kind = img_enemyShotLaser[1]; // 黄
            pEnemyShot->param_i[0] = 1; // 1:閃光(短命)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 1) {
            // 閃光は短時間だけ直進して消える。
            // これは画面外消去ではなく、演出上の寿命による意図的な削除。
            if (pShot->count >= 6) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 泡本体：初速を保ちつつ重力で徐々に落下に転じる
            pShot->param_d[1] += 0.06; // 重力加速度
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->param_d[0] *= 0.985; // 空気抵抗でわずかに減衰

            // 弧を描く軌道がわかるよう、mukiは常に進行方向へ追従させる
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        }

        pShot = pNext;
    }
}

// メントス着弾位置に泡の爆発を発生させる
static void SpawnFoamBurst(double x, double y)
{
    SpawnShotSet(ShotFoamBurst, x, y);
}

// 弾幕：メントスの投下(1セットにつき5個を時間差で投下)
static void ShotMentos(sEnemyShotSet* pEnemyShotSet)
{
    const int mentosNum = 5;
    const int dropInterval = 15;    // 投下の時間差(フレーム)
    const double fallDistance = 220.0; // この距離落下したら着弾とみなす

    if (pEnemyShotSet->count % dropInterval == 0 && pEnemyShotSet->count / dropInterval < mentosNum) {
        int i = pEnemyShotSet->count / dropInterval;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x + (i - (mentosNum - 1) / 2.0) * 70.0;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 真下に固定(タメを見せるため速度方向と一致させている)
        pEnemyShot->speed = 1.3;
        pEnemyShot->kind = img_enemyShotMediumOval[6]; // 白

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 一定距離落下したら着弾 → 泡を噴出させ、自身は役目を終えて消える。
        // これも画面外消去ではなく、着弾という状態遷移による意図的な削除。
        if (pShot->y >= pEnemyShotSet->y + fallDistance) {
            SpawnFoamBurst(pShot->x, pShot->y);

            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
            continue;
        }

        pShot = pNext;
    }
}

// 弾幕：気泡(常時発生する背景密度担当)
static void ShotBubble(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double baseX = 20.0 + GetRand(440);

            pEnemyShot->x = baseX;
            pEnemyShot->y = 480.0 + GetRand(40);
            pEnemyShot->muki = -DX_PI / 2.0; // 初期は真上
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotBullet[8]; // 橙
            pEnemyShot->param_d[0] = baseX;                       // 蛇行の基準x
            pEnemyShot->param_d[1] = GetRand(628) / 100.0;        // 位相
            pEnemyShot->param_d[2] = 15.0 + GetRand(200) / 10.0;  // 振幅
            pEnemyShot->param_d[3] = 0.03 + GetRand(300) / 10000.0; // 角速度

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double prevX = pShot->x;
        double prevY = pShot->y;

        pShot->y -= pShot->speed;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * sin(pShot->param_d[3] * pShot->count + pShot->param_d[1]);

        // 蛇行しながら上昇する軌跡がわかるよう、mukiは実際の移動方向に追従させる
        pShot->muki = atan2(pShot->y - prevY, pShot->x - prevX);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MentosCola_Claude()
{
    static int mukiDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        mukiDir = 1;
    }
    else {
        enemy.x += 0.5 * (double)mukiDir;
        if (count % 200 == 100) mukiDir *= -1;
    }

    // 気泡は常時、密に発生させ続ける(背景密度担当)
    if (count % 8 == 1) {
        SpawnShotSet(ShotBubble, enemy.x, enemy.y);
    }

    // メントス投下は一定間隔で周期的に発生(1回につき5個、時間差で投下)
    if (count % 400 == 1) {
        SpawnShotSet(ShotMentos, enemy.x, enemy.y + 20.0);
    }
}