// enemyPat_tmp.cpp
// 弾幕：ヒマワリ ‐ 向日葵の輪 ‐
//   フェーズ1「種まき」：黒の銃弾（タネ）7発を自機方向へ扇状に発射、減速してリング上に静止
//   フェーズ2「開花」  ：静止したタネの周りに黄色の小玉（花びら）を展開＋自機狙いの橙の大玉（2回分裂）
//   フェーズ3「向日」  ：花全体が回転しながら徐々に加速し、最後に画面外へ流れていく
//   8秒周期で繰り返し、繰り返すごとにタネの静止半径を詰めて花の密度を上げる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// リンクリスト末尾（headの直前）へ弾を1発追加するヘルパー
static sEnemyShot* CreateEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 弾幕本体
static void ShotSunflower(sEnemyShotSet* pSet)
{
    const int    cnt = pSet->count;
    const double cx = pSet->x;                     // 花（回転）の中心
    const double cy = pSet->y;
    const double targetR = max(360.0 - pSet->kind * 15.0, 50.0);        // タネの静止半径
    const double dir = (pSet->kind % 2 == 0) ? 1.0 : -1.0; // 回転方向は発射ごとに交互

    // ---------- フェーズ1：種まき ----------
    if (cnt == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 7; i++) {
            // 自機方向へ扇状（中央1発＋両側±36°まで12°刻み）
            double ang = pSet->muki + (i - 3) * (12.0 / 180.0 * DX_PI);
            sEnemyShot* p = CreateEnemyShot(pSet, cx, cy, ang, 3.5 * targetR / 120.0, img_enemyShotBullet[7]); // 黒の銃弾＝タネ
            p->param_i[0] = 0;                    // 0:飛行中のタネ
            p->param_d[2] = targetR;              // 静止目標半径
            p->param_d[3] = 3.5 * 3.5 / (2.0 * targetR); // 減速度（targetRでちょうど止まる等減速）
            p->margin = 120;
        }
    }

    // 開花の予告音
    //if (cnt == 75) {
    //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    // ---------- フェーズ2：開花 ----------
    if (cnt == 90) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 静止したタネの周りに花びら（黄色の小玉）を6発ずつ咲かせる
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double sa = pShot->param_d[0];   // タネの中心からの角度
                double sr = pShot->param_d[1];   // タネの中心からの半径
                for (int k = 0; k < 7; k++) {
                    double pang = sa + (k - 3) * 0.021;       // タネを中心に扇状に並べる
                    double prad = sr + 12.0 + (k % 2) * 10.0; // 二段構えで花びらの厚みを出す
                    sEnemyShot* p = CreateEnemyShot(pSet,
                        cx + cos(pang) * prad, cy + sin(pang) * prad,
                        pang, 0.0, img_enemyShotSmallBall[1]);  // 黄色の小玉＝花びら
                    p->param_i[0] = 2;           // 2:回転する花びら
                    p->param_d[0] = pang;        // 中心からの角度
                    p->param_d[1] = prad;        // 中心からの半径
                }
            }
            pShot = pShot->next;
        }

        // 花の中心：自機狙いのゆっくりした橙の大玉
        double ang = atan2(player.y - cy, player.x - cx);
        sEnemyShot* p = CreateEnemyShot(pSet, cx, cy, ang, 1.1, img_enemyShotLargeBall[8]);
        p->param_i[0] = 5;                       // 5:分裂する大玉
    }

    // ---------- 毎フレーム更新 ----------
    // フェーズ3用の回転パラメータ（回転は徐々に加速）
    double t = cnt - 90;
    if (t < 0.0)   t = 0.0;
    if (t > 210.0) t = 210.0;
    const double rotSpeed = dir * (0.006 + 0.010 * (t / 210.0)); // 回転角速度
    const double petalGrowth = 0.04 + 0.10 * (t / 210.0);           // 花びらのゆっくりした外への広がり

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {

        case 0: { // タネ：飛行 → 減速してリング上に静止
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed -= pShot->param_d[3];
            double dx = pShot->x - cx;
            double dy = pShot->y - cy;
            double dist = sqrt(dx * dx + dy * dy);
            if (pShot->speed <= 0.0 || dist >= pShot->param_d[2]) {
                pShot->speed = 0.0;
                pShot->param_i[0] = 1;               // 静止
                pShot->param_d[0] = atan2(dy, dx);   // 中心からの角度を記憶
                pShot->param_d[1] = dist;            // 中心からの半径を記憶
            }
            break;
        }

        case 1:   // タネ：静止 → 回転 → 放出
        case 2: { // 花びら：回転 → 放出
            if (cnt < 300) {
                if (cnt >= 90) { // フェーズ3「向日」：回転（タネは半径固定、花びらはゆっくり広がる）
                    pShot->param_d[0] += rotSpeed;
                    pShot->param_d[1] += (pShot->param_i[0] == 2) ? petalGrowth : 0.0;
                }
                pShot->x = cx + cos(pShot->param_d[0]) * pShot->param_d[1];
                pShot->y = cy + sin(pShot->param_d[0]) * pShot->param_d[1];
            }
            else {
                // 放出：外向き＋接線方向へ飛ばして画面外へ流す（メインルーチンで消去される）
                pShot->muki = pShot->param_d[0] + dir * 0.6;
                pShot->speed = 2.2 + pSet->kind * 0.15;   // 繰り返しごとに少し速く
                pShot->param_i[0] = 3;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;
        }

        case 3: // 自由飛行（画面外へ）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case 5: { // 大玉：進行しながら2回分裂（橙の小玉を4発ずつ垂らす）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count == 60 || pShot->count == 130) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                for (int j = 0; j < 4; j++) {
                    double ang = pShot->muki + (j * 90 + 45) / 180.0 * DX_PI;
                    sEnemyShot* p = CreateEnemyShot(pSet, pShot->x, pShot->y, ang, 1.9, img_enemyShotSmallBall[8]);
                    p->param_i[0] = 3;
                }
            }
            break;
        }
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sunflower_Zai()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 110.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        shot_count = 0;
    }
    else {
        // 花の形が崩れないよう、中心は大きく動かさずゆっくり揺れるだけにする
        enemy.x = 240.0 + 30.0 * sin(count * 0.008);
        enemy.y = 110.0 + 6.0 * sin(count * 0.015);
    }

    // 8秒（480フレーム）周期で弾幕セットを発射
    if (count % 60 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSunflower;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x); // タネ撒きは自機狙い
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}