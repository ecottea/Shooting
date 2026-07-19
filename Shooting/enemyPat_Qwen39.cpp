// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数
// ============================================================

// 1. インベーダー隊形（ドット絵）の維持・移動
// 敵本体の座標に、各弾が持つ相対座標（param_d）を加算して位置を決定する。
// これにより、敵本体が移動するとドット絵全体が追従する。
static void FormationFollow(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = enemy.x + pShot->param_d[0];
        pShot->y = enemy.y + pShot->param_d[1];
        pShot = pShot->next;
    }
}

// 2. インベーダーのミサイル攻撃（直線落下）
// 敵本体の位置（ドット絵の最下段想定）から真下に弾を落とす。
static void InvaderShot(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 発射音
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 弾の生成
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = DX_PI / 2.0; // 真下
        pShot->speed = 4.0 + GetRand(10) / 10.0; // 速さ 4.0～5.0
        pShot->count = 0;

        // 素材: 銃弾 (img_enemyShotBullet) の 赤色 (0)
        pShot->kind = img_enemyShotBullet[0];

        // リストに登録
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 弾の移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 3. UFOレーザー（横方向の妨害）
static void UfoLaser(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 発射音
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->count = 0;

        // 方向と速度は param_d に保存してある
        pShot->muki = pSet->param_d[0];
        pShot->speed = pSet->param_d[1];

        // 素材: レーザー (img_enemyShotLaser) の 白 (6) または シアン (3)
        pShot->kind = img_enemyShotLaser[3];

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Invader_Qwen()
{
    static int muki;       // 移動方向 (1:右, -1:左)
    static int move_speed; // 移動速度
    static sEnemyShotSet* pFormation; // ドット絵用セット

    if (count == 1) {
        // --- 初期化 ---
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 親玉として硬く
        muki = 1;
        move_speed = 1;

        // ドット絵セットの生成
        pFormation = new sEnemyShotSet;
        pFormation->count = 0;
        pFormation->patternFunc = FormationFollow;
        pFormation->x = enemy.x;
        pFormation->y = enemy.y;
        pFormation->kind = 0;

        // ダミーのヘッドノード作成
        pFormation->pEnemyShotHead = new sEnemyShot;
        pFormation->pEnemyShotHead->prev = pFormation->pEnemyShotHead;
        pFormation->pEnemyShotHead->next = pFormation->pEnemyShotHead;

        // グリッド弾（ドット絵）の生成
        // 5列 x 5行 = 25個の弾でインベーダーを表現
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 5; col++) {
                sEnemyShot* pShot = new sEnemyShot;

                // 相対座標の設定 (param_dに保存)
                // 間隔は20ピクセル
                pShot->param_d[0] = (col - 2) * 20.0;
                pShot->param_d[1] = (row - 2) * 20.0;

                // 初期座標（FormationFollowですぐ上書きされるが念のため）
                pShot->x = enemy.x + pShot->param_d[0];
                pShot->y = enemy.y + pShot->param_d[1];
                pShot->muki = 0;
                pShot->speed = 0; // 自身では動かない
                pShot->count = 0;

                // 素材: 小玉 (img_enemyShotSmallBall) の 緑色 (2)
                // インベーダーのドットに見立てる
                pShot->kind = img_enemyShotSmallBall[2];

                // リスト登録
                pShot->prev = pFormation->pEnemyShotHead->prev;
                pShot->next = pFormation->pEnemyShotHead;
                pFormation->pEnemyShotHead->prev->next = pShot;
                pFormation->pEnemyShotHead->prev = pShot;
            }
        }

        // グローバルリストへ登録
        pFormation->prev = enemyShotSetHead.prev;
        pFormation->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFormation;
        enemyShotSetHead.prev = pFormation;
    }
    else {
        // --- 移動処理（インベーダーアルゴリズム） ---

        // 速度上昇（カウント経過とともに加速）
        // 60フレーム(約1秒)ごとに速度レベルが上がるイメージ
        move_speed = 1 + (count / 180);
        if (move_speed > 6) move_speed = 6; // 上限

        enemy.x += muki * move_speed;

        // 画面端判定と下降
        // 画面幅480、敵サイズ考慮
        if (enemy.x > 440 || enemy.x < 40) {
            muki *= -1; // 反転
            enemy.y += 24.0; // 一段下降

            // 下降時にUFOレーザー発射イベント
            if (GetRand(10) < 4) { // 40%の確率
                sEnemyShotSet* pUfo = new sEnemyShotSet;
                pUfo->count = 0;
                pUfo->patternFunc = UfoLaser;
                pUfo->x = enemy.x;
                pUfo->y = enemy.y - 20.0; // 少し上から
                pUfo->kind = 0;

                // 横方向のランダムな速度
                double ufo_speed = 6.0 + GetRand(20) / 10.0;
                pUfo->param_d[0] = (GetRand(1) == 0) ? 0.0 : DX_PI; // 左か右
                pUfo->param_d[1] = ufo_speed;

                pUfo->pEnemyShotHead = new sEnemyShot;
                pUfo->pEnemyShotHead->prev = pUfo->pEnemyShotHead;
                pUfo->pEnemyShotHead->next = pUfo->pEnemyShotHead;

                pUfo->prev = enemyShotSetHead.prev;
                pUfo->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pUfo;
                enemyShotSetHead.prev = pUfo;
            }
        }
    }

    // --- 攻撃処理（ミサイル発射） ---
    // 移動速度が上がるにつれ、攻撃頻度も上げる
    int shot_interval = 20 - (move_speed * 2);
    if (shot_interval < 5) shot_interval = 5;

    if (count % shot_interval == 0) {
        sEnemyShotSet* pShotSet = new sEnemyShotSet;
        pShotSet->count = 0;
        pShotSet->patternFunc = InvaderShot;

        // 敵本体の位置から発射（ドット絵の最下段付近の想定）
        pShotSet->x = enemy.x;
        pShotSet->y = enemy.y + 40.0;
        pShotSet->muki = DX_PI / 2.0;
        pShotSet->kind = 0;

        pShotSet->pEnemyShotHead = new sEnemyShot;
        pShotSet->pEnemyShotHead->prev = pShotSet->pEnemyShotHead;
        pShotSet->pEnemyShotHead->next = pShotSet->pEnemyShotHead;

        pShotSet->prev = enemyShotSetHead.prev;
        pShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pShotSet;
        enemyShotSetHead.prev = pShotSet;
    }
}