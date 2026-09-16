// enemyPat_Tmp.cpp
// 弾幕：桜と彗星（斑鳩風極性弾幕）
// マゼンタ小玉 = 桜、シアン小玉 = 彗星
// 同じ色の敵弾は対応する小玉に触れると消滅する

#include "gv.h"  // 必要に応じて実際のヘッダーに合わせてください

// -------------------------------------------------------
// 桜弾幕（マゼンタ）: ボス1モチーフ
// -------------------------------------------------------
static void ShotSakura(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 5枚の花びら状に放射しつつ、各花びら内で少し弧を描くように発射
        for (int petal = 0; petal < 5; petal++) {
            double baseAngle = pEnemyShotSet->muki + petal * (DX_PI * 2.0 / 5.0);
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                double offset = (i - 1.5) * 0.18;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + offset;
                pEnemyShot->speed = 2.2 + i * 0.35;
                pEnemyShot->kind = img_enemyShotSmallBall[5];
                pEnemyShot->margin = 20.0;
                // 花びらが少し回転しながら広がるためのパラメータ
                pEnemyShot->param_d[0] = 0.012 * (petal % 2 == 0 ? 1.0 : -1.0);
                pEnemyShot->param_d[1] = pEnemyShot->muki;
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動：花びらがゆっくり回転しながら広がる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[1] += pShot->param_d[0];
        pShot->muki = pShot->param_d[1];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -------------------------------------------------------
// 彗星弾幕（シアン）: ボス2モチーフ
// -------------------------------------------------------
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 彗星の核と長く伸びる尾を表現した複数のストリーム
        for (int stream = 0; stream < 5; stream++) {
            double baseAngle = pEnemyShotSet->muki + (stream - 2) * 0.18;
            // 核となる先頭弾
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 4.2;
            pEnemyShot->kind = img_enemyShotSmallBall[3];
            pEnemyShot->margin = 20.0;
            pEnemyShot->param_d[0] = 0.0; // 尾の減衰用

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 尾を構成する弾を後方に配置し、徐々に遅くなる
            for (int i = 1; i <= 5; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x - cos(baseAngle) * i * 9.0;
                pEnemyShot->y = pEnemyShotSet->y - sin(baseAngle) * i * 9.0;
                pEnemyShot->muki = baseAngle + (GetRand(10) - 5) * 0.01;
                pEnemyShot->speed = 4.2 - i * 0.35;
                pEnemyShot->kind = img_enemyShotSmallBall[3];
                pEnemyShot->margin = 20.0;
                pEnemyShot->param_d[0] = i * 0.002; // わずかな揺らぎ

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動：尾がわずかに揺れながら直進
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki += pShot->param_d[0] * sin(pShot->count * 0.15);
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -------------------------------------------------------
// 極性小玉（自機追従・色切替）
// param_i[0] : 0=マゼンタ, 1=シアン
// -------------------------------------------------------
static void ShotPolarityOrb(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期配置（count==0で一回だけ）
    if (pEnemyShotSet->count == 0) {
        // 自機を取り囲むように8個配置
        for (int i = 0; i < 8 * 4; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = i * (DX_PI * 2.0 / 8.0 / 4);
            pEnemyShot->x = player.x + 70.0 * cos(angle);
            pEnemyShot->y = player.y + 70.0 * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[5];  // 最初はマゼンタ
            pEnemyShot->margin = 999.0;                     // 画面外で消えない

            // 自由パラメータに角度を保存（追従用）
            pEnemyShot->param_d[0] = angle;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[0] = 0;  // 現在マゼンタ
    }

    // 色切替タイミング（1秒後に初回、以降3秒おき）
    // 0秒: charge → 1秒: マゼンタ配置
    // 3秒: charge → 4秒: シアンに変化
    // 6秒: charge → 7秒: マゼンタに変化 ... と繰り返す
    int phase = pEnemyShotSet->count;
    if (phase == 180 || (phase > 180 && (phase - 180) % 180 == 0)) {
        // 色を反転
        pEnemyShotSet->param_i[0] ^= 1;
        int newKind = (pEnemyShotSet->param_i[0] == 0) ? img_enemyShotSmallBall[5] : img_enemyShotSmallBall[3];

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->kind = newKind;
            pShot = pShot->next;
        }
    }

    // 自機追従（円形配置を維持しながら中心を自機に合わせる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        // ゆっくり自機に追従
        pShot->x = player.x + 70.0 * cos(angle);
        pShot->y = player.y + 70.0 * sin(angle);
        pShot = pShot->next;
    }

    // 同じ色の敵弾を消滅させる
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            // 極性小玉自身は対象外
            if (pShot->margin < 900.0) {
                sEnemyShot* pOrb = pEnemyShotSet->pEnemyShotHead->next;
                while (pOrb != pEnemyShotSet->pEnemyShotHead) {
                    if (pShot->kind == pOrb->kind) {
                        double dx = pShot->x - pOrb->x;
                        double dy = pShot->y - pOrb->y;
                        // 小玉半径2.5同士の簡易円判定
                        if (dx * dx + dy * dy < 100) {
                            pShot->prev->next = pShot->next;
                            pShot->next->prev = pShot->prev;
                            delete pShot;
                            break;
                        }
                    }
                    pOrb = pOrb->next;
                }
            }
            pShot = pNext;
        }
        pSet = pSet->next;
    }
}

// -------------------------------------------------------
// 敵本体パターン
// -------------------------------------------------------
void EnemyPat_miComet_Grok()
{
    static bool polarityInitialized = false;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        polarityInitialized = false;
    }

    {
        // 桜側はゆっくり円弧を描くように漂う
        enemy.x = 180.0 + 80.0 * sin(count * 0.012);
        enemy.y = 50.0 + 18.0 * sin(count * 0.018);

        // 彗星側はより鋭く左右に振れながら少し前後する
        enemy.x2 = 300.0 + 90.0 * sin(count * 0.021 + 1.2);
        enemy.y2 = 55.0 + 12.0 * cos(count * 0.015);
    }

    // -------------------------------------------------------
    // 0秒: sound_enemyCharge
    // 1秒: 極性小玉（マゼンタ）配置 + sound_enemyShot_extreme
    // -------------------------------------------------------
    if (count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count == 60 && !polarityInitialized) {  // 1秒後
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPolarityOrb;
        pEnemyShotSet->x = player.x;
        pEnemyShotSet->y = player.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        polarityInitialized = true;
    }

    // -------------------------------------------------------
    // 3秒おきに charge → 1秒後に色変化（仕様どおり）
    // 実際の色変化は ShotPolarityOrb 内で行う
    // -------------------------------------------------------
    if (count >= 180 && (count - 180) % 180 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count >= 240 && (count - 240) % 180 == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // -------------------------------------------------------
    // ボス1（左）: 桜弾幕（マゼンタ）を定期的に発射
    // -------------------------------------------------------
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSakura;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // -------------------------------------------------------
    // ボス2（右）: 彗星弾幕（シアン）を定期的に発射
    // -------------------------------------------------------
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet;
        pEnemyShotSet->x = enemy.x2;
        pEnemyShotSet->y = enemy.y2 + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}