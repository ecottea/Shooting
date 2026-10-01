#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 素材選定メモ
// 使える弾: 小玉(2.5) / 中玉(7.0) / 大玉(20.0) / 銃弾(5.0x2.0) / 鱗弾(4.0x3.0) / 菱形弾 / 中楕円弾(10.5x7.0) / 短レーザー(64.0x4.0)
// 今回使用:
// 紙(未細断) : 中楕円弾(白) img_enemyShotMediumOval[6] - 横に広く紙の欠片に見える
// 紙片(本体): 短レーザー(白) img_enemyShotLaser[6] - 64x4の縦長でシュレッダーの帯そのもの
// 紙片(端) : 銃弾(白) img_enemyShotBullet[6] - 細長弾でギザギザ感を補強
// 毛羽/紙粉 : 小玉(白) img_enemyShotSmallBall[6] - トレイルとして重ねる
// 筐体 : 中玉(黒) img_enemyShotMediumBall[7] - シュレッダー本体の枠
// =============================================

// 機密抹消『クロスカット・シュレッダー』本体
static void ShotShredderPaper(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 横15 x 縦3 = 45枚で1枚の紙を構成
        // GetRand(x)は 0〜x の x+1通りを返すので注意
        const int cols = 15;
        const int rows = 3;
        for (int iy = 0; iy < rows; iy++) {
            for (int ix = 0; ix < cols; ix++) {
                pEnemyShot = new sEnemyShot;
                // 480x480画面、上から給紙されるイメージ
                pEnemyShot->x = pEnemyShotSet->x + (ix - cols / 2) * 10.0 + (GetRand(40) - 20) / 10.0;
                pEnemyShot->y = pEnemyShotSet->y - 60.0 + iy * 11.0 + (GetRand(20) - 10) / 10.0;
                // ほぼ真下、わずかにばらつき
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI * 0.15;
                pEnemyShot->speed = 1.2 + GetRand(30) / 100.0;

                pEnemyShot->kind = img_enemyShotMediumOval[6]; // 白: 紙

                pEnemyShot->param_i[0] = 0; // 0=紙状態, 1=細断後
                pEnemyShot->param_d[0] = GetRand(628) / 100.0; // ひらひら用位相 0〜6.28
                pEnemyShot->param_d[1] = (ix - cols / 2) * 0.5; // 収束時のオフセット保持

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 筐体の黒い枠を吸い込み口に見せるため、位置を記憶
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 途中で新規生成しても安全なように先に保存

        if (pShot->param_i[0] == 0) {
            // --- Phase: 紙が吸い込まれる ---
            // 中央に寄せながら落下
            pShot->x += (enemy.x - pShot->x) * 0.018;
            pShot->x += pShot->speed * cos(pShot->muki) * 0.3;
            pShot->y += pShot->speed * sin(pShot->muki);

            // シュレッダー投入口(enemy.y)を超えたら細断
            if (pShot->y > enemy.y + 8.0) {
                pShot->param_i[0] = 1;
                // 短レーザーを縦向きにして紙片に
                if (GetRand(100) < 70) {
                    pShot->kind = img_enemyShotLaser[6]; // 7割は長い帯
                }
                else {
                    pShot->kind = img_enemyShotBullet[6]; // 3割は短い端
                }
                pShot->muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI * 0.3;
                pShot->speed = 2.0 + GetRand(150) / 100.0;

                // 毛羽立ち用に小玉を1つ追加生成 (同じセットの末尾に)
                if (GetRand(100) < 60) {
                    sEnemyShot* pDust = new sEnemyShot;
                    pDust->x = pShot->x + GetRand(6) - 3;
                    pDust->y = pShot->y + GetRand(4) - 2;
                    pDust->muki = pShot->muki + (GetRand(20) - 10) / 180.0 * DX_PI * 0.2;
                    pDust->speed = pShot->speed * 0.85;
                    pDust->kind = img_enemyShotSmallBall[6];
                    pDust->param_i[0] = 1;
                    pDust->param_d[0] = pShot->param_d[0] + GetRand(100) / 100.0;
                    pDust->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pDust->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pDust;
                    pEnemyShotSet->pEnemyShotHead->prev = pDust;
                }

                if (CheckSoundMem(sound_enemyShot_noize)) StopSoundMem(sound_enemyShot_noize);
                PlaySoundMem(sound_enemyShot_noize, DX_PLAYTYPE_BACK);
            }
        }
        else {
            // --- Phase: 紙吹雪としてひらひら落ちる ---
            // sin波で横揺れ
            double sway = sin((double)pEnemyShotSet->count * 0.08 + pShot->param_d[0]) * 0.7;
            pShot->x += sway;
            pShot->x += pShot->speed * cos(pShot->muki) * 0.2;
            pShot->y += pShot->speed * sin(pShot->muki) * 0.9;
            // 揺れに合わせて角度も少し傾ける(レーザー/銃弾は向きで回転する)
            pShot->muki += sway * 0.02;
        }

        pShot = pNext;
    }
}

// シュレッダー筐体の表現(黒い中玉が投入口の両脇に残る)
static void ShotShredderBody(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int side = 0; side < 2; side++) {
            sEnemyShot* p = new sEnemyShot;
            double offsetX = (side == 0) ? -30.0 : 30.0;
            p->x = pEnemyShotSet->x + offsetX;
            p->y = pEnemyShotSet->y + 10.0;
            p->muki = 0;
            p->speed = 0; // 自前で敵に追従させる
            p->kind = img_enemyShotMediumBall[7]; // 黒
            p->param_d[0] = offsetX; // 初期オフセット
            p->param_d[1] = GetRand(628) / 100.0; // 揺れ位相
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 敵本体に追従 + 微小な揺れで振動しているように見せる
        pShot->x = enemy.x + pShot->param_d[0] + sin(pEnemyShotSet->count * 0.07 + pShot->param_d[1]) * 1.2;
        pShot->y = enemy.y + 10.0 + cos(pEnemyShotSet->count * 0.05 + pShot->param_d[1]) * 1.0;
        pShot = pShot->next;
    }
}

void EnemyPat_Shredder_MetaAI()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 本体はゆっくり左右に移動して投入位置をずらす
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) muki *= -1;
    }

    // 90Fごとに紙の束を給紙
    if (count % 90 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShredderPaper;
        pSet->x = enemy.x + GetRand(40) - 20; // 給紙位置を少しランダムに
        pSet->y = enemy.y - 20.0;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 筐体は120Fごとに再生成(消えないように)
    if (count % 120 == 10) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShredderBody;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
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