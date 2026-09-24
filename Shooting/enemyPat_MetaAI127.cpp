#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------------------
// ガブリエルのラッパ - ベル部分のワイヤーフレーム
//------------------------------------------------------------
static void ShotGabrielBell(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const int rings = 5;
        for (int r = 0; r < rings; r++) {
            double baseRadius = 14.0 + r * 16.0; // 奥 14 -> 手前 78
            double yOffset = r * 18.0;
            int num = 12 + r * 4; // 奥は疎、手前は密
            double twist = r * 0.18;

            for (int j = 0; j < num; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double ang = 2.0 * DX_PI * j / num + twist;

                pEnemyShot->x = pEnemyShotSet->x + baseRadius * cos(ang);
                // Yは0.35倍に潰して楕円遠近にする
                pEnemyShot->y = pEnemyShotSet->y + yOffset + baseRadius * 0.35 * sin(ang);
                pEnemyShot->muki = ang; // 半径方向に膨張
                pEnemyShot->speed = 0.12 + r * 0.05;

                // 奥ほど暗い色で立体感
                if (r <= 1) pEnemyShot->kind = img_enemyShotMediumOval[0]; // 赤
                else if (r == 2) pEnemyShot->kind = img_enemyShotMediumOval[1]; // 黄
                else pEnemyShot->kind = img_enemyShotMediumOval[6]; // 白ハイライト

                pEnemyShot->param_i[0] = r; // リング番号
                pEnemyShot->param_d[0] = ang; // 元の角度
                pEnemyShot->param_d[1] = baseRadius;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 中心のチャージ大玉
        {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[6];
            pEnemyShot->param_i[0] = 999; // 識別用
            pEnemyShot->param_d[2] = 0.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 999) {
            // チャージ玉は脈動
            pShot->param_d[2] += 0.08;
            // 少しだけ下に追従
            pShot->y += 0.15;
        }
        else {
            // ゆっくり回転させて首振り立体感
            pShot->param_d[0] += 0.012;
            pShot->muki = pShot->param_d[0];
            // 遠近を保ったまま膨張 + 全体を少し下へ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki) * 0.35 + 0.18;
        }
        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 筒部分 - 4本のレーザーで側面を表現
//------------------------------------------------------------
static void ShotGabrielTube(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int k = 0; k < 4; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double ang = k * DX_PI / 2.0; // 0,90,180,270度
            double r = 13.0;
            pEnemyShot->x = pEnemyShotSet->x + r * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + r * 0.35 * sin(ang);
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.55;
            pEnemyShot->kind = img_enemyShotLaser[1]; // 黄で真鍮
            pEnemyShot->param_i[0] = k;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki) * 0.1; // ほぼ垂直落下
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 主砲 - ラッパの奔流
//------------------------------------------------------------
static void ShotGabrielMain(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 36; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // -35度〜+35度の扇
            double spread = (GetRand(70) - 35) / 180.0 * DX_PI;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(10) - 5;
            pEnemyShot->y = pEnemyShotSet->y + 80.0; // ベル先端から
            pEnemyShot->muki = DX_PI / 2.0 + spread;
            pEnemyShot->speed = 2.2 + GetRand(150) / 100.0;
            // 白コアと橙アウターを交互に
            if (i % 2 == 0) pEnemyShot->kind = img_enemyShotSmallBall[6];
            else pEnemyShot->kind = img_enemyShotSmallBall[8];

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
        // 手前に来るほど加速して立体感
        if (pShot->count > 20) pShot->speed += 0.02;
        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 倍音リング - ベルから広がる衝撃波
//------------------------------------------------------------
static void ShotGabrielRingWave(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int num = 24;
        double baseSpeed = pEnemyShotSet->param_d[0]; // リングごとに速度変える
        if (baseSpeed < 0.1) baseSpeed = 2.0;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double ang = 2.0 * DX_PI * i / num;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = baseSpeed + GetRand(30) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄
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

//------------------------------------------------------------
// 音符弾 - 3つで1音符のクラスター、サイン波で飛ぶ
//------------------------------------------------------------
static void ShotGabrielNote(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1つの音符を3弾で構成
        for (int n = 0; n < 3; n++) {
            for (int k = 0; k < 3; k++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double offsetX = (k == 0 ? -6 : k == 1 ? 6 : 0);
                double offsetY = (k == 2 ? -14 : 0);
                pEnemyShot->x = pEnemyShotSet->x + offsetX + n * 18.0 - 18.0;
                pEnemyShot->y = pEnemyShotSet->y + offsetY;
                pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(20) - 10) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.8 + GetRand(40) / 100.0;
                if (k == 2) pEnemyShot->kind = img_enemyShotScale[4]; // 符幹
                else pEnemyShot->kind = img_enemyShotSmallBall[4]; // 符頭(青)

                pEnemyShot->param_d[0] = GetRand(100) / 100.0 * DX_PI; // サイン波位相
                pEnemyShot->param_d[1] = (k == 2 ? 1 : 0); // 幹フラグ

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double wave = sin(pShot->count * 0.12 + pShot->param_d[0]) * 0.9;
        pShot->x += pShot->speed * cos(pShot->muki) + wave;
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_GabrielsHorn_MetaAI()
{
    static double baseX;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        baseX = 240.0;
    }
    else {
        // ラッパを構えて左右にスイング、立体感を強調
        baseX += sin(count * 0.015) * 0.4;
        enemy.x = baseX + sin(count * 0.03) * 18.0;
        enemy.y = 60.0 + cos(count * 0.02) * 6.0;
    }

    int seq = count % 300;

    // 1. ベル形成
    if (seq == 10) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGabrielBell;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 2. 筒
    if (seq == 20) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGabrielTube;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 12.0;
        pSet->muki = DX_PI / 2.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 3. 咆哮主砲 + リング
    if (seq == 120) {
        // 主砲
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotGabrielMain;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 20.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // 倍音リング2重
        for (int w = 0; w < 2; w++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotGabrielRingWave;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 75.0 + w * 15.0;
            pSet->param_d[0] = 1.8 + w * 0.6;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // 4. 音符で追撃
    if (seq == 145 || seq == 175 || seq == 205) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGabrielNote;
        pSet->x = enemy.x + (GetRand(60) - 30);
        pSet->y = enemy.y + 80.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}