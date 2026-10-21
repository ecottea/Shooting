#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double POOL_Y = 380.0;

//------------------------------------------------------------
// 1. 壺 - 橙中楕円で表現。常に自機方向へ少し傾く
//------------------------------------------------------------
static void ShotJug(sEnemyShotSet* pSet)
{
    const double base[6][2] = {
        {-10, -12}, {10, -12},
        {-12, 0}, {12, 0},
        {-10, 10}, {10, 10}
    };

    if (pSet->count == 0) {
        for (int i = 0; i < 6; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + base[i][0];
            p->y = pSet->y + base[i][1];
            p->kind = img_enemyShotMediumOval[8]; // 橙
            p->param_d[0] = base[i][0];
            p->param_d[1] = base[i][1];
            p->margin = 100.0;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 自機方向へ向けて傾ける
    double target = atan2(player.y - enemy.y, player.x - enemy.x);
    double tilt = target - DX_PI / 2.0;
    if (tilt > 0.4) tilt = 0.4;
    if (tilt < -0.4) tilt = -0.4;
    pSet->param_d[2] = tilt;

    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double ox = p->param_d[0];
        double oy = p->param_d[1];
        double rx = ox * cos(tilt) - oy * sin(tilt);
        double ry = ox * sin(tilt) + oy * cos(tilt);
        p->x = pSet->x + rx;
        p->y = pSet->y + ry;
        p->muki = tilt + DX_PI / 2.0;
        p = p->next;
    }
}

//------------------------------------------------------------
// 2. 注乳弾 - 白小玉。遅延追尾してから急落下
//------------------------------------------------------------
static void ShotMilkStream(sEnemyShotSet* pSet)
{
    double tilt = pSet->param_d[0];
    double mouth_ox = 12.0;
    double mouth_oy = 2.0;
    double mx = mouth_ox * cos(tilt) - mouth_oy * sin(tilt);
    double my = mouth_ox * sin(tilt) + mouth_oy * cos(tilt);
    pSet->x = enemy.x + mx;
    pSet->y = enemy.y + 12.0 + my;

    // 頻繁に発射
    if (pSet->count % 4 == 0) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = DX_PI / 2.0 + tilt * 0.5;
        p->speed = 1.5;
        p->kind = img_enemyShotSmallBall[6]; // 白
        p->margin = 40.0;
        p->param_i[0] = 0; // 0:追尾 1:落下 2:跳ね
        p->param_d[0] = GetRand(628) / 100.0;

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;

        if (p->param_i[0] == 0) { // 追尾フェーズ
            double aim = atan2(player.y - p->y, player.x - p->x);
            p->muki = p->muki * 0.95 + aim * 0.05; // 緩やかに曲がる
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p->speed += 0.03;
            if (p->y > POOL_Y - 40) p->param_i[0] = 1;
        }
        else if (p->param_i[0] == 1) { // 落下フェーズ
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p->speed += 0.06;
            if (p->y >= POOL_Y) {
                p->param_i[0] = 2;
                p->y = POOL_Y;
                p->speed = 2.5;
                // 着弾時に3方向に跳ねさせる
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* q = new sEnemyShot;
                    q->x = p->x;
                    q->y = p->y;
                    q->muki = -DX_PI / 2.0 + (i - 1) * DX_PI / 6.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                    q->speed = 2.0 + GetRand(30) / 100.0;
                    q->kind = img_enemyShotSmallBall[6];
                    q->margin = 30.0;
                    q->param_i[0] = 3; // 跳ね弾
                    q->prev = pSet->pEnemyShotHead->prev;
                    q->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = q;
                    pSet->pEnemyShotHead->prev = q;
                }
            }
        }
        else if (p->param_i[0] == 2) { // 跳ねた後
            p->speed *= 0.92;
            p->y -= p->speed;
            if (p->speed < 0.5) p->param_i[0] = 3;
        }
        else { // 消滅待ち
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = next;
    }
}

//------------------------------------------------------------
// 3. 泡弾 - 白中玉。一定時間後に自機狙い4wayに分裂
//------------------------------------------------------------
static void ShotBubble(sEnemyShotSet* pSet)
{
    if (pSet->count % 60 == 0) {
        sEnemyShot* p = new sEnemyShot;
        p->x = 120.0 + GetRand(240);
        p->y = POOL_Y + (GetRand(20) - 10);
        p->muki = -DX_PI / 2.0;
        p->speed = 0.6;
        p->kind = img_enemyShotMediumBall[6]; // 白中玉
        p->margin = 40.0;
        p->param_i[0] = 0; // 上昇中
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;

        if (p->param_i[0] == 0) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p->speed *= 0.995;
            if (p->count > 40) {
                // 4way自機狙いで破裂
                for (int i = 0; i < 4; i++) {
                    sEnemyShot* q = new sEnemyShot;
                    q->x = p->x;
                    q->y = p->y;
                    q->muki = atan2(player.y - p->y, player.x - p->x) + (i - 1.5) * DX_PI / 8.0;
                    q->speed = 2.2 + GetRand(40) / 100.0;
                    q->kind = img_enemyShotSmallBall[6];
                    q->param_i[0] = 1;
                    q->margin = 30.0;
                    q->prev = pSet->pEnemyShotHead->prev;
                    q->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = q;
                    pSet->pEnemyShotHead->prev = q;
                }
                // 本体消去
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
                p = next;
                continue;
            }
        }
        else {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p->speed *= 0.995;
        }
        p = next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Milkmaid_MetaAI()
{
    static bool init = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        init = false;
    }
    {
        // 左右に振りながら自機を追う
        enemy.x += 1.2 * cos((count - 1) * 0.015);
    }

    if (!init) {
        init = true;
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 壺
        sEnemyShotSet* s1 = new sEnemyShotSet;
        s1->count = 0; s1->patternFunc = ShotJug; s1->x = enemy.x; s1->y = enemy.y + 10;
        s1->pEnemyShotHead = new sEnemyShot; s1->pEnemyShotHead->prev = s1->pEnemyShotHead; s1->pEnemyShotHead->next = s1->pEnemyShotHead;
        s1->prev = enemyShotSetHead.prev; s1->next = &enemyShotSetHead; enemyShotSetHead.prev->next = s1; enemyShotSetHead.prev = s1;

        // 注乳
        sEnemyShotSet* s2 = new sEnemyShotSet;
        s2->count = 0; s2->patternFunc = ShotMilkStream; s2->x = enemy.x; s2->y = enemy.y;
        s2->pEnemyShotHead = new sEnemyShot; s2->pEnemyShotHead->prev = s2->pEnemyShotHead; s2->pEnemyShotHead->next = s2->pEnemyShotHead;
        s2->prev = enemyShotSetHead.prev; s2->next = &enemyShotSetHead; enemyShotSetHead.prev->next = s2; enemyShotSetHead.prev = s2;

        // 泡
        sEnemyShotSet* s3 = new sEnemyShotSet;
        s3->count = 0; s3->patternFunc = ShotBubble; s3->x = 240; s3->y = POOL_Y;
        s3->pEnemyShotHead = new sEnemyShot; s3->pEnemyShotHead->prev = s3->pEnemyShotHead; s3->pEnemyShotHead->next = s3->pEnemyShotHead;
        s3->prev = enemyShotSetHead.prev; s3->next = &enemyShotSetHead; enemyShotSetHead.prev->next = s3; enemyShotSetHead.prev = s3;
    }
}