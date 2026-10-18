#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 銃符「ルナティックガン」 Seiran
// 原作: 三角弾(自機依存)と座薬弾(大玉)がボスから半径離れた位置から出現
static void ShotLunaticGun(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 初回発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ---- 1. 三角弾: 自機狙い、周囲から出現 ----
        // GetRand(x)は0～xのx+1種類なので注意
        int triNum = 9 + GetRand(4); // 9～13発
        double spawnRadiusTri = 45.0 + (double)GetRand(20); // 45～65

        for (int i = 0; i < triNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 出現位置をボス周囲の円周上に取る = 異次元から飛ばす能力の再現
            // 自機方向を中心に±50度にばらつかせる
            double spawnAng = pEnemyShotSet->muki + (GetRand(100) - 50) / 180.0 * DX_PI * 0.8;
            // さらに少しランダムに散らす
            spawnAng += (GetRand(20) - 10) / 180.0 * DX_PI * 0.2;

            double sx = pEnemyShotSet->x + spawnRadiusTri * cos(spawnAng);
            double sy = pEnemyShotSet->y + spawnRadiusTri * sin(spawnAng);

            // 出現位置から自機へ再狙い (+-10度の誤差)
            double aim = atan2(player.y - sy, player.x - sx);
            aim += (GetRand(20) - 10) / 180.0 * DX_PI * 0.3;

            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            pEnemyShot->muki = aim;
            pEnemyShot->speed = 2.8 + GetRand(100) / 100.0; // 2.8～3.8

            // 三角弾は菱形弾で代用、色は月らしいシアン(3)青(4)を交互
            int col = (i % 2 == 0) ? 3 : 4;
            // 8色だが橙(8)まで使えるため GetRand範囲注意、ここは固定色
            pEnemyShot->kind = img_enemyShotDiamond[col];

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 2. 座薬弾: 遅くて大きい壁弾、半径遠めから出現 ----
        int largeNum = 5 + GetRand(2); // 5～7発
        double spawnRadiusLarge = 75.0 + (double)GetRand(20); // 75～95

        for (int i = 0; i < largeNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 円周上に均等+ランダム
            double base = pEnemyShotSet->muki;
            double ang = base + (2.0 * DX_PI / largeNum) * i + (GetRand(30) - 15) / 180.0 * DX_PI;

            double sx = pEnemyShotSet->x + spawnRadiusLarge * cos(ang);
            double sy = pEnemyShotSet->y + spawnRadiusLarge * sin(ang);

            double aim = atan2(player.y - sy, player.x - sx);
            // 壁として少し横に逸らす
            aim += (GetRand(40) - 20) / 180.0 * DX_PI * 0.4;

            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            pEnemyShot->muki = aim;
            pEnemyShot->speed = 1.2 + GetRand(80) / 100.0; // 1.2～2.0 遅い

            // 中楕円弾(10.5x7.0) 白(6)で座薬感、偶数は大玉で圧迫感
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotMediumOval[6]; // 白
            }
            else {
                pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白大玉
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 3. 銃弾アクセント: 中心から速射 ----
        {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 5.0 + GetRand(50) / 100.0; // 5.0～5.5
            pEnemyShot->kind = img_enemyShotBullet[1]; // 黄 銃弾
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

void EnemyPat_LunaticGun_MetaAI()
{
    static int dir;
    static int shot_kind;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 仕様固定
        dir = 1;
        shot_kind = 0;
    }
    else {
        // 原作はゆっくり横移動しながら撃つ。体当たりも原作の脅威
        enemy.x += 0.6 * (double)dir;
        enemy.y = 60.0 + sin(count * 0.02) * 8.0;
        if (enemy.x < 90.0) dir = 1;
        if (enemy.x > 390.0) dir = -1;
    }

    // 14Fごとに1セット発射 = 原作の連射感。count, pEnemyShotSet->count, pEnemyShot->countのインクリメントはメイン側
    if (count % 14 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLunaticGun;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_kind++;

        // ダミーヘッド生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストへ連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}