#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ヘリンボーン・テキスタイル
static void ShotHerringbone(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 一定間隔でV字を構成する斜め弾列を発射（カウントが240未満の間継続）
    if (pEnemyShotSet->count % 1 == 0 && pEnemyShotSet->count < 240) {
        // 時間経過で発射角度を滑らかに変化させ、V字の織り目模様を形成
        double t = pEnemyShotSet->count * 0.1;
        double angle_offset = sin(t) * 0.6;

        // 左右2方向の斜め弾（右下がりと右上がり）を同時に生成
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            if (i == 0) {
                // 右下方向の斜線（シアンの小玉）
                pEnemyShot->muki = (DX_PI / 4.0) + angle_offset;
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
            }
            else {
                // 左下方向の斜線（青の小玉）
                pEnemyShot->muki = (DX_PI * 3.0 / 4.0) - angle_offset;
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
            }
            pEnemyShot->speed = 2.5;

            // 弾リストへの登録
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 折り返しや交差のアクセントとして、周期的に白の銃弾（米弾）を混入
        if (pEnemyShotSet->count % 3 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // GetRand(40) は 0〜40（41種類）を返すため、-20して中央付近に微小なランダム性を持たせる
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotBullet[6]; // 白の銃弾（米弾）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Herringbone_Gemini()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体は左右にゆっくり往復移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 一定間隔でヘリンボーン弾幕セットを生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringbone;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
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