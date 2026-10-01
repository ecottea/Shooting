#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：シュレッダー（紙が圧縮され、裁断されて紙片となって落下する）
// 使用素材：
//   - 弾種: 小玉（紙本体・紙片）、短レーザー（裁断刃の演出）
//   - 色: 白(6)＝紙、シアン(3)＝裁断刃
//   - 効果音: sound_enemyCharge（予告）、sound_enemyShot_heavy（裁断音）
static void ShotShredder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int COLS = 7 * 5;
    const int ROWS = 5;
    
    // ========== 初期化（紙の生成） ==========
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 紙のグリッド：横7列 × 縦5行 の小玉（白）
        // param_i[0] = 列番号(0〜6)、param_i[1] = 行番号(0〜4)
        // param_d[0] = 元の相対Xオフセット係数（圧縮計算用）
        const double X_SPACING = 20.0;
        const double Y_SPACING = 11.0;

        for (int col = 0; col < COLS; col++) {
            for (int row = 0; row < ROWS; row++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + (col - (COLS - 1) * 0.5) * X_SPACING;
                pEnemyShot->y = pEnemyShotSet->y + (row - (ROWS - 1) * 0.5) * Y_SPACING;
                pEnemyShot->muki = DX_PI * 0.5; // 下向き
                pEnemyShot->speed = 0.0;        // 初期は速度0（位置制御で動かす）
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白の小玉＝紙
                pEnemyShot->param_i[0] = col;
                pEnemyShot->param_i[1] = row;
                pEnemyShot->param_d[0] = (col - (COLS - 1) * 0.5); // 相対オフセット係数
                pEnemyShot->param_d[1] = 0.0; // 予備

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // フェーズ管理用
        pEnemyShotSet->param_i[0] = 0; // 0:圧縮降下中 1:裁断済み
    }

    // ========== 裁断刃の生成（シュレッダー作動演出） ==========
    if (pEnemyShotSet->count == 75) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 中央付近に縦方向の短レーザーを数本生成（裁断刃）
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 3) * 28.0;
            pEnemyShot->y = pEnemyShotSet->y - 30.0;
            pEnemyShot->muki = DX_PI * 0.5; // 下向き
            pEnemyShot->speed = 6.0;
            pEnemyShot->kind = img_enemyShotLaser[3]; // シアンの短レーザー
            pEnemyShot->param_i[0] = -1; // 刃フラグ（紙片処理から除外）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ========== 毎フレームの弾制御 ==========
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 裁断刃は単純に直進
        if (pShot->param_i[0] == -1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
            continue;
        }

        int t = pEnemyShotSet->count;

        // ---- フェーズ0：ゆっくり降下しながら中央へ圧縮 ----
        if (t < 90) {
            // 圧縮率：1.0 → 0.25 まで線形に縮小
            double compress = 1.0 - (double)t / 90.0 * 0.75;
            if (compress < 0.25) compress = 0.25;

            // Xは圧縮、Yは一定速度で降下
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * 20.0 * compress;
            pShot->y += 1.15;

            // セット自体の基準Yも更新（後続処理の基準にするため）
            // （複数弾があるので最後の弾で更新されるが実用上問題なし）
            pEnemyShotSet->y = pShot->y - (pShot->param_i[1] - ROWS/2) * 11.0 * 0.5;
        }
        // ---- フェーズ切替：裁断瞬間（速度・向きを紙片用に設定） ----
        else if (t == 90) {
            // 列ごとにわずかに異なる落下角度を付けて紙片感を出す
            double baseAngle = DX_PI * 0.5;
            double colOffset = -(pShot->param_i[0] - COLS/2) * 0.04; // 列で少し左右に開く
            double randAngle = (GetRand(30) - 15) / 180.0 * DX_PI; // ±15度程度のランダム

            pShot->muki = baseAngle + colOffset + randAngle;
            pShot->speed = 2.4 + GetRand(80) / 100.0; // 2.4〜3.2

            // 紙片として少し横に散らす
            pShot->x += (pShot->param_i[0] - COLS/2) * 4.0;
        }
        // ---- フェーズ1：紙片の落下（ひらひら揺れ） ----
        else {
            // 基本の直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ひらひら揺れ（列番号で位相をずらす）
            pShot->x += 0.25 * sin(pShot->count * 0.09 + pShot->param_i[0] * 0.7);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Shredder_Grok()
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
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;

        // 画面端で反転
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
    }

    // 一定間隔でシュレッダー弾幕を発生
    if (count % 210 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShredder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI * 0.5; // 下向き基準
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