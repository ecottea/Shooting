#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
//  弾幕大相撲『満員土俵の押し出し』
//  - 画面いっぱいの土俵を俵弾で表現
//  - ボスが自機に向かって突進
//  - 近距離でのみ両者の間に斥力が働く
// ============================================================

// ---- リスト操作ヘルパ ----
static void AddShotToList(sEnemyShotSet* set, sEnemyShot* s)
{
    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

static sEnemyShotSet* CreateSet(double x, double y, double muki, int kind,
    sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->kind = kind;
    p->patternFunc = func;
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// ============================================================
//  土俵（俵弾）：画面いっぱいの円周上に大玉を並べ、ゆっくり回転
//  param_d[0] : 半径 / param_d[1] : 回転方向
//  各弾の param_d[0] : 現在角度 / param_d[1] : 角速度
// ============================================================
static void ShotDohyo(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int n = 60;
        double r = pEnemyShotSet->param_d[0];
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            double ang = 2.0 * DX_PI * (double)i / (double)n;
            p->x = 240.0 + r * cos(ang);
            p->y = 240.0 + r * sin(ang);
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLargeBall[1]; // 黄（俵）
            p->param_d[0] = ang;
            p->param_d[1] = 0.0055;              // 回転角速度
            AddShotToList(pEnemyShotSet, p);
        }
    }

    // 土俵をゆっくり締めていく（土煙のイメージ）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->param_d[0] > 200.0) {
        pEnemyShotSet->param_d[0] -= 0.02;
    }

    double r = pEnemyShotSet->param_d[0];
    double dir = pEnemyShotSet->param_d[1];
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->param_d[0] += p->param_d[1] * dir;
        p->x = 240.0 + r * cos(p->param_d[0]);
        p->y = 240.0 + r * sin(p->param_d[0]);
        p = p->next;
    }
}

// ============================================================
//  塩まき：白い小玉を扇状に散布
// ============================================================
static void ShotSalt(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = 7;
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x + (GetRand(20) - 10);
            p->y = pEnemyShotSet->y + (GetRand(20) - 10);
            p->muki = pEnemyShotSet->muki + (GetRand(80) - 40) / 180.0 * DX_PI;
            p->speed = 2.5 + GetRand(180) / 100.0;
            p->kind = img_enemyShotSmallBall[6]; // 白
            AddShotToList(pEnemyShotSet, p);
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  突進の残り火：ボス後方へ散る中玉
// ============================================================
static void ShotTrail(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int n = 3;
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x + (GetRand(16) - 8);
            p->y = pEnemyShotSet->y + (GetRand(16) - 8);
            p->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            p->speed = 3.0 + GetRand(200) / 100.0;
            p->kind = img_enemyShotMediumBall[8]; // 橙
            AddShotToList(pEnemyShotSet, p);
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  押し合いの火花：接触点から垂直方向の両側へ
// ============================================================
static void ShotSpark(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int n = 3;
        for (int i = 0; i < n; i++) {
            for (int s = -1; s <= 1; s += 2) {
                sEnemyShot* p = new sEnemyShot;
                p->x = pEnemyShotSet->x + (GetRand(10) - 5);
                p->y = pEnemyShotSet->y + (GetRand(10) - 5);
                p->muki = pEnemyShotSet->muki + s * (GetRand(30) / 180.0 * DX_PI);
                p->speed = 2.0 + GetRand(220) / 100.0;
                p->kind = img_enemyShotSmallBall[8]; // 橙
                AddShotToList(pEnemyShotSet, p);
            }
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  突っ張り弾：中央復帰中に自機狙いで放つ
// ============================================================
static void ShotTsuppari(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = 5;
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = pEnemyShotSet->muki + (GetRand(30) - 15) / 180.0 * DX_PI;
            p->speed = 3.5 + GetRand(150) / 100.0;
            p->kind = img_enemyShotBullet[0]; // 赤い銃弾
            AddShotToList(pEnemyShotSet, p);
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体：弾幕大相撲『満員土俵の押し出し』
//  フェーズ 0:仕切り / 1:立ち合い(突進) / 2:押し合い / 3:引き技
// ============================================================
void EnemyPat_Sumou_DeepSeek()
{
    static int    phase;
    static int    phaseStart;
    static double chargeVX, chargeVY;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        phaseStart = count;

        // 土俵（俵弾）を生成
        sEnemyShotSet* set = CreateSet(240.0, 240.0, 0.0, 0, ShotDohyo);
        set->param_d[0] = 225.0; // 土俵半径（画面いっぱい）
        set->param_d[1] = 1.0;   // 回転方向
    }

    int t = count - phaseStart;

    switch (phase) {

        // --------------------------------------------------------
        //  仕切り：中央で塩を撒き、自機をロックオン
        // --------------------------------------------------------
    case 0: {
        if (t > 0 && t % 25 == 0 && t < 100) {
            double ang = -DX_PI * 0.5 + (GetRand(60) - 30) / 180.0 * DX_PI;
            CreateSet(enemy.x, enemy.y, ang, 0, ShotSalt);
        }
        if (t >= 120) {
            double dx = player.x - enemy.x;
            double dy = player.y - enemy.y;
            double d = sqrt(dx * dx + dy * dy);
            if (d < 1.0) d = 1.0;
            chargeVX = dx / d * 5.5;
            chargeVY = dy / d * 5.5;
            phase = 1;
            phaseStart = count;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;
    }

          // --------------------------------------------------------
          //  立ち合い：自機めがけて一直線に突進
          // --------------------------------------------------------
    case 1: {
        enemy.x += chargeVX;
        enemy.y += chargeVY;

        // 残り火
        if (t % 4 == 0) {
            double back = atan2(chargeVY, chargeVX) + DX_PI;
            CreateSet(enemy.x, enemy.y, back, 0, ShotTrail);
        }

        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);

        // 中心からの距離（土俵からはみ出ないよう制限）
        double cx = enemy.x - 240.0;
        double cy = enemy.y - 240.0;
        double cd = sqrt(cx * cx + cy * cy);

        if (dist < 130.0) {
            // 近接 → 押し合いへ
            phase = 2;
            phaseStart = count;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else if (t > 70 || cd > 190.0) {
            // 外した／土俵際 → 引き技へ
            phase = 3;
            phaseStart = count;
        }
        break;
    }

          // --------------------------------------------------------
          //  押し合い：近距離でのみ斥力が発生
          // --------------------------------------------------------
    case 2: {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 1.0) dist = 1.0;
        double nx = dx / dist; // ボス→自機の単位ベクトル
        double ny = dy / dist;

        const double maxR = 130.0;
        double power = 1.0 - dist / maxR;
        if (power < 0.0) power = 0.0;

        // ボスは前進を続ける（押し込み）
        enemy.x += chargeVX * 0.6;
        enemy.y += chargeVY * 0.6;

        // 斥力でボスは自機から押し戻される（質量大なので小さめ）
        double bossPush = power * 3.0;
        enemy.x -= nx * bossPush;
        enemy.y -= ny * bossPush;

        // 自機側の斥力表現（パーティクル）
        double pullSpeed;                 // 引き寄せの強さ
        double angle = atan2(dy, dx);     // ボス→自機の向き＝斥力の向き
        pullSpeed = power * 6.0;
        double ratio = 3.0;               // 比例定数
        spawnForceParticles(player.x, player.y,
            pullSpeed * cos(angle) * ratio,
            pullSpeed * sin(angle) * ratio);

        // 火花弾（接触点から垂直方向の両側へ）
        if (t % 7 == 0) {
            double cxp = (player.x + enemy.x) * 0.5;
            double cyp = (player.y + enemy.y) * 0.5;
            double perp = angle + DX_PI * 0.5;
            CreateSet(cxp, cyp, perp, 0, ShotSpark);
        }

        // 離れた／時間切れ → 引き技へ
        if (dist > maxR * 1.3 || t > 100) {
            phase = 3;
            phaseStart = count;
        }
        break;
    }

          // --------------------------------------------------------
          //  引き技：中央へ戻りつつ突っ張り弾
          // --------------------------------------------------------
    case 3: {
        double dx = 240.0 - enemy.x;
        double dy = 240.0 - enemy.y;
        double d = sqrt(dx * dx + dy * dy);
        if (d < 4.0) {
            enemy.x = 240.0;
            enemy.y = 240.0;
            phase = 0;
            phaseStart = count;
        }
        else {
            enemy.x += dx / d * 3.0;
            enemy.y += dy / d * 3.0;
        }
        if (t > 0 && t % 15 == 0 && t < 60) {
            double ang = atan2(player.y - enemy.y, player.x - enemy.x);
            CreateSet(enemy.x, enemy.y, ang, 0, ShotTsuppari);
        }
        break;
    }

    } // switch
}