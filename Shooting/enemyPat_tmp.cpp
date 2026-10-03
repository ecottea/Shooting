// enemyPat_tmp.cpp
// 弾幕パターン: 「怯えた兎」
//  - 通常フェーズ: 自機狙いリング弾を撃ちながらゆっくり浮遊
//  - 一定ダメージ(50)ごとに逃走モードへ移行
//    1) 尻尾撃ち   : 移動軌跡上に時限爆発弾を落とす(追うと危険)
//    2) 振り返り撃ち: 急停止→自機狙いの扇状高速度弾→再逃走
//    3) 角撤退     : 画面端に追い詰められると壁から内側へレーザー
//  - 逃走後は息切れして硬直(攻撃チャンス)、復帰時に緊急回避弾
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ヘルパー(リンクリスト処理の共通化)
// ============================================================
// 弾幕セットを生成して連結リストへ登録
static sEnemyShotSet* CreateShotSet(void (*func)(sEnemyShotSet*), double x, double y, double muki)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}
// 弾を生成してセットへ登録
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}
// セット内の弾を等速直線移動(共通処理)
static void MoveShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕パターン
// ============================================================
// 通常時: 自機狙いを基準にした全方位リング弾
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        const int NUM = 12;
        for (int i = 0; i < NUM; i++) {
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                pEnemyShotSet->muki + i * DX_PI * 2.0 / NUM,
                2.3 + GetRand(60) / 100.0,
                img_enemyShotSmallBall[0]); // 赤
        }
    }
    MoveShots(pEnemyShotSet);
}
// 尻尾撃ち: しばらく漂った後、自機へ向かって高速化する時限爆発弾
static void ShotTailBomb(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = AddShot(pEnemyShotSet,
                pEnemyShotSet->x + GetRand(30) - 15,
                pEnemyShotSet->y + GetRand(30) - 15,
                GetRand(628) / 100.0, // 0〜2πのランダムな向き
                0.6,
                img_enemyShotSmallBall[1]); // 黄: 爆発前の予兆弾
            pShot->param_i[0] = 45 + GetRand(20); // 導火時間(フレーム)
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] > 0) {
            pShot->param_i[0]--;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->param_i[0] == 0) {
                // 爆発: 自機へ向かって高速化し、見た目も変更
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 4.0;
                pShot->kind = img_enemyShotMediumBall[0]; // 赤: 爆発弾
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}
// 振り返り撃ち: 自機狙いの扇状高速度弾(銃弾)
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        const int NUM = 7;
        for (int i = 0; i < NUM; i++) {
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                pEnemyShotSet->muki + (i - (NUM - 1) / 2) * 7.0 * DX_PI / 180.0,
                5.5,
                img_enemyShotBullet[0]); // 赤
        }
    }
    MoveShots(pEnemyShotSet);
}
// 角撤退: 壁から内側へ押し出すように水平に走るレーザー
static void ShotWallLaser(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3; i++) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y + (i - 1) * 40.0,
                pEnemyShotSet->muki, 3.5,
                img_enemyShotLaser[4]); // 青
        }
    }
    MoveShots(pEnemyShotSet);
}
// 緊急回避弾: 硬直復帰の瞬間に全方向へゆっくりばら撒く
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        const int NUM = 16;
        for (int i = 0; i < NUM; i++) {
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                i * DX_PI * 2.0 / NUM, 1.8,
                img_enemyShotSmallBall[8]); // 橙
        }
    }
    MoveShots(pEnemyShotSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    // phase: 0=通常 1=逃走 2=息切れ硬直
    static int phase;
    static int phaseCount;
    static int lastHp;
    static double targetX, targetY;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        phaseCount = 0;
        lastHp = enemy.hp;
        targetX = enemy.x;
        targetY = enemy.y;
        return;
    }

    phaseCount++;

    switch (phase) {
    case 0: // ---------- 通常フェーズ ----------
        // ゆっくり左右に浮遊しながらリング弾
        enemy.x = 240.0 + sin(phaseCount * 0.03) * 60.0;
        enemy.y = 40.0 + sin(phaseCount * 0.05) * 10.0;

        if (phaseCount % 60 == 1) {
            CreateShotSet(ShotRing, enemy.x, enemy.y + 10.0,
                atan2(player.y - (enemy.y + 10.0), player.x - enemy.x));
        }

        // 一定ダメージ(50)で逃走モードへ移行
        if (enemy.hp <= lastHp - 50) {
            lastHp = enemy.hp;
            phase = 1;
            phaseCount = 0;
            // 逃走開始の予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;

    case 1: // ---------- 逃走フェーズ ----------
    {
        // 60フレームごとにランダムな目標点へ方向転換(=ジグザグ移動)
        if (phaseCount % 60 == 1) {
            targetX = 60.0 + GetRand(360); // 60〜420
            targetY = 40.0 + GetRand(240); // 40〜280
        }
        // 振り返り撃ちの前後は急停止(怯えて立ち止まる)
        int cycle = phaseCount % 90;
        bool stop = (cycle >= 50 && cycle < 65);

        if (!stop) {
            double dx = targetX - enemy.x;
            double dy = targetY - enemy.y;
            double len = sqrt(dx * dx + dy * dy);
            if (len > 2.0) {
                enemy.x += 4.0 * dx / len;
                enemy.y += 4.0 * dy / len;
            }
            // 怯え表現の小刻みな震え
            enemy.x += (GetRand(3) - 1) * 0.5;
            enemy.y += (GetRand(3) - 1) * 0.5;
        }

        // 尻尾撃ち: 移動中は常に後方へ時限爆発弾を落とす
        if (!stop && phaseCount % 8 == 0) {
            CreateShotSet(ShotTailBomb, enemy.x, enemy.y + 10.0, 0.0);
        }
        // 振り返り撃ち: 急停止した瞬間に自機狙いの扇弾
        if (cycle == 50) {
            CreateShotSet(ShotFan, enemy.x, enemy.y + 10.0,
                atan2(player.y - (enemy.y + 10.0), player.x - enemy.x));
        }
        // 角撤退: 画面端に追い詰められたら壁から内側へレーザー
        if ((enemy.x < 70.0 || enemy.x > 410.0) && phaseCount % 40 == 0) {
            double muki = (enemy.x < 240.0) ? 0.0 : DX_PI; // 内側へ向く水平方向
            CreateShotSet(ShotWallLaser, enemy.x, enemy.y, muki);
        }
        // 逃走時間が経過したら息切れして硬直
        if (phaseCount >= 240) {
            phase = 2;
            phaseCount = 0;
        }
        break;
    }
    case 2: // ---------- 息切れ硬直(最大の攻撃チャンス) ----------
        // 息を切らしてその場で震える
        enemy.x += (GetRand(3) - 1) * 0.3;
        enemy.y += (GetRand(3) - 1) * 0.3;

        // 約2.5秒後、復帰の瞬間に緊急回避弾を全方向へ
        if (phaseCount >= 150) {
            CreateShotSet(ShotBurst, enemy.x, enemy.y, 0.0);
            phase = 0;
            phaseCount = 0;
            lastHp = enemy.hp; // 硬直中に削った分は次の逃走トリガーに含めない
        }
        break;
    }
}