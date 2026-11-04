#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 飛鉢「伝説の飛空円盤」再現
// ・4つの巨大リング(各32x2重=64発) = UFOの縁
// ・リングは中心に追従して回転、中心は安全
// ・自機がボスを撃つとHP減少を検知して自機狙い反撃弾

// リング弾幕用
static void ShotSaucerRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 生成時演出
        if (CheckSoundMem(sound_enemyCharge) == 0) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // param_d[0]=centerX, [1]=centerY, [2]=radius1, [3]=rotation, [4]=rotSpeed, [5]=bulletNum
        double radius1 = pSet->param_d[2];
        double radius2 = radius1 + 8.0; // UFOの厚みを出す2重リング
        int n = (int)pSet->param_d[5]; // 片側の弾数 24~36

        for (int r = 0; r < 2; r++) {
            double rad = (r == 0) ? radius1 : radius2;
            for (int i = 0; i < n; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                double baseAng = 2.0 * DX_PI * i / n;
                if (r == 1) baseAng += DX_PI / n; // 2重目は半分ずらして隙間を埋める

                pShot->param_d[0] = baseAng; // 基準角度
                pShot->param_d[1] = rad;      // 自身の半径

                pShot->x = pSet->param_d[0] + rad * cos(baseAng);
                pShot->y = pSet->param_d[1] + rad * sin(baseAng);
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                // 青～シアンでHard/Lunatic版の色。kind %8の仕様に合わせ色番号で指定
                // 小玉 2.5x2.5
                int col = pSet->param_i[1]; // 0:赤 3:シアン 4:青
                pShot->kind = img_enemyShotSmallBall[col];
                pShot->margin = 240;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        if (CheckSoundMem(sound_enemyShot_medium) == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // 回転と中心の微移動
        pSet->param_d[3] += pSet->param_d[4]; // rotation += rotSpeed

        // ごくゆっくり中心を揺らす(原作の漂い)
        pSet->param_d[0] += sin(pSet->count * 0.008 + pSet->param_i[0]) * 0.15;
        pSet->param_d[1] += cos(pSet->count * 0.006 + pSet->param_i[0]) * 0.12;

        // 半径をじわじわ縮めて圧迫(伝説の飛空円盤は徐々に詰まる)
        if (pSet->count > 120 && pSet->param_d[2] > 42.0) {
            pSet->param_d[2] -= -0.03;
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double baseAng = pShot->param_d[0];
            double curAng = baseAng + pSet->param_d[3];
            double rad = pShot->param_d[1];
            // 内側リングは縮小に追従
            if (rad > 60.0) {
                // 外側リングも少し縮む
                rad = pSet->param_d[2] + 8.0;
                pShot->param_d[1] = rad;
            }
            else {
                rad = pSet->param_d[2];
                pShot->param_d[1] = rad;
            }
            pShot->x = pSet->param_d[0] + rad * cos(curAng);
            pShot->y = pSet->param_d[1] + rad * sin(curAng);
            pShot = pShot->next;
        }
    }
}

// 反撃弾幕用 : ボス被弾時に自機狙いをばら撒く
static void ShotRetaliation(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int num = pSet->param_i[0]; // 発射数
        if (num < 3) num = 3;
        if (num > 10) num = 10;
        double baseSpeed = pSet->param_d[0];

        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            // GetRand(x)は0..x inclusiveなので注意。-10..10を作るならGetRand(20)-10
            double spread = (GetRand(20) - 10) / 180.0 * DX_PI;
            if (num > 1) {
                spread += (i - num / 2) * 0.18; // 扇状に広げる
            }
            pShot->muki = pSet->muki + spread;
            pShot->speed = baseSpeed + GetRand(80) / 100.0; // 3.0~4.5程度
            // 反撃は赤小玉で目立たせる 5x2の銃弾だと速く見えるので小玉のまま赤に
            pShot->kind = img_enemyShotSmallBall[0]; // 0:赤

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyShot_heavy) == 0) {
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

void EnemyPat_HikouEnban_MetaAI()
{
    static int muki;
    static int prevHp;
    static int retaliationCool;
    static int ringsCreated;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        prevHp = enemy.hp;
        retaliationCool = 0;
        ringsCreated = 0;

        // --- 4つのUFOリングを生成 (Hard/Lunatic版 伝説の飛空円盤) ---
        struct RingDef { double cx, cy, r, rotSpd; int col; };
        RingDef defs[4] = {
            {120.0, 160.0, 68.0,  0.008, 4}, // 左上 青
            {360.0, 160.0, 68.0, -0.009, 3}, // 右上 シアン
            {120.0, 340.0, 68.0, -0.007, 4}, // 左下
            {360.0, 340.0, 68.0,  0.010, 3}, // 右下
        };

        for (int k = 0; k < 4; k++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSaucerRing;
            pSet->x = defs[k].cx;
            pSet->y = defs[k].cy;
            pSet->muki = 0.0;
            pSet->param_d[0] = defs[k].cx; // centerX
            pSet->param_d[1] = defs[k].cy; // centerY
            pSet->param_d[2] = defs[k].r*1.5;  // radius1
            pSet->param_d[3] = GetRand(360) / 180.0 * DX_PI; // 初期回転ランダム
            pSet->param_d[4] = defs[k].rotSpd;
            pSet->param_d[5] = 28; // 片側28発、2重で56発。原作Hard 24, Lunatic 36の中間
            pSet->param_i[0] = k * 2; // 揺れ位相ずれ用
            pSet->param_i[1] = defs[k].col; // 色 3 or 4

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        ringsCreated = 1;
    }
    else {
        // ボス本体はゆっくり左右に動く(原作はほぼ中央固定だが少し揺れる)
        enemy.x += 0.45 * (double)muki;
        if (enemy.x < 200.0) muki = 1;
        if (enemy.x > 280.0) muki = -1;
        if (count % 240 == 0) muki *= -1;

        // --- 反撃システム : HPが減ったら反撃 ---
        if (retaliationCool > 0) retaliationCool--;

        if (enemy.hp < prevHp && retaliationCool == 0) {
            int dmg = prevHp - enemy.hp; // 1フレームで複数ヒットするとdmg>1
            if (dmg > 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotRetaliation;
                pSet->x = enemy.x;
                pSet->y = enemy.y + 10.0;
                pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
                pSet->param_d[0] = 3.2 + GetRand(100) / 100.0; // 3.2~4.2
                pSet->param_i[0] = dmg * 3 + 2; // ダメージに比例して弾数増加

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;

                retaliationCool = 4; // 連射しすぎ防止
            }
            prevHp = enemy.hp;
        }
        else if (enemy.hp == prevHp) {
            // HPが変わらなかったフレームはそのまま
        }
        else {
            // HPが増えることはないが念のため同期
            if (enemy.hp < prevHp) prevHp = enemy.hp;
        }
    }
}