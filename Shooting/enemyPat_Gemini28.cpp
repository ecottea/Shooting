// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：金閣寺の一枚天井
static void ShotKinkakuji(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 弾の生成処理（セットのカウントが0のタイミングで実行） ---
    if (pEnemyShotSet->count == 0) {

        // パラメータの受け取り
        // param_i[0]: 弾の種類 (0:天井の大玉, 1:バラ撒き緑, 2:バラ撒きシアン, 3:バラ撒き青)
        int shotType = pEnemyShotSet->param_i[0];

        if (shotType == 0) {
            // 【天井弾の生成】
            // 重たい弾幕が迫る効果音
            if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 画面横幅480に対して、大玉を等間隔に並べる
            // 隙間（安全地帯）を作るためのインデックスをランダムに決定
            // 0〜15の全16箇所中、連続する2箇所を隙間とする
            int gapIndex = GetRand(14);

            for (int i = 0; i < 16; i++) {
                // 隙間の位置なら弾を生成しない
                if (i == gapIndex || i == gapIndex + 1) {
                    continue;
                }

                pEnemyShot = new sEnemyShot;
                // X座標を等間隔に配置 (15〜465辺りまでカバー)
                pEnemyShot->x = 15.0 + i * 30.0 + GetRand(6) - 3; // わずかなブレを持たせる
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = DX_PI / 2.0; // 真下向き
                pEnemyShot->speed = 1.2;         // 天井がじわじわと迫る低速

                // 赤の大玉を使用
                pEnemyShot->kind = img_enemyShotLargeBall[0];

                // リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else {
            // 【バラ撒き自機狙い弾の生成】
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // shotType に応じて弾の色とWAY数を変える
            int bulletImg = img_enemyShotMediumBall[2]; // デフォルト緑
            int wayCount = 3;

            if (shotType == 1) {      // 第1段階: 緑
                bulletImg = img_enemyShotMediumBall[2];
                wayCount = 3;
            }
            else if (shotType == 2) { // 第2段階: シアン
                bulletImg = img_enemyShotMediumBall[3];
                wayCount = 5;
            }
            else if (shotType == 3) { // 第3段階: 青 (発狂)
                bulletImg = img_enemyShotMediumBall[4];
                wayCount = 7;
            }

            // 自機狙い方向を基準に扇状（WAY弾）に展開
            double baseMuki = pEnemyShotSet->muki;
            double angleInterval = 15.0 / 180.0 * DX_PI; // 弾同士の間隔角度

            for (int i = 0; i < wayCount; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 中心からのオフセット計算
                double offsetAngle = (i - (wayCount - 1) / 2.0) * angleInterval;
                // わずかなランダム性を加えて文花帖らしさを表現
                double randomAngle = (GetRand(20) - 10) / 180.0 * DX_PI;

                pEnemyShot->muki = baseMuki + offsetAngle + randomAngle;
                pEnemyShot->speed = 2.0 + (GetRand(100) / 100.0); // 2.0 〜 3.0 のランダム速度

                pEnemyShot->kind = bulletImg;

                // リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --- 弾の移動ルーチン（全フレームで実行） ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン関数（メインルーチンから毎フレーム呼ばれる）
void EnemyPat_Kinkakuji_Gemini()
{
    static int moveDir;      // 揺動用の移動方向
    static int phase;        // 弾幕の段階 (1〜3)

    // 初期化処理
    if (count == 1) {
        // 敵の初期位置（画面上部中央）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // HPは200固定仕様
        moveDir = 1;
        phase = 1;
    }
    else {
        // 輝夜の微細な左右移動（どっしりと構えつつ少し動く）
        enemy.x += 0.4 * (double)moveDir;
        if (count % 80 == 0) {
            moveDir *= -1;
        }
    }

    // 残りHPに応じて段階（発狂度）を変化させる
    if (enemy.hp > 130) {
        phase = 1; // 初期段階
    }
    else if (enemy.hp > 60) {
        phase = 2; // 中期段階（シアン追加）
    }
    else {
        phase = 3; // 最終段階（青追加・発狂）
    }

    // --------------------------------------------------
    // 1. 天井（大玉の壁）の定期的生成
    // --------------------------------------------------
    // 75フレームに1回、画面最上部から降ってくる天井をセット
    if (count % 75 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKinkakuji;
        pEnemyShotSet->x = 0.0;     // X座標はセット内ループで計算するため基準は0
        pEnemyShotSet->y = -10.0;   // 画面上部外から出現
        pEnemyShotSet->muki = DX_PI / 2.0;

        pEnemyShotSet->param_i[0] = 0; // 0番: 天井大玉を示すフラグ

        // ヘッドノード初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体セットリストに結合
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --------------------------------------------------
    // 2. 自機狙いバラ撒き弾の定期的生成
    // --------------------------------------------------
    // 段階（phase）が上がるほど、発射頻度が上昇する
    bool canShootScatter = false;
    int currentShotType = 1; // 1:緑, 2:シアン, 3:青

    switch (phase) {
    case 1:
        // 35フレームに1回発射 (緑のみ)
        if (count % 35 == 0) {
            canShootScatter = true;
            currentShotType = 1;
        }
        break;
    case 2:
        // 25フレームに1回発射 (緑とシアンが交互・またはランダム)
        if (count % 25 == 0) {
            canShootScatter = true;
            currentShotType = (GetRand(10) > 5) ? 1 : 2;
        }
        break;
    case 3:
        // 18フレームに1回発射 (高密度・全色混ざる発狂状態)
        if (count % 18 == 0) {
            canShootScatter = true;
            int r = GetRand(2);
            currentShotType = (r == 0) ? 1 : (r == 1) ? 2 : 3;
        }
        break;
    }

    if (canShootScatter) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKinkakuji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // 自機への正確な方向ベクトルを計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->param_i[0] = currentShotType; // バラ撒き弾の色タイプを割り当て

        // ヘッドノード初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体セットリストに結合
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}