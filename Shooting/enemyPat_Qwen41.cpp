// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：プリズム・ブレイク・チェイン
// ============================================================
static void ShotPrismBreak(sEnemyShotSet* pSet)
{
    // param_i[0] の役割
    // 0: ブロック (静止、当たり判定あり)
    // 1: コアボール (反射、ブロック破壊)
    // 2: 分裂弾 (直進、消滅)

    // param_i[1] の役割
    // pSet全体で: 終了フラグ (1になったら終了)
    // 弾個別で: 未使用

    // --- 初期化 (count == 0) ---
    if (pSet->count == 0) {
        // 効果音: 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ブロックの配置 (5列 x 4行 = 20個)
        // 素材選定: img_enemyShotMediumOval (中楕円) の シアン(3) を使用。
        // ブロック崩しの「硬質で整列した壁」を表現するため。
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 4; j++) {
                sEnemyShot* pBlock = new sEnemyShot;
                pBlock->x = 120.0 + i * 60.0;
                pBlock->y = 60.0 + j * 25.0;
                pBlock->muki = 0.0;
                pBlock->speed = 0.0; // 静止
                pBlock->kind = img_enemyShotMediumOval[3]; // シアン
                pBlock->param_i[0] = 0; // Block Flag

                // リストに登録
                pBlock->prev = pSet->pEnemyShotHead->prev;
                pBlock->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pBlock;
                pSet->pEnemyShotHead->prev = pBlock;
            }
        }
    }

    // --- コアボールの生成 (60フレームごと) ---
    // ブロックが存在する場合のみ生成
    if (pSet->count % 60 == 0 && pSet->param_i[1] == 0) {
        // 効果音: 重めの発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pCore = new sEnemyShot;
        pCore->x = enemy.x;
        pCore->y = enemy.y + 20.0;

        // プレイヤー狙い、あるいはブロック群へのランダム狙い
        // ここでは少しランダム性を持たせる
        pCore->muki = atan2(player.y - pCore->y, player.x - pCore->x) + (GetRand(60) - 30) * DX_PI / 180.0 * 0;
        pCore->speed = 3.5; // 速すぎず遅すぎず
        pCore->kind = img_enemyShotLargeBall[0]; // 赤い大玉
        pCore->param_i[0] = 1; // Core Flag

        // リスト登録
        pCore->prev = pSet->pEnemyShotHead->prev;
        pCore->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pCore;
        pSet->pEnemyShotHead->prev = pCore;
    }

    // --- 毎フレームの更新処理 ---
    int blockCount = 0; // 生存ブロック数のカウント

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除に備えて次を退避

        if (pShot->param_i[0] == 0) {
            // [ブロック]
            blockCount++;
            // 静止しているが、微細な振動で存在感を出す（オプション）
            // pShot->x += sin(pSet->count * 0.1 + pShot->y) * 0.2; 
        }
        else {
            // [ボール (コア or 分裂)]

            // 1. 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 2. 壁反射 (画面左右、上)
            // 画面下は自機領域なので、ここでは反射させず画面外へ去らせる
            if (pShot->x < 10.0 || pShot->x > 470.0) {
                pShot->muki = DX_PI - pShot->muki;
                // 壁めり込み補正
                if (pShot->x < 10.0) pShot->x = 10.0;
                if (pShot->x > 470.0) pShot->x = 470.0;
            }
            if (pShot->y < 10.0) {
                pShot->muki = -pShot->muki;
                pShot->y = 10.0;
            }

            // 3. 画面外判定 (画面下へ出たら消去)
            if (pShot->y > 500.0) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }

            // 4. ブロックとの当たり判定 (コアボールのみ行う)
            if (pShot->param_i[0] == 1) {
                sEnemyShot* pBlock = pSet->pEnemyShotHead->next;
                while (pBlock != pSet->pEnemyShotHead) {
                    sEnemyShot* pBNext = pBlock->next;

                    if (pBlock->param_i[0] == 0) {
                        double dx = pShot->x - pBlock->x;
                        double dy = pShot->y - pBlock->y;
                        // 距離判定 (半径の和の2乗)
                        // ブロックは中楕円なので判定を少し甘くする
                        if (dx * dx + dy * dy < 35.0 * 35.0) {

                            // ブロック破壊
                            pBlock->prev->next = pBlock->next;
                            pBlock->next->prev = pBlock->prev;
                            delete pBlock;
                            blockCount--; // カウント調整

                            // 効果音: ブロック破壊音 (軽快な音)
                            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                            // 分裂弾の生成 (3方向)
                            const int N = 30;
                            for (int i = 0; i < N; i++) {
                                sEnemyShot* pFrag = new sEnemyShot;
                                pFrag->x = pShot->x;
                                pFrag->y = pShot->y;
                                pFrag->muki = (DX_PI * 2.0 / N) * i + GetRand(30) * DX_PI / 180.0 * 0;
                                pFrag->speed = 2.5;
                                // 素材選定: 小玉の黄。分裂した破片のイメージ。
                                pFrag->kind = img_enemyShotSmallBall[1];
                                pFrag->param_i[0] = 2; // Fragment Flag

                                pFrag->prev = pSet->pEnemyShotHead->prev;
                                pFrag->next = pSet->pEnemyShotHead;
                                pSet->pEnemyShotHead->prev->next = pFrag;
                                pSet->pEnemyShotHead->prev = pFrag;
                            }

                            // コアボールは貫通させる（反射させない）ことで、
                            // 1発で複数のブロックを破壊する「連鎖」を促進する
                        }
                    }
                    pBlock = pBNext;
                }
            }
        }
        pShot = pNext;
    }

    // --- 終了判定 ---
    // ブロックがすべてなくなったら終了フラグを立てる
    if (blockCount == 0 && pSet->count > 60) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        pSet->param_i[1] = 1; // 終了フラグ
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_BlockBreak_Qwen()
{
    static int muki;
    static int phase;
    static sEnemyShotSet* pMyShotSet;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 少し硬め
        muki = 1;
        phase = 0;
        pMyShotSet = nullptr;
    }
    else {
        // 敵の移動 (左右往復)
        enemy.x += 1.2 * (double)muki;
        if (enemy.x < 60.0 || enemy.x > 420.0) muki *= -1;
    }

    // --- フェーズ管理 ---
    if (phase == 0) {
        // セットアップ完了を待つ
        if (count > 60) phase = 1;
    }

    if (phase == 1) {
        // パターン開始
        if (pMyShotSet == nullptr) {
            pMyShotSet = new sEnemyShotSet;
            pMyShotSet->count = 0;
            pMyShotSet->patternFunc = ShotPrismBreak;
            pMyShotSet->x = enemy.x;
            pMyShotSet->y = enemy.y;
            pMyShotSet->muki = 0.0;
            pMyShotSet->kind = 0;
            pMyShotSet->param_i[0] = 0;
            pMyShotSet->param_i[1] = 0; // 終了フラグ初期化

            pMyShotSet->pEnemyShotHead = new sEnemyShot;
            pMyShotSet->pEnemyShotHead->prev = pMyShotSet->pEnemyShotHead;
            pMyShotSet->pEnemyShotHead->next = pMyShotSet->pEnemyShotHead;

            // グローバルリストへ登録
            pMyShotSet->prev = enemyShotSetHead.prev;
            pMyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pMyShotSet;
            enemyShotSetHead.prev = pMyShotSet;
        }

        // 終了フラグが立ったらクリーンアップ
        if (pMyShotSet->param_i[1] == 1) {
            // リストから外す
            pMyShotSet->prev->next = pMyShotSet->next;
            pMyShotSet->next->prev = pMyShotSet->prev;

            // 中の弾を全て削除
            sEnemyShot* pShot = pMyShotSet->pEnemyShotHead->next;
            while (pShot != pMyShotSet->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next;
                delete pShot;
                pShot = pNext;
            }
            delete pMyShotSet->pEnemyShotHead;
            delete pMyShotSet;
            pMyShotSet = nullptr;

            // 次のフェーズへ (ここではループさせるため phase 1 のまま、あるいは 2 へ)
            // 簡単のため、少し待機してから再展開
            phase = 2;
        }
    }

    if (phase == 2) {
        // 待機
        if (count % 120 == 0) {
            phase = 1; // 再展開
        }
    }
}