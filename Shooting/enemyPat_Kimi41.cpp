// enemyPat_tmp.cpp
// ブロック崩しモチーフ弾幕：リフレクション・ブレイク

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  ファイルスコープ static
// ------------------------------------------------------------
namespace {
    static sEnemyShot* s_blockShots[8] = {};   // ブロックとしての弾ポインタ
    static int         s_blockHp[8] = {};      // 各ブロックのHP
    static int         s_remainBlocks = 0;
    static sEnemyShot* s_pBall = nullptr;
    static int         s_ballState = 0;        // 0:浮遊 1:降下 2:反射上昇 3:撃墜消失 4:狂化
    static int         s_targetIdx = -1;
    static int         s_finalTriggered = 0;
    static double      s_ballHitX = 0.0, s_ballHitY = 0.0;
}

// ------------------------------------------------------------
//  汎用弾幕パターン関数
// ------------------------------------------------------------
static void ShotLinear(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 固定弾用：速度0で待機、速度が付いたら直線運動
static void ShotFixed(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->speed > 0.0) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// 誘導弾用（狂化時）
static void ShotHoming(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double targetMuki = atan2(player.y - p->y, player.x - p->x);
        double diff = targetMuki - p->muki;
        while (diff > DX_PI)  diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        p->muki += diff * 0.06;
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
//  ヘルパー：弾幕セット・弾生成
// ------------------------------------------------------------
static sEnemyShotSet* NewShotSet(void (*func)(sEnemyShotSet*))
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = 0.0; p->y = 0.0; p->muki = 0.0; p->kind = 0;
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

static void AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int marker = 0)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x; p->y = y; p->muki = muki; p->speed = speed;
    p->kind = kind;
    p->param_i[0] = marker;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 扇状3way小弾（ブロック破壊時）
static void Create3Way(double x, double y, double baseMuki)
{
    sEnemyShotSet* pSet = NewShotSet(ShotLinear);
    pSet->x = x; pSet->y = y;
    for (int i = -30; i <= 30; i++) {
        double m = GetRand(359) / 360.0 * (2 * DX_PI);
        int    col = 1 + GetRand(2);  // 黄(1), 緑(2), シアン(3)
        AddShot(pSet, x, y, m, 2.5, img_enemyShotSmallBall[col]);
    }
}

// 8方向分裂弾（ボール撃墜時）
static void Create8Way(double x, double y)
{
    sEnemyShotSet* pSet = NewShotSet(ShotLinear);
    pSet->x = x; pSet->y = y;
    for (int i = 0; i < 200; i++) {
        double m = i * (2 * DX_PI / 200);
        AddShot(pSet, x, y, m, 4.0, img_enemyShotMediumBall[8]); // 橙
    }
}

// 固定弾生成（残りブロック数に比例、撃墜時の代償）
static void CreateFixedShots(int numBlocks, double x, double y)
{
    if (numBlocks <= 0) return;
    sEnemyShotSet* pSet = NewShotSet(ShotFixed);
    pSet->x = x; pSet->y = y;
    pSet->param_i[0] = 777;
    int n = numBlocks * 2;
    for (int i = 0; i < n; i++) {
        double fx = x + GetRand(200) - 100.0;
        double fy = y + GetRand(200) - 100.0;
        if (fx < 20.0) fx = 20.0; if (fx > 460.0) fx = 460.0;
        if (fy < 20.0) fy = 20.0; if (fy > 460.0) fy = 460.0;
        AddShot(pSet, fx, fy, 0.0, 0.0, img_enemyShotScale[4], 777); // 青・鱗弾
    }
}

// 固定弾を誘導弾へ一斉変換（全ブロック破壊後の狂化）
static void ConvertFixedToHoming()
{
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->param_i[0] == 777) {
            pSet->patternFunc = ShotHoming;
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                if (p->param_i[0] == 777) {
                    p->speed = 2.2;
                    p->muki = atan2(player.y - p->y, player.x - p->x);
                    p->kind = img_enemyShotBullet[5]; // マゼンタ
                }
                p = p->next;
            }
        }
        pSet = pSet->next;
    }
}

// ------------------------------------------------------------
//  ボール（核）用パターン関数
// ------------------------------------------------------------
static void ShotBall(sEnemyShotSet* pSet)
{
    if (!s_pBall) return;

    // ---- 状態0：浮遊 ----
    if (s_ballState == 0) {
        s_pBall->x = 240.0 + sin(pSet->count / 45.0) * 20.0;
        s_pBall->y = 60.0 + sin(pSet->count / 30.0) * 15.0;
        s_pBall->speed = 0.0;

        if (pSet->count % 180 == 0) {
            int cand[8], n = 0;
            for (int i = 0; i < 8; i++) if (s_blockHp[i] > 0) cand[n++] = i;
            if (n > 0) {
                s_targetIdx = cand[GetRand(n - 1)];
                s_ballState = 1;
                double dx = s_blockShots[s_targetIdx]->x - s_pBall->x;
                double dy = s_blockShots[s_targetIdx]->y - s_pBall->y;
                s_pBall->muki = atan2(dy, dx);
                s_pBall->speed = 2.2;
            }
            else if (!s_finalTriggered) {
                s_finalTriggered = 1;
                s_ballState = 4;
                s_pBall->speed = 3.0;
                s_pBall->muki = atan2(player.y - s_pBall->y, player.x - s_pBall->x);
                s_pBall->kind = img_enemyShotLargeBall[0]; // 赤に変化
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
                ConvertFixedToHoming();
            }
        }
    }
    // ---- 状態1：降下中 ----
    else if (s_ballState == 1) {
        s_pBall->x += s_pBall->speed * cos(s_pBall->muki);
        s_pBall->y += s_pBall->speed * sin(s_pBall->muki);

        for (int i = 0; i < 8; i++) {
            if (s_blockHp[i] > 0) {
                double dx = s_pBall->x - s_blockShots[i]->x;
                double dy = s_pBall->y - s_blockShots[i]->y;
                if (dx * dx + dy * dy < 400.0) {
                    s_blockHp[i] = 0;
                    s_remainBlocks--;
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                    Create3Way(s_blockShots[i]->x, s_blockShots[i]->y, s_pBall->muki);

                    // ブロック弾を画面外に追い出してメインルーチンに消去させる
                    s_blockShots[i]->y = -9999.0;

                    s_ballState = 2;
                    s_pBall->muki = atan2(player.y - s_pBall->y, player.x - s_pBall->x);
                    s_pBall->speed = 2.8;
                    break;
                }
            }
        }
    }
    // ---- 状態2：反射上昇 ----
    else if (s_ballState == 2) {
        s_pBall->x += s_pBall->speed * cos(s_pBall->muki);
        s_pBall->y += s_pBall->speed * sin(s_pBall->muki);
        if (s_pBall->y > 470.0 || s_pBall->x < -20.0 || s_pBall->x > 500.0) {
            s_ballState = 0;
            s_pBall->speed = 0.0;
            s_pBall->x = 240.0;
            s_pBall->y = 60.0;
            s_pBall->kind = img_enemyShotLargeBall[6]; // 白に戻す
        }
    }
    // ---- 状態4：狂化（プレイヤー追尾） ----
    else if (s_ballState == 4) {
        s_pBall->x += s_pBall->speed * cos(s_pBall->muki);
        s_pBall->y += s_pBall->speed * sin(s_pBall->muki);
        double targetMuki = atan2(player.y - s_pBall->y, player.x - s_pBall->x);
        double diff = targetMuki - s_pBall->muki;
        while (diff > DX_PI)  diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        s_pBall->muki += diff * 0.04;
        if (s_pBall->y < -20.0 || s_pBall->y > 500.0 ||
            s_pBall->x < -20.0 || s_pBall->x > 500.0) {
            s_pBall->x = 240.0; s_pBall->y = 60.0;
        }
    }

    // ---- プレイヤー弾との当たり判定（撃ち落とし） ----
    if (s_ballState == 1 || s_ballState == 2 || s_ballState == 4) {
        sPlayerShot* ps = playerShotHead.next;
        while (ps != &playerShotHead) {
            double dx = s_pBall->x - ps->x;
            double dy = s_pBall->y - ps->y;
            if (dx * dx + dy * dy < 256.0) {
                s_ballHitX = s_pBall->x;
                s_ballHitY = s_pBall->y;
                s_ballState = 3;
                s_pBall->x = -9999.0;
                s_pBall->speed = 0.0;

                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                Create8Way(s_ballHitX, s_ballHitY);
                CreateFixedShots(s_remainBlocks, s_ballHitX, s_ballHitY);

                s_pBall = nullptr;
                break;
            }
            ps = ps->next;
        }
    }
}

// ------------------------------------------------------------
//  敵本体パターン
// ------------------------------------------------------------
void EnemyPat_BlockBreak_Kimi()
{
    static int muki = 1;

    if (count == 1) {
        // --- 初期化 ---
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        s_remainBlocks = 8;
        s_ballState = 0;
        s_targetIdx = -1;
        s_finalTriggered = 0;

        // ブロックを敵弾（白色大玉）として生成
        sEnemyShotSet* pBlockSet = NewShotSet(ShotFixed);
        pBlockSet->param_i[0] = 888; // ブロックセット識別子
        for (int i = 0; i < 8; i++) {
            s_blockHp[i] = 1;
            s_blockShots[i] = new sEnemyShot;
            s_blockShots[i]->x = 30.0 + i * 60.0; // 30,90,150,210,270,330,390,450
            s_blockShots[i]->y = 80.0;
            s_blockShots[i]->muki = 0.0;
            s_blockShots[i]->speed = 0.0;
            s_blockShots[i]->kind = img_enemyShotLargeBall[6]; // 白色大玉
            s_blockShots[i]->param_i[0] = 888; // ブロック識別子

            s_blockShots[i]->prev = pBlockSet->pEnemyShotHead->prev;
            s_blockShots[i]->next = pBlockSet->pEnemyShotHead;
            pBlockSet->pEnemyShotHead->prev->next = s_blockShots[i];
            pBlockSet->pEnemyShotHead->prev = s_blockShots[i];
        }

        // ボール（核）生成：白色大玉
        sEnemyShotSet* pBallSet = new sEnemyShotSet;
        pBallSet->count = 0;
        pBallSet->patternFunc = ShotBall;
        pBallSet->x = 240.0; pBallSet->y = 60.0;
        pBallSet->muki = 0.0; pBallSet->kind = 0;
        pBallSet->pEnemyShotHead = new sEnemyShot;
        pBallSet->pEnemyShotHead->prev = pBallSet->pEnemyShotHead;
        pBallSet->pEnemyShotHead->next = pBallSet->pEnemyShotHead;

        s_pBall = new sEnemyShot;
        s_pBall->x = 240.0; s_pBall->y = 60.0;
        s_pBall->muki = 0.0; s_pBall->speed = 0.0;
        s_pBall->kind = img_enemyShotLargeBall[6]; // 白色
        s_pBall->param_i[0] = 100; // ボール識別子

        s_pBall->prev = pBallSet->pEnemyShotHead->prev;
        s_pBall->next = pBallSet->pEnemyShotHead;
        pBallSet->pEnemyShotHead->prev->next = s_pBall;
        pBallSet->pEnemyShotHead->prev = s_pBall;

        pBallSet->prev = enemyShotSetHead.prev;
        pBallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBallSet;
        enemyShotSetHead.prev = pBallSet;
    }
    else {
        // 敵本体の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}