#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ユーティリティ
// ============================================================
static sEnemyShotSet* CreateEmptyShotSet(double x, double y, double muki, sEnemyShotSet::PatternFunc func, int kind = 0)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;
    pSet->patternFunc = func;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static int ColorByHp(int hp, int maxHp)
{
    if (hp <= 1) return 8;
    if (hp * 4 <= maxHp) return 0;
    if (hp * 2 <= maxHp) return 1;
    if (hp * 4 <= maxHp * 3) return 2;
    if (hp == maxHp) return 6;
    return 4;
}

// 大玉ザコ判定: 240margin + 大玉画像 + HPが設定されているもののみ
static bool IsZakoLargeBall(const sEnemyShot* s)
{
    if (s->margin < 239.0) return false; // 仕様240.0のザコのみ
    if (s->param_i[14] <= 0) return false; // maxHPが無いのはザコ弾
    // 画像ハンドルが大玉であることを確認
    for (int c = 0; c < 9; c++) {
        if (s->kind == img_enemyShotLargeBall[c]) return true;
    }
    return false;
}

// ============================================================
// ザコ弾パターン (大玉以外のみ使用)
// ============================================================
static void Shot_ZakoAimed3Way(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + i * 18.0 / 180.0 * DX_PI + (GetRand(20) - 10) / 180.0 * DX_PI;
            pShot->speed = 2.8 + GetRand(80) / 100.0;
            pShot->kind = img_enemyShotMediumBall[0];
            pShot->margin = 20.0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

static void Shot_ZakoFastBullet(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki + (GetRand(30) - 15) / 180.0 * DX_PI;
        pShot->speed = 4.5 + GetRand(100) / 100.0;
        pShot->kind = img_enemyShotBullet[4];
        pShot->margin = 20.0;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

static void Shot_ZakoSmallCircle(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + i * DX_PI * 2 / 8.0;
            pShot->speed = 1.8 + GetRand(40) / 100.0;
            pShot->kind = img_enemyShotSmallBall[GetRand(8) % 9];
            pShot->margin = 20.0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

static void Shot_ZakoScaleRain(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x + GetRand(20) - 10;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + i * 12.0 / 180.0 * DX_PI;
            pShot->speed = 2.2 + GetRand(60) / 100.0;
            pShot->kind = (i == 0) ? img_enemyShotDiamond[1] : img_enemyShotScale[2];
            pShot->margin = 20.0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

// ============================================================
// 衝突判定 (大玉ザコ vs 自機ショット のみ)
// ザコ弾は対象外 = 自機ショットを素通りする
// ============================================================
static void ProcessCollision_LargeZako()
{
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        sEnemyShotSet* nextSet = pSet->next;
        sEnemyShot* pZako = pSet->pEnemyShotHead->next;
        bool setDeleted = false;

        while (pZako != pSet->pEnemyShotHead) {
            sEnemyShot* nextZako = pZako->next;
            if (!IsZakoLargeBall(pZako)) { // 大玉ザコ以外は無視
                pZako = nextZako;
                continue;
            }

            sPlayerShot* pP = playerShotHead.next;
            while (pP != &playerShotHead) {
                sPlayerShot* nextP = pP->next;
                double dx = pZako->x - pP->x;
                double dy = pZako->y - pP->y;
                if (dx * dx + dy * dy <= 25.0 * 25.0) {
                    if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
                    PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);
                    
                    pZako->param_i[15]--;
                    int hp = pZako->param_i[15];
                    int maxHp = pZako->param_i[14];
                    int col = ColorByHp(hp > 0 ? hp : 1, maxHp);
                    pZako->kind = img_enemyShotLargeBall[col];

                    pP->prev->next = pP->next;
                    pP->next->prev = pP->prev;
                    delete pP;

                    if (hp <= 0) {
                        enemy.hp--;
                        // 撃破時演出: 小玉円形 (自機ショットとは衝突しない)
                        CreateEmptyShotSet(pZako->x, pZako->y, 0, Shot_ZakoSmallCircle, 0);

                        pZako->prev->next = pZako->next;
                        pZako->next->prev = pZako->prev;
                        delete pZako;

                        if (pSet->pEnemyShotHead->next == pSet->pEnemyShotHead) {
                            sEnemyShot* head = pSet->pEnemyShotHead;
                            pSet->prev->next = pSet->next;
                            pSet->next->prev = pSet->prev;
                            delete head;
                            delete pSet;
                            setDeleted = true;
                        }
                        break;
                    }
                }
                pP = nextP;
            }
            if (setDeleted) break;
            pZako = nextZako;
        }
        pSet = nextSet;
    }
}

// ============================================================
// ザコ生成
// ============================================================
static void SpawnZako(double x, double y, double vx, double vy, int hp, sEnemyShotSet::PatternFunc moveFunc)
{
    hp = 2;

    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = atan2(player.y - y, player.x - x);
    pSet->patternFunc = moveFunc;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    sEnemyShot* body = new sEnemyShot;
    body->x = x;
    body->y = y;
    body->kind = img_enemyShotLargeBall[6];
    body->margin = 240.0;
    body->param_i[15] = hp;
    body->param_i[14] = hp;
    body->param_d[0] = vx;
    body->param_d[1] = vy;
    body->prev = pSet->pEnemyShotHead->prev;
    body->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = body;
    pSet->pEnemyShotHead->prev = body;
}

// ============================================================
// ザコ移動
// ============================================================
static void ZakoPattern_Horizontal(sEnemyShotSet* pSet)
{
    sEnemyShot* b = pSet->pEnemyShotHead->next;
    if (b == pSet->pEnemyShotHead || !IsZakoLargeBall(b)) return;
    b->x += b->param_d[0];
    b->y += b->param_d[1];
    pSet->x = b->x; pSet->y = b->y;

    if (pSet->count == 60) {
        b->param_d[0] *= 0.32;
        b->param_d[1] = 0.7 + GetRand(30) / 30.0;
    }
    if (pSet->count > 60 && pSet->count < 200) {
        b->param_d[0] += sin(pSet->count * 0.07) * 0.02;
        if (pSet->count % 68 == 0) {
            double m = atan2(player.y - b->y, player.x - b->x);
            CreateEmptyShotSet(b->x, b->y, m, Shot_ZakoAimed3Way, 0);
        }
    }
    if (pSet->count >= 200) {
        b->param_d[1] += 0.035;
        if (pSet->count % 56 == 0) {
            double m = atan2(player.y - b->y, player.x - b->x);
            CreateEmptyShotSet(b->x, b->y, m, Shot_ZakoFastBullet, 0);
        }
    }
}

static void ZakoPattern_V(sEnemyShotSet* pSet)
{
    sEnemyShot* b = pSet->pEnemyShotHead->next;
    if (b == pSet->pEnemyShotHead || !IsZakoLargeBall(b)) return;
    b->x += b->param_d[0];
    b->y += b->param_d[1];
    b->param_d[0] += sin(pSet->count * 0.05 + b->x * 0.01) * 0.01;
    pSet->x = b->x; pSet->y = b->y;

    if (pSet->count > 40 && pSet->count % 84 == 0) {
        double m = atan2(player.y - b->y, player.x - b->x);
        auto f = (pSet->count % 2 == 0) ? Shot_ZakoScaleRain : Shot_ZakoAimed3Way;
        CreateEmptyShotSet(b->x, b->y, m, f, 0);
    }
}

static void ZakoPattern_Diagonal(sEnemyShotSet* pSet)
{
    sEnemyShot* b = pSet->pEnemyShotHead->next;
    if (b == pSet->pEnemyShotHead || !IsZakoLargeBall(b)) return;
    b->x += b->param_d[0];
    b->y += b->param_d[1];
    pSet->x = b->x; pSet->y = b->y;

    if (pSet->count % 60 == 0 && pSet->count > 20) {
        double m = atan2(player.y - b->y, player.x - b->x);
        CreateEmptyShotSet(b->x, b->y, m, Shot_ZakoFastBullet, 0);
    }
    if (pSet->count == 90) b->param_d[1] += 0.6;
}

// ============================================================
// ボス本体
// ============================================================
void EnemyPat_OnTheWay_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 50;
    }
    else {
        enemy.x = 240.0;
        enemy.y = -80.0;
    }

    ProcessCollision_LargeZako();
    if (enemy.hp <= 0) return;

    int hpScale = 8 + count / 1200;
    if (hpScale > 14) hpScale = 14;

    if (count % 280 == 10) {
        for (int i = 0; i < 5; i++) {
            double y0 = 70.0 + i * 38.0 + GetRand(10);
            double vx = 2.6 + GetRand(20) / 20.0;
            SpawnZako(-80.0, y0, vx, 0.2, hpScale, ZakoPattern_Horizontal);
        }
    }
    if (count % 280 == 55) {
        for (int i = 0; i < 5; i++) {
            double y0 = 90.0 + i * 38.0 + GetRand(10);
            double vx = -2.6 - GetRand(20) / 20.0;
            SpawnZako(560.0, y0, vx, 0.2, hpScale, ZakoPattern_Horizontal);
        }
    }
    if (count % 460 == 90) {
        for (int i = -3; i <= 3; i++) {
            double x0 = 240.0 + i * 46.0;
            double y0 = -100.0 - abs(i) * 24.0 - GetRand(15);
            SpawnZako(x0, y0, i * 0.18, 1.6 + GetRand(20) / 20.0, hpScale + 2, ZakoPattern_V);
        }
    }
    if (count % 580 == 130) {
        for (int k = 0; k < 3; k++) {
            SpawnZako(-60.0 - GetRand(30), -90.0 - GetRand(30), 2.0 + GetRand(15) / 15.0, 2.4 + GetRand(10) / 20.0, hpScale - 1, ZakoPattern_Diagonal);
            SpawnZako(540.0 + GetRand(30), -90.0 - GetRand(30), -2.0 - GetRand(15) / 15.0, 2.4 + GetRand(10) / 20.0, hpScale - 1, ZakoPattern_Diagonal);
        }
    }
}