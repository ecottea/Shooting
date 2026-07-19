// enemyPat_bubbleSpiral.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：シャボン玉の夢幻螺旋
static void ShotBubbleSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音を再生
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 9個のシャボン玉（大玉）を生成
        for (int i = 0; i < 9; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 初期角度をランダムに設定
            double angle = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;

            // 大玉、色はシアン(3)、ピンク(5)、金色(8)からランダムに選択
            int color = 3 + (GetRand(2) * 2);
            if (color > 8) color = 8;
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            // 膨張率
            pEnemyShot->param_d[0] = 0.05 + GetRand(10) / 200.0;
            // 螺旋の回転速度
            pEnemyShot->param_d[1] = (GetRand(20) - 10) / 1000.0;
            // プレイヤーの位置を保存
            pEnemyShot->param_d[2] = player.x;
            pEnemyShot->param_d[3] = player.y;
            // 割れるタイミング
            pEnemyShot->param_i[0] = 60 + GetRand(60);
            // 割れたかどうかのフラグ
            pEnemyShot->param_i[1] = 0;

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pEnemyShot->next;

        // 割れるタイミングに達したら
        if (pEnemyShot->count >= pEnemyShot->param_i[0] && pEnemyShot->param_i[1] == 0) {
            pEnemyShot->param_i[1] = 1;

            // 小さな弾を8方向にばら撒く
            for (int j = 0; j < 8; j++) {
                sEnemyShot* newShot = new sEnemyShot;
                newShot->x = pEnemyShot->x;
                newShot->y = pEnemyShot->y;
                newShot->muki = pEnemyShot->muki + (j * 45.0 / 180.0 * DX_PI);
                newShot->speed = 2.0 + GetRand(10) / 10.0;
                newShot->kind = img_enemyShotSmallBall[6]; // 小玉、白色
                newShot->param_i[1] = 1;

                newShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                newShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = newShot;
                pEnemyShotSet->pEnemyShotHead->prev = newShot;
            }
        }

        if (pEnemyShot->param_i[1] == 0) {
            // 膨張率で速度を増加
            pEnemyShot->speed += pEnemyShot->param_d[0];

            // 螺旋軌道：プレイヤーを中心として回転
            double dx = pEnemyShot->param_d[2] - pEnemyShot->x;
            double dy = pEnemyShot->param_d[3] - pEnemyShot->y;
            double distance = sqrt(dx * dx + dy * dy);
            if (distance > 0) {
                double target_angle = atan2(dy, dx);
                pEnemyShot->muki = target_angle + pEnemyShot->param_d[1] * pEnemyShot->count;
            }

            // 移動
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = nextShot;
    }
}

// 敵本体のパターン
void EnemyPat_SoapBubbles_Vibe()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右に移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60フレームごとに弾幕を発射
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}