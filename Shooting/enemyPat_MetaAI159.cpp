#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 1. チャージ演出：斬る前に敵の周りに気が集まる
// ============================================================
static void ShotChargeEffect(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int NUM = 12;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = (2.0 * DX_PI / NUM) * i; // 初期角度
            pEnemyShot->param_d[1] = 45.0 + GetRand(10); // 初期半径
            // 黄色の中玉でチャージっぽく
            pEnemyShot->kind = img_enemyShotMediumBall[1];
            pEnemyShot->margin = 100.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    double centerX = pEnemyShotSet->x;
    double centerY = pEnemyShotSet->y;

    // 30Fかけて縮小しながら回転
    double t = pEnemyShotSet->count / 30.0;
    if (t > 1.0) t = 1.0;
    double radiusRate = 1.0 - t;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double ang = pShot->param_d[0] + pEnemyShotSet->count * 0.15;
        double r = pShot->param_d[1] * radiusRate;
        pShot->x = centerX + r * cos(ang);
        pShot->y = centerY + r * sin(ang);
        pShot->muki = ang + DX_PI / 2.0;

        // 縮小しきったら画面外に飛ばしてメインルーチンに消してもらう
        if (pEnemyShotSet->count > 30) {
            pShot->x = -1000.0;
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 2. 刀本体：銃弾と中玉で1本の刀を組み立てる
// 素材選定理由:
// 銃弾(5.0x2.0)は最も細長く、重ねると日本刀の鋭さが出る -> 刀身に採用 [白:6]
// 中玉(7.0x7.0)は柄の太さに丁度良い -> 柄[黒:7] 鍔[橙:8]に採用
// ============================================================
static void ShotKatana(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int BLADE_LEN = 26; // 刀身の長さ

    if (pEnemyShotSet->count == 0) {
        // --- 刀身 ---
        for (int i = 0; i < BLADE_LEN; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = 12.0 + i * 5.0; // 中心からの距離 r
            pEnemyShot->kind = img_enemyShotBullet[6]; // 白の銃弾
            pEnemyShot->margin = 200.0; // 画面内では消えないように大きめ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // --- 鍔 ---
        {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = 2.0;
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙の中玉
            pEnemyShot->margin = 200.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // --- 柄 (3つ) ---
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = -8.0 - i * 7.0;
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒の中玉
            pEnemyShot->margin = 200.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 刀は常にボスに追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    // 居合の振り：30Fで180度振り抜く。最初が速いイージング
    double startAng = pEnemyShotSet->param_d[0];
    double endAng = pEnemyShotSet->param_d[1];
    double t = pEnemyShotSet->count / 30.0;
    if (t > 1.0) t = 1.0;
    // easeOutCubicで「ビュッ！」と抜ける感じ
    double eased = 1.0 - pow(1.0 - t, 3.0);
    double curAng = startAng + (endAng - startAng) * eased;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->param_d[0];
        pShot->x = pEnemyShotSet->x + r * cos(curAng);
        pShot->y = pEnemyShotSet->y + r * sin(curAng);
        pShot->muki = curAng; // 銃弾の向きを刀の向きに合わせる
        pShot = pShot->next;
    }

    // 40F経ったら残像を消す（画面外に飛ばして自動消去）
    if (pEnemyShotSet->count > 40) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->x = -1000.0;
            p = p->next;
        }
    }
}

// ============================================================
// 3. 斬撃波：刀の軌跡から飛ぶ三日月弾
// 白小玉(2.5x2.5)で斬線、橙鱗弾(4.0x3.0)で火花を表現
// ============================================================
static void ShotSlashWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 三日月状に24発
        const int NUM = 24*5;
        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;
            // 120度の扇状に広げる
            double spread = (DX_PI * 2.0 / 3.0) * ((double)i / (NUM - 1) - 0.5);
            pEnemyShot->muki = pEnemyShotSet->muki + spread;
            // 内側は速く、外側は少し遅くして三日月を保つ
            pEnemyShot->speed = 4.5 - fabs(spread) * 0.8 + GetRand(50+30) / 100.0;
            pEnemyShot->x = pEnemyShotSet->x + cos(pEnemyShot->muki) * 20.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(pEnemyShot->muki) * 20.0;

            // 偶数は斬線、奇数は火花で交互に
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白小玉
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[8]; // 橙鱗弾
            }
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 追加で自機狙いの火花を5発
        for (int i = 0; i < 5*5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙小玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体：居合 -月閃- のループ
// ============================================================
void EnemyPat_Sword_MetaAI()
{
    static double slashAngle;
    static int slashDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        slashDir = 1;
    }
    else {
        // 中央でゆらゆら
        enemy.x = 240.0 + sin(count * 0.02) * 30.0;
        enemy.y = 80.0 + cos(count * 0.015) * 10.0;
    }

    // 300F周期で居合を繰り返す
    int cycle = count % 300;

    if (cycle == 1) {
        // 斬る方向を決定（自機狙い）
        slashAngle = atan2(player.y - enemy.y, player.x - enemy.x);
        slashDir *= -1; // 交互に逆袈裟に

        // チャージエフェクト生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotChargeEffect;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (cycle == 35) {
        // 刀出現・振り
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKatana;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // 振り始め -> 振り終わり。Dirで左右切り替え
        pSet->param_d[0] = slashAngle - DX_PI / 2.0 * slashDir;
        pSet->param_d[1] = slashAngle + DX_PI / 2.0 * slashDir;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    else if (cycle == 52) {
        // 刀が中間まで振れた瞬間に斬撃波を飛ばす
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSlashWave;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = slashAngle;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}