#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：バーンズリーのシダ（フラクタル・ファーン）
static void ShotFractalFern(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 親弾（茎）を生成
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki; // 茎の成長方向
        pShot->speed = 1.0;
        pShot->kind = img_enemyShotMediumOval[2]; // 緑の中楕円弾（茎）
        pShot->param_i[0] = 0; // 世代: 0=親
        pShot->param_i[1] = 30; // 次の分裂までのカウント
        pShot->param_i[2] = 0; // 分裂回数カウンター
        pShot->param_d[0] = 0.0; // うねり用位相
        // リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        // 親弾の軌道をうねらせる（自然な茎の表現）
        if (pShot->param_i[0] == 0) {
            pShot->param_d[0] += 0.05;
            pShot->muki = pEnemyShotSet->muki + sin(pShot->param_d[0]) * 0.15;
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->param_i[0] == 0) { // 親弾の処理
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->param_i[1] = 24-10; // 分裂インターバル
                pShot->param_i[2]++;

                // 左右に子弾（小葉）を生成
                double baseAngle = pShot->muki;
                for (int i = -1; i <= 1; i += 2) {
                    sEnemyShot* pChild = new sEnemyShot;
                    pChild->x = pShot->x;
                    pChild->y = pShot->y;
                    // 進行方向に対して外側へ広がる
                    pChild->muki = baseAngle + i * (DX_PI / 3.0);
                    pChild->speed = 1.2;
                    pChild->kind = img_enemyShotSmallBall[2]; // 緑の小玉（葉の軸）
                    pChild->param_i[0] = 1; // 世代: 1=子
                    pChild->param_i[1] = 40; // 孫弾生成までの寿命
                    pChild->param_d[0] = pChild->muki;
                    // リスト追加
                    pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pChild->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                    pEnemyShotSet->pEnemyShotHead->prev = pChild;
                }
            }
        }
        else if (pShot->param_i[0] == 1) { // 子弾の処理
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                // 寿命が来たら孫弾（葉のディテール）を生成して自分は消滅
                double baseAngle = pShot->param_d[0];
                int numGrandChildren = 5-2;
                double spread = DX_PI / 2.5; // 扇状の広がり

                for (int i = 0; i < numGrandChildren; i++) {
                    sEnemyShot* pGrand = new sEnemyShot;
                    pGrand->x = pShot->x;
                    pGrand->y = pShot->y;
                    pGrand->muki = baseAngle - spread / 2.0 + (spread * i / (numGrandChildren - 1));
                    pGrand->speed = 0.6 + GetRand(20) / 100.0;
                    pGrand->kind = img_enemyShotScale[2]; // 緑の鱗弾（葉のディテール）
                    pGrand->param_i[0] = 2; // 世代: 2=孫
                    // リスト追加
                    pGrand->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pGrand->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pGrand;
                    pEnemyShotSet->pEnemyShotHead->prev = pGrand;
                }

                // 自分（子弾）をリストから外して削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = next;
                continue;
            }
        }
        pShot = next;
    }
}

// 敵本体のパターン
void EnemyPat_BarnsleyFern_Qwen()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 一定間隔でシダ弾幕を発射
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFractalFern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 30.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下に向かって成長
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