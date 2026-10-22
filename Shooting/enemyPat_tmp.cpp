#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  雪の結晶弾幕 - 1つの結晶 = 1つの ShotSet
//  使う素材:
//   中玉(7.0x7.0) 白  : 結晶の中心核  img_enemyShotMediumBall[6]
//   銃弾(5.0x2.0) 青  : 主枝6本(針が枝に見える) img_enemyShotBullet[4]
//   小玉(2.5x2.5) シアン: 第一分岐12本 img_enemyShotSmallBall[3]
//   菱形弾(4.5x2.5) 白: 第二分岐12本・先端の煌めき img_enemyShotDiamond[6]
//  色: 3=シアン, 4=青, 6=白 で雪っぽさを表現
// ============================================================
static void ShotSnowCrystal(sEnemyShotSet* pSet)
{
    const double DEG60 = DX_PI / 3.0;                 // 60度
    const double DEG40 = 40.0 / 180.0 * DX_PI;        // 40度
    const double DEG25 = 25.0 / 180.0 * DX_PI;        // 25度

    // ---- 0フレーム目: 中心 + 主枝6本を生成 ----
    if (pSet->count == 0) {
        // パラメータ初期化
        // param_d[0] = 結晶全体のベース回転角
        // param_d[1] = 落下速度
        // param_d[2] = 回転速度
        pSet->param_d[0] = GetRand(359) / 180.0 * DX_PI;          // 0-359度をランダム
        pSet->param_d[1] = 0.60 + GetRand(40) / 100.0;             // 0.60-1.00 ゆっくり落下
        pSet->param_d[2] = 0.004 + GetRand(10) / 10000.0;          // 0.004-0.005 回転
        if (GetRand(1) == 0) pSet->param_d[2] *= -1.0;             // 左右どちらかに回転

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中心核
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotMediumBall[6]; // 中玉・白
            p->count = 0;
            p->param_i[0] = 0; // 0=中心
            p->param_d[0] = 0.0; // 半径0
            p->param_d[1] = 0.0; // 角度オフセット0
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        // 主枝6本
        for (int k = 0; k < 6; k++) {
            double offset = k * DEG60; // 60度刻み
            double radius = 14.0 + GetRand(4); // 14-18 少しバラつかせる
            double ang = pSet->param_d[0] + offset;

            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + radius * cos(ang);
            p->y = pSet->y + radius * sin(ang);
            p->muki = ang; // 針弾は向きが重要
            p->speed = 0.0;
            p->kind = img_enemyShotBullet[4]; // 銃弾(針)・青 5.0x2.0
            p->count = 0;
            p->param_i[0] = 1; // 1=主枝
            p->param_d[0] = radius; // 初期半径を記憶
            p->param_d[1] = offset; // ベースからの角度差を記憶
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    // ---- 18フレーム後: 第一分岐 12本 ----
    else if (pSet->count == 18) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 中心を探す
        sEnemyShot* pCenter = nullptr;
        for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
            if (s->param_i[0] == 0) { pCenter = s; break; }
        }
        if (!pCenter) return;

        double base = pSet->param_d[0];
        for (int k = 0; k < 6; k++) {
            for (int si = 0; si < 2; si++) {
                int s = (si == 0) ? -1 : 1;
                double offset = k * DEG60 + s * DEG40;
                double radius = 28.0 + GetRand(3); // 28-31
                double ang = base + offset;

                sEnemyShot* p = new sEnemyShot;
                p->x = pCenter->x + radius * cos(ang);
                p->y = pCenter->y + radius * sin(ang);
                p->muki = ang;
                p->speed = 0.0;
                p->kind = img_enemyShotSmallBall[3]; // 小玉・シアン
                p->count = 0;
                p->param_i[0] = 2; // 2=第一分岐
                p->param_d[0] = radius;
                p->param_d[1] = offset;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }
    // ---- 36フレーム後: 第二分岐(先端) 12本 ----
    else if (pSet->count == 36) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pCenter = nullptr;
        for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
            if (s->param_i[0] == 0) { pCenter = s; break; }
        }
        if (!pCenter) return;

        double base = pSet->param_d[0];
        for (int k = 0; k < 6; k++) {
            for (int si = 0; si < 2; si++) {
                int s = (si == 0) ? -1 : 1;
                double offset1 = k * DEG60 + s * DEG40;
                double offset2 = offset1 + s * DEG25; // さらに外側へ折り返す
                double radius = 40.0 + GetRand(4); // 40-44
                double ang = base + offset2;

                sEnemyShot* p = new sEnemyShot;
                p->x = pCenter->x + radius * cos(ang);
                p->y = pCenter->y + radius * sin(ang);
                p->muki = ang;
                p->speed = 0.0;
                p->kind = img_enemyShotDiamond[6]; // 菱形弾・白 キラッと光る先端
                p->count = 0;
                p->param_i[0] = 3; // 3=第二分岐
                p->param_d[0] = radius;
                p->param_d[1] = offset2;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // ---- 毎フレームの移動処理: 結晶全体を落下＋回転＋緩やかに膨張 ----
    sEnemyShot* pCenter = nullptr;
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        if (s->param_i[0] == 0) { pCenter = s; break; }
    }
    if (!pCenter) return; // 中心が画面外で消されたら結晶全体終了

    // 中心を落下
    if (pSet->count > 0) {
        pCenter->y += pSet->param_d[1]; // 落下速度
        // ほんの少し左右に揺らすと雪っぽい
        pCenter->x += sin(pSet->count * 0.02) * 0.15;
    }
    // 全体回転を進める
    pSet->param_d[0] += pSet->param_d[2];

    double expansion = pSet->count * 0.045; // 1フレームあたり0.045px膨張

    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        if (s->param_i[0] == 0) continue; // 中心は移動済み
        double ang = pSet->param_d[0] + s->param_d[1]; // ベース+オフセット
        double rad = s->param_d[0] + expansion;
        s->x = pCenter->x + rad * cos(ang);
        s->y = pCenter->y + rad * sin(ang);
        s->muki = ang; // 針弾の向きを外側に
    }
}

// 敵本体のパターン - 雪結晶を定期的に降らせる
void EnemyPat_Tmp()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;
        // 少し上下にも揺れる
        enemy.y = 60.0 + sin(count * 0.015) * 8.0;
    }

    // 85フレーム毎に雪結晶を1枚生成
    // GetRandは0-x inclusiveなので、85間隔で安定して出るように剰余で判定
    if (count % 55 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSnowCrystal;
        pSet->x = enemy.x + (GetRand(60) - 30); // 生成位置に少しバラつき
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x); // 使わないが一応自機方向を入れておく
        pSet->kind = shot_count++;
        pSet->param_i[0] = 0;
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = 0.0;
        pSet->param_d[2] = 0.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}