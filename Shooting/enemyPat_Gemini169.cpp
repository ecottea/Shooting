#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 1. ナワバリウェーブ（上から降りてくるインクの壁と背後の塗り）
static void ShotSplatoonWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // パラメータ0に格納された隙間の中心座標を取得
        int gap_pos = pEnemyShotSet->param_i[0];

        // 画面横一列にインクの壁（ローラー・ストライク）を生成。隙間付近だけ弾を作らない。
        for (double x = 20.0; x <= 460.0; x += 20.0) {
            if (fabs(x - gap_pos) < 30.0) {
                continue; // イカが通り抜けるための隙間
            }
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI/2; // 下向き
            pEnemyShot->speed = 1.1;     // ゆっくり降りてくる
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙色の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 塗りつぶされた領域（背後の小玉の群れ）
        for (int i = 0; i < 15*2; i++) {
            pEnemyShot = new sEnemyShot;
            // GetRand(x) は 0 から x までの x+1 種類
            pEnemyShot->x = pEnemyShotSet->x + GetRand(400) - 200;
            pEnemyShot->y = pEnemyShotSet->y - GetRand(60);
            pEnemyShot->muki = DX_PI/2 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.7 + GetRand(50) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙色の小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理（カウントや画面外消去はメインルーチン側で行われます）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 2. プレイヤーを狙うインク飛沫弾（スロッシャー風カウンター）
static void ShotSplatoonSplash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // シアン色の5-way弾（相手チームの反撃）
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 2) * 12.0 / 180.0 * DX_PI;
            pEnemyShot->speed = 3.2 + GetRand(40) / 100.0;
            pEnemyShot->kind = img_enemyShotBullet[3]; // シアン色の銃弾

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

// 3. 敵本体のパターン
void EnemyPat_Splatoon_Gemini()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体の左右移動
        enemy.x += 1.0 * (double)muki;
        if (enemy.x < 60.0 || enemy.x > 420.0) {
            muki *= -1;
        }
    }

    // 一定周期で「インクの波（ナワバリウェーブ）」を展開
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSplatoonWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        // 隙間の位置をランダムに決定 (60 〜 420 の範囲)
        pEnemyShotSet->param_i[0] = 60 + GetRand(360);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // プレイヤーを狙う「インク飛沫弾」を定期的に発射
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSplatoonSplash;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
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