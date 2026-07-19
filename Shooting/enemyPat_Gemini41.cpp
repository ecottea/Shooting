// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の役割をわかりやすくするための定数定義
enum {
    SHOT_TYPE_BALL = 0,   // 大型反射弾（ボール）
    SHOT_TYPE_BLOCK,      // 静止ターゲット（ブロック）
    SHOT_TYPE_DEBRIS      // ブロックが壊れたときの破片（小型弾）
};

// -----------------------------------------------------------------------------
// ブロック（静止弾）の配置初期化（ピラミッド型に配置）
// -----------------------------------------------------------------------------
static void InitializeBlocks(sEnemyShotSet* pEnemyShotSet)
{
    int colorIdx = 1; // ブロックは「黄」にする（弾の色一覧 1:黄）
    int blockRows = 12; // 4段のピラミッド型

    for (int row = 0; row < blockRows; row++) {
        // 各段のブロック数（1段目:7個, 2段目:5個, 3段目:3個, 4段目:1個）
        int blockCount = 2 * (blockRows - row) - 1;
        double startX = 240.0 - (blockCount - 1) * 20.0; // 中央揃えの開始X座標
        double blockY = 100.0 + (row * 25.0);            // 段ごとのY座標

        for (int i = 0; i < blockCount; i++) {
            sEnemyShot* pBlock = new sEnemyShot;
            pBlock->x = startX + (i * 40.0);
            pBlock->y = blockY;
            pBlock->muki = 0.0;
            pBlock->speed = 0.0; // 静止させる
            pBlock->kind = img_enemyShotMediumBall[colorIdx]; // 中玉を使用

            // 自由パラメータに属性を記録
            pBlock->param_i[0] = SHOT_TYPE_BLOCK; // 弾種: ブロック

            // 双方向リストへ追加
            pBlock->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBlock->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBlock;
            pEnemyShotSet->pEnemyShotHead->prev = pBlock;
        }
    }
}

// -----------------------------------------------------------------------------
// 崩壊障壁「バウンド＆バースト」弾幕処理ルーチン
// -----------------------------------------------------------------------------
static void ShotBounceAndBurst(sEnemyShotSet* pEnemyShotSet)
{
    // 最初のフレームでブロックを生成
    if (pEnemyShotSet->count == 0) {
        InitializeBlocks(pEnemyShotSet);
        pEnemyShotSet->param_i[0] = 0; // 発射したボールの総数をカウントする用
        return; // 次のフレームから本格始動
    }

    // 定期的にボス（パドル）からボール（大型反射弾）を発射
    // 画面内の最大ボール数を制限しつつ、一定間隔（120フレームごと）に発射
    if (pEnemyShotSet->count % 120 == 40) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pBall = new sEnemyShot;
        pBall->x = enemy.x;
        pBall->y = enemy.y + 10.0;

        // 自機方向へのランダムな角度（少し散らす）
        double baseMuki = atan2(player.y - pBall->y, player.x - pBall->x);
        pBall->muki = baseMuki + ((GetRand(60) - 30) / 180.0 * DX_PI);
        pBall->speed = 2.5; // 初期速度
        pBall->kind = img_enemyShotLargeBall[0]; // 大玉・赤

        pBall->param_i[0] = SHOT_TYPE_BALL; // 弾種: ボール
        pBall->param_i[1] = 0;               // 反射（加速）回数カウンター

        // 双方向リストへ追加
        pBall->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBall->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBall;
        pEnemyShotSet->pEnemyShotHead->prev = pBall;

        pEnemyShotSet->param_i[0]++; // ボール発射数インクリメント
    }

    // 弾全体の更新および当たり判定・反射処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 次のループに備えてあらかじめ保持（途中で delete される可能性があるため）
        sEnemyShot* pNextShot = pShot->next;

        // ボール（大型反射弾）の個別処理
        if (pShot->param_i[0] == SHOT_TYPE_BALL) {
            // 移動処理
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 1. 画面左右の壁での反射判定（ゲーム画面横幅：480）
            if (pShot->x < 10.0 && cos(pShot->muki) < 0) {
                pShot->muki = DX_PI - pShot->muki;
                pShot->x = 10.0;
            }
            else if (pShot->x > 470.0 && cos(pShot->muki) > 0) {
                pShot->muki = DX_PI - pShot->muki;
                pShot->x = 470.0;
            }

            // 2. ボス（パドル）による打ち返し判定（画面上部に戻ってきた時）
            if (pShot->y < enemy.y + 20.0 && sin(pShot->muki) < 0) {
                // ボスとのX距離が近ければパドルで打ち返したとみなす（幅60px程度）
                if (fabs(pShot->x - enemy.x) < 30.0) {
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    // 反射して下方向へ（少し自機を狙うように補正）
                    double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->muki = targetMuki + ((GetRand(20) - 10) / 180.0 * DX_PI);

                    // 打ち返すたびに速度を一段階アップ（最大速度制限 6.0）
                    if (pShot->speed < 6.0) {
                        pShot->speed += 0.8;
                    }
                    pShot->y = enemy.y + 20.0;
                }
                // ボスをすり抜けて画面最上部まで行ったら、通常の天井反射
                else if (pShot->y < 10.0) {
                    pShot->muki = -pShot->muki;
                    pShot->y = 10.0;
                }
            }

            // 3. ブロックとの衝突判定
            sEnemyShot* pCheckBlock = pEnemyShotSet->pEnemyShotHead->next;
            while (pCheckBlock != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* pNextBlock = pCheckBlock->next;

                // 対象がブロック弾である場合のみ判定
                if (pCheckBlock->param_i[0] == SHOT_TYPE_BLOCK) {
                    double dist = hypot(pShot->x - pCheckBlock->x, pShot->y - pCheckBlock->y);

                    // ボール大玉(半径10) ＋ ブロック中玉(半径3.5) ＝ 衝突距離 約13.5
                    if (dist < 15.0) {
                        // 【反射処理】簡易的に上下または左右の侵入角度からベクトルを反転
                        if (fabs(pShot->x - pCheckBlock->x) > fabs(pShot->y - pCheckBlock->y)) {
                            pShot->muki = DX_PI - pShot->muki; // 左右反射
                        }
                        else {
                            pShot->muki = -pShot->muki;       // 上下反射
                        }

                        // 【ブロック破壊・破片散布】
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                        // ブロックの位置から全方位に小玉（破片）をばらまく（8方向）
                        for (int d = 0; d < 16; d++) {
                            sEnemyShot* pDebris = new sEnemyShot;
                            pDebris->x = pCheckBlock->x;
                            pDebris->y = pCheckBlock->y;
                            // 均等8方向に、ランダムな微小変化を加える
                            pDebris->muki = (d * 45.0 / 180.0 * DX_PI) + ((GetRand(10) - 5) / 180.0 * DX_PI);
                            pDebris->speed = 1.2 + (GetRand(100) / 100.0); // 1.2〜2.2のランダムな速度
                            pDebris->kind = img_enemyShotSmallBall[3]; // 小玉・シアン色

                            pDebris->param_i[0] = SHOT_TYPE_DEBRIS; // 弾種: 破片

                            // リストに挿入
                            pDebris->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pDebris->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pDebris;
                            pEnemyShotSet->pEnemyShotHead->prev = pDebris;
                        }

                        // 衝突したブロックを削除してリストから解放
                        pCheckBlock->prev->next = pCheckBlock->next;
                        pCheckBlock->next->prev = pCheckBlock->prev;
                        delete pCheckBlock;

                        // 1フレームに複数のブロックに同時衝突するのを防ぐため判定を抜ける
                        break;
                    }
                }
                pCheckBlock = pNextBlock;
            }
        }
        // 破片弾（小型弾）の通常移動処理
        else if (pShot->param_i[0] == SHOT_TYPE_DEBRIS) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNextShot;
    }
}

// -----------------------------------------------------------------------------
// 敵本体のパターン関数（メインルーチンから毎フレーム呼ばれる）
// -----------------------------------------------------------------------------
void EnemyPat_BlockBreak_Gemini()
{
    static int move_dir; // 移動方向（1:右, -1:左）

    // 初期化フレーム（count==1は出現時を想定）
    if (count == 1) {
        // ゲーム画面中央上部に配置
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // HPは200で固定
        move_dir = 1;

        // 弾幕セット（コントロール用親玉）を生成・登録
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBounceAndBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 管理用のダミーヘッダを構築
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体の弾幕セットリスト（enemyShotSetHead）の末尾に結合
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 【パドル移動AI】
        // 通常は緩やかに左右に往復移動するが、戻ってくるボールを受け止めるパドルの動きを演出。
        // 画面端の手前で折り返すように移動制限（左右幅 60〜420 の間を移動）
        enemy.x += 1.5 * (double)move_dir;

        if (enemy.x > 420.0) {
            enemy.x = 420.0;
            move_dir = -1;
        }
        else if (enemy.x < 60.0) {
            enemy.x = 60.0;
            move_dir = 1;
        }
    }
}