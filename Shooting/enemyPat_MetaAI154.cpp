#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

static sEnemyShotSet* g_pDohyoSet = nullptr;

// 前方宣言
static void ShotDohyoBorder(sEnemyShotSet* pSet);
static void ShotShikoShock(sEnemyShotSet* pSet);
static void ShotTuppariFan(sEnemyShotSet* pSet);
static void ShotChargeWarning(sEnemyShotSet* pSet);

// 共通: ShotSet生成ヘルパー
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, void(*func)(sEnemyShotSet*), int kind = 0)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;
    pSet->patternFunc = func;
    for (int i = 0; i < 16; i++) { pSet->param_i[i] = 0; pSet->param_d[i] = 0.0; }
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// 土俵 = 画面いっぱいの俵壁
static void ShotDohyoBorder(sEnemyShotSet* pSet)
{
    const int perSide = 28;
    if (pSet->count == 0) {
        pSet->param_d[0] = 0.0; // shrink量
        pSet->param_d[1] = 0.055; // shrink速度
        g_pDohyoSet = pSet;

        // 4辺生成
        for (int side = 0; side < 4; side++) {
            for (int i = 0; i < perSide; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                double t = (double)i / (double)(perSide - 1); // 0-1
                pShot->param_i[0] = side;
                pShot->param_d[0] = t;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->count = 0;
                pShot->kind = img_enemyShotMediumBall[6]; // 白 中玉 = 俵
                pShot->margin = 100.0; // 壁なので消さない
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 収縮更新
    pSet->param_d[0] += pSet->param_d[1];
    if (pSet->param_d[0] > 220.0) pSet->param_d[0] = 220.0;

    double sh = pSet->param_d[0];
    double left = 2.0 + sh;
    double right = 478.0 - sh;
    double top = 2.0 + sh;
    double bottom = 478.0 - sh;

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double t = p->param_d[0];
        int side = p->param_i[0];
        if (side == 0) { // 左
            p->x = left;
            p->y = top + t * (bottom - top);
        }
        else if (side == 1) { // 右
            p->x = right;
            p->y = top + t * (bottom - top);
        }
        else if (side == 2) { // 上
            p->y = top;
            p->x = left + t * (right - left);
        }
        else { // 下
            p->y = bottom;
            p->x = left + t * (right - left);
        }
        // 俵が波打く演出: 敵が近いと揺れる
        double ddx = p->x - enemy.x;
        double ddy = p->y - enemy.y;
        double dist = sqrt(ddx * ddx + ddy * ddy);
        if (dist < 80.0) {
            p->x += (GetRand(20) - 10) / 20.0;
            p->y += (GetRand(20) - 10) / 20.0;
        }
        p = p->next;
    }
}

// 四股の衝撃波
static void ShotShikoShock(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 32; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = (2.0 * DX_PI / 32.0) * i + GetRand(10) / 100.0;
            pShot->speed = 2.0 + GetRand(80) / 100.0;
            pShot->count = 0;
            pShot->kind = img_enemyShotSmallBall[1]; // 黄 小玉 = 砂埃
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
    if (pSet->count > 120) {
        sEnemyShot* q = pSet->pEnemyShotHead->next;
        while (q != pSet->pEnemyShotHead) { sEnemyShot* n = q->next; delete q; q = n; }
        delete pSet->pEnemyShotHead;
        pSet->prev->next = pSet->next;
        pSet->next->prev = pSet->prev;
        delete pSet;
    }
}

// 突っ張り扇
static void ShotTuppariFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        double centerAng = pSet->muki; // 土俵中央への向き
        for (int i = 0; i < 22; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double spread = (GetRand(120) - 60) / 180.0 * DX_PI; // ±60度
            // 俵側から内側へ撃つので 180度反転
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = centerAng + spread;
            pShot->speed = 3.0 + GetRand(200) / 100.0;
            pShot->count = 0;
            // 中楕円 橙
            pShot->kind = img_enemyShotMediumOval[8];
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p->speed += 0.02;
        p = p->next;
    }
    if (pSet->count > 90) {
        sEnemyShot* q = pSet->pEnemyShotHead->next;
        while (q != pSet->pEnemyShotHead) { sEnemyShot* n = q->next; delete q; q = n; }
        delete pSet->pEnemyShotHead;
        pSet->prev->next = pSet->next;
        pSet->next->prev = pSet->prev;
        delete pSet;
    }
}

// 突進予告レーザー
static void ShotChargeWarning(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki;
        pShot->speed = 0.0;
        pShot->count = 0;
        pShot->kind = img_enemyShotLaser[0]; // 赤 短レーザー
        pShot->margin = 1000.0;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x = enemy.x;
        p->y = enemy.y;
        p->muki = pSet->muki;
        p = p->next;
    }
}

// ================= ボス本体 =================
void EnemyPat_Sumou_MetaAI()
{
    static int state; // 0:中央へ 1:四股待機 2:予告 3:突進 4:スタン
    static int stateTimer;
    static double chargeMuki;
    static double chargeTargetX, chargeTargetY;

    const double REPEL_DIST = 75.0;
    const double DOHYO_MARGIN = 18.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        state = 0;
        stateTimer = 0;
        g_pDohyoSet = nullptr;

        // 土俵生成
        CreateShotSet(240.0, 240.0, 0.0, ShotDohyoBorder, 0);
    }

    // --- 斥力判定: ボスと自機が近いときのみ ---
    {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < REPEL_DIST && dist > 1.0) {
            // 指定のコードを必ず含める
            double pullSpeed; // 引き寄せの強さ
            double angle; // 引き寄せの向き
            double ratio = 3.0; // 比例定数
            angle = atan2(dy, dx); // 敵→自機方向
            pullSpeed = (REPEL_DIST - dist) * 0.28; // 近いほど強い
            spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);

            // 実際の斥力適用: 自機を外へ弾く、敵は少し後退
            double pushP = pullSpeed * 1.6;
            double pushE = pullSpeed * 0.35;
            player.x += pushP * cos(angle);
            player.y += pushP * sin(angle);
            enemy.x -= pushE * cos(angle);
            enemy.y -= pushE * sin(angle);

            // 近くの砂弾を吹き飛ばす演出はパーティクルで代用
        }
    }

    // 土俵の現在サイズ取得
    double sh = 0.0;
    if (g_pDohyoSet) sh = g_pDohyoSet->param_d[0];
    double left = 2.0 + sh;
    double right = 478.0 - sh;
    double top = 2.0 + sh;
    double bottom = 478.0 - sh;

    // 自機を土俵内にクランプ(壁は弾なので当たり判定は別だが押し出しで埋まらないように)
    if (player.x < left + 8.0) player.x = left + 8.0;
    if (player.x > right - 8.0) player.x = right - 8.0;
    if (player.y < top + 8.0) player.y = top + 8.0;
    if (player.y > bottom - 8.0) player.y = bottom - 8.0;

    stateTimer++;

    switch (state) {
    case 0: // 中央へ移動
    {
        double tx = 240.0, ty = 240.0;
        double ang = atan2(ty - enemy.y, tx - enemy.x);
        enemy.x += 2.2 * cos(ang);
        enemy.y += 2.2 * sin(ang);
        if (fabs(enemy.x - tx) < 3.0 && fabs(enemy.y - ty) < 3.0) {
            state = 1; stateTimer = 0;
        }
    }
    break;

    case 1: // 四股・仕切り
        if (stateTimer % 60 == 1) {
            CreateShotSet(enemy.x, enemy.y, 0.0, ShotShikoShock, 0);
        }
        if (stateTimer > 140) {
            // 予告へ
            chargeTargetX = player.x;
            chargeTargetY = player.y;
            chargeMuki = atan2(chargeTargetY - enemy.y, chargeTargetX - enemy.x);
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            CreateShotSet(enemy.x, enemy.y, chargeMuki, ShotChargeWarning, 0);
            state = 2; stateTimer = 0;
        }
        break;

    case 2: // 予告
        if (stateTimer > 50) {
            // 予告レーザー削除
            sEnemyShotSet* p = enemyShotSetHead.next;
            while (p != &enemyShotSetHead) {
                sEnemyShotSet* nxt = p->next;
                if (p->patternFunc == ShotChargeWarning) {
                    sEnemyShot* q = p->pEnemyShotHead->next;
                    while (q != p->pEnemyShotHead) { sEnemyShot* n = q->next; delete q; q = n; }
                    delete p->pEnemyShotHead;
                    p->prev->next = p->next;
                    p->next->prev = p->prev;
                    delete p;
                }
                p = nxt;
            }
            state = 3; stateTimer = 0;
        }
        break;

    case 3: // 突進
    {
        double spd = 9.0;
        enemy.x += spd * cos(chargeMuki);
        enemy.y += spd * sin(chargeMuki);

        // 張り手: 正面からの自機弾を無効化するイメージで、前方に小さな当たり判定で弾消し(演出)
        // ここでは省略: ダメージは背面のみ通るという仕様はplayer側で判定する想定

        bool hitWall = false;
        double hitX = enemy.x, hitY = enemy.y;
        double toCenter = 0.0;
        if (enemy.x <= left + DOHYO_MARGIN) { hitWall = true; hitX = left; hitY = enemy.y; toCenter = 0.0; }
        else if (enemy.x >= right - DOHYO_MARGIN) { hitWall = true; hitX = right; hitY = enemy.y; toCenter = DX_PI; }
        else if (enemy.y <= top + DOHYO_MARGIN) { hitWall = true; hitX = enemy.x; hitY = top; toCenter = DX_PI * 0.5; }
        else if (enemy.y >= bottom - DOHYO_MARGIN) { hitWall = true; hitX = enemy.x; hitY = bottom; toCenter = -DX_PI * 0.5; }

        if (hitWall || stateTimer > 70) {
            // 衝突で俵を崩す & 突っ張り
            sEnemyShotSet* fan = CreateShotSet(hitX, hitY, 0.0, ShotTuppariFan, 0);
            fan->muki = atan2(240.0 - hitY, 240.0 - hitX); // 中央へ向ける
            // 俵の該当箇所を少し外へ押し戻す演出
            if (g_pDohyoSet && g_pDohyoSet->param_d[0] > 10.0) {
                g_pDohyoSet->param_d[0] -= 12.0;
            }
            state = 4; stateTimer = 0;
        }
    }
    break;

    case 4: // スタン: 背面がガラ空き
        if (stateTimer > 65) {
            state = 1; stateTimer = 0;
        }
        break;
    }
}