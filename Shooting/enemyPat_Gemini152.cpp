#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾を追加するためのヘルパー関数
static void AddMullerLyerShot(sEnemyShotSet* pSet, double x, double y, double speed, double angle, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = angle;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = 240;

    // 双方向循環リストの末尾（ヘッドの直前）に追加
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕：ミュラー・リヤー錯視（ダブル・アロー）
static void ShotMullerLyer(sEnemyShotSet* pEnemyShotSet)
{
    // 発射フレーム（最初の1フレーム）でのみ全弾を配置する
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double speed = 2.5;
        double angle = DX_PI / 2.0; // 画面下方向へ直進

        // 軸は「赤の銃弾」、羽は「青の菱形弾」を使用
        int kindAxis = img_enemyShotBullet[0];
        int kindWing = img_enemyShotDiamond[4];

        int numAxis = 7+4;             // 軸の弾数
        double spacing = 32.0/1.5;       // 軸弾の間隔
        double wingSpacingX = 14.0/1.5;  // 羽弾のX間隔（広がり具合）
        double wingSpacingY = 14.0/1.5;  // 羽弾のY間隔（角度）
        int numWings = 3+2;            // 羽の長さ（片側あたりの弾数）

        // 左右の矢印の中心X座標（間隔を空けて配置）
        double baseX_Left = pEnemyShotSet->x - 100.0/2;
        double baseX_Right = pEnemyShotSet->x + 100.0/2;

        // 矢印の先端と最後尾のY座標
        double headY = pEnemyShotSet->y;
        double tailY = headY - (numAxis - 1) * spacing;

        // -----------------------------------------------------
        // 【左側の弾幕】外開きパターン（長く見える）
        // -----------------------------------------------------
        // 軸の生成
        for (int i = 0; i < numAxis; i++) {
            AddMullerLyerShot(pEnemyShotSet, baseX_Left, headY - i * spacing, speed, angle, kindAxis);
        }
        // 羽の生成
        for (int j = 1; j <= numWings; j++) {
            // 先頭の羽 (外側に開く ＝ 進行方向・外側へ伸びる)
            AddMullerLyerShot(pEnemyShotSet, baseX_Left - j * wingSpacingX, headY + j * wingSpacingY, speed, angle, kindWing);
            AddMullerLyerShot(pEnemyShotSet, baseX_Left + j * wingSpacingX, headY + j * wingSpacingY, speed, angle, kindWing);
            // 最後尾の羽 (外側に開く ＝ 逆方向・外側へ伸びる)
            AddMullerLyerShot(pEnemyShotSet, baseX_Left - j * wingSpacingX, tailY - j * wingSpacingY, speed, angle, kindWing);
            AddMullerLyerShot(pEnemyShotSet, baseX_Left + j * wingSpacingX, tailY - j * wingSpacingY, speed, angle, kindWing);
        }

        // -----------------------------------------------------
        // 【右側の弾幕】内折りパターン（短く見える）
        // -----------------------------------------------------
        // 軸の生成（左側と全く同じ長さと弾数）
        for (int i = 0; i < numAxis; i++) {
            AddMullerLyerShot(pEnemyShotSet, baseX_Right, headY - i * spacing, speed, angle, kindAxis);
        }
        // 羽の生成
        for (int j = 1; j <= numWings; j++) {
            // 先頭の羽 (内側に折る ＝ 逆方向・外側へ伸びる)
            AddMullerLyerShot(pEnemyShotSet, baseX_Right - j * wingSpacingX, headY - j * wingSpacingY, speed, angle, kindWing);
            AddMullerLyerShot(pEnemyShotSet, baseX_Right + j * wingSpacingX, headY - j * wingSpacingY, speed, angle, kindWing);
            // 最後尾の羽 (内側に折る ＝ 進行方向・外側へ伸びる)
            AddMullerLyerShot(pEnemyShotSet, baseX_Right - j * wingSpacingX, tailY + j * wingSpacingY, speed, angle, kindWing);
            AddMullerLyerShot(pEnemyShotSet, baseX_Right + j * wingSpacingX, tailY + j * wingSpacingY, speed, angle, kindWing);
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
void EnemyPat_MullerLyer_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    // 移動処理（画面上部をゆっくり左右に往復）
    else {
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 140.0) muki = 1;
        if (enemy.x > 340.0) muki = -1;
    }

    // 発射の予兆音
    //if (count % 160 == 110) {
    //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    // 周期的に弾幕を発射
    if (count % 90 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMullerLyer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // ダミーのヘッドノードを作成してリストを初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}