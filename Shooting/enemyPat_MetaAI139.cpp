#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 台風弾幕「暴風域と静かな目」
// ============================================================

// --------------- 1. アイウォール (目の壁) ---------------
static void ShotTyphoonEyeWall(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const int WALL_NUM = 24;
    const double EYE_MIN_R = 45.0;
    const double EYE_MAX_R = 170.0;

    if (pSet->count == 0) {
        // 半径と状態初期化
        // param_d[0]: 現在半径, param_i[0]: 0=膨張 1=停止 2=収縮, param_i[1]: 停止タイマー
        pSet->param_d[0] = EYE_MIN_R;
        pSet->param_i[0] = 0;
        pSet->param_i[1] = 90; // 停止フレーム

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < WALL_NUM; i++) {
            pShot = new sEnemyShot;
            double angle = 2.0 * DX_PI / WALL_NUM * i;
            pShot->param_d[0] = angle; // 角度を保存
            pShot->x = pSet->x + pSet->param_d[0] * cos(angle);
            pShot->y = pSet->y + pSet->param_d[0] * sin(angle);
            pShot->muki = angle + DX_PI / 2.0; // 接線方向に寝かせて雲の壁に見せる
            pShot->speed = 0.0;
            // 中楕円弾・白で雲
            pShot->kind = img_enemyShotMediumOval[6];
            pShot->margin = 240.0; // 画面外判定を少し緩める

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 半径の更新（Set側で一元管理）
    if (pSet->param_i[0] == 0) { // 膨張
        pSet->param_d[0] += 1.2;
        if (pSet->param_d[0] >= EYE_MAX_R) {
            pSet->param_i[0] = 1;
        }
    }
    else if (pSet->param_i[0] == 1) { // 停止
        pSet->param_i[1]--;
        if (pSet->param_i[1] <= 0) pSet->param_i[0] = 2;
    }
    else if (pSet->param_i[0] == 2) { // 収縮
        pSet->param_d[0] -= 0.9;
    }

    // ボスに追従させる
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    // 各弾を半径に追従
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;
        double ang = p->param_d[0];
        p->x = pSet->x + pSet->param_d[0] * cos(ang);
        p->y = pSet->y + pSet->param_d[0] * sin(ang);
        p->muki += 0.008; // 少しずつ回転して渦っぽく

        // 中心まで戻ったら消去（画面外削除では消えないため手動消去）
        if (pSet->param_i[0] == 2 && pSet->param_d[0] <= 38.0) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        p = pNext;
    }
}

// --------------- 2. レインバンド (螺旋の雨) ---------------
static void ShotTyphoonRainBand(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        // pSet->param_d[0] に親から渡された基底回転角が入っている
        double baseRot = pSet->param_d[0];
        // 5本腕
        for (int i = 0; i < 5; i++) {
            pShot = new sEnemyShot;
            double angle = baseRot + 2.0 * DX_PI / 5.0 * i;
            pShot->x = pSet->x + cos(angle) * 8.0;
            pShot->y = pSet->y + sin(angle) * 8.0;
            pShot->muki = angle;
            pShot->speed = 1.8 + GetRand(40) / 100.0; // 1.8〜2.2
            // 青とシアンをランダムで雨っぽく
            int col = (GetRand(1) == 0) ? 4 : 3;
            pShot->kind = img_enemyShotSmallBall[col];
            // カーブ量を保存 0.008〜0.018
            pShot->param_d[0] = 0.008 + GetRand(10) / 1000.0;
            // 3本に1本は逆回転にして乱流表現
            if (GetRand(2) == 0) pShot->param_d[0] *= -0.6;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        // 雨は音を毎回鳴らすとウルサイので20%だけ
        if (GetRand(4) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        // 螺旋を描くように少しずつ曲げる
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------- 3. 突風 (高速針弾) ---------------
static void ShotTyphoonGust(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 8方向 x 3way = 24発
        for (int dir = 0; dir < 8; dir++) {
            double base = 2.0 * DX_PI / 8.0 * dir + pSet->param_d[0]; // param_d[0]で少しずらす
            for (int w = -1; w <= 1; w++) {
                sEnemyShot* pShot = new sEnemyShot;
                double angle = base + w * 0.18; // 約10度ずらし
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = angle;
                pShot->speed = 6.0;
                pShot->kind = img_enemyShotBullet[6]; // 銃弾・白で風の筋
                pShot->param_d[0] = 0.0; // 減速フラグ用

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->count < 16) {
            // 最初は超高速
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        else {
            // 急減速してレインバンドに同化
            if (p->speed > 2.0) p->speed *= 0.92;
            if (p->speed < 2.0) p->speed = 2.0;
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// --------------- 敵本体 ---------------
void EnemyPat_Typhoon_MetaAI()
{
    static double spiral_rot;
    static double gust_rot;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        spiral_rot = 0.0;
        gust_rot = 0.0;
    }
    else {
        // 台風らしくゆっくり左右に漂う
        enemy.x = 240.0 + sin(count * 0.015) * 70.0;
        enemy.y = 40.0 + sin(count * 0.01) * 6.0;
    }

    // 1) 常時：レインバンド 5フレーム毎
    if (count % 5 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTyphoonRainBand;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->param_d[0] = spiral_rot; // 基底角
        spiral_rot += 0.18; // 毎回約10度回す
        if (spiral_rot > DX_TWO_PI) spiral_rot -= DX_TWO_PI;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 2) アイウォール 300フレーム毎 (約5秒)
    if (count % 300 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTyphoonEyeWall;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 3) 突風 240フレーム毎
    if (count % 240 == 150) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTyphoonGust;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->param_d[0] = gust_rot;
        gust_rot += 0.35; // 少しずらして毎回隙間を変える
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}