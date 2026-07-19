// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：クロースレンジ・バースト（近接信管の散弾）
static void ShotCloseRangeBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 1. 発射の瞬間（セットカウント0）に、スラッグ弾（大玉）を1発だけ生成
    if (pEnemyShotSet->count == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 7.5;                       // ショットガンらしい高速な初速
        pEnemyShot->kind = img_enemyShotLargeBall[0];   // 素材選択：大玉・赤(0)
        pEnemyShot->count = 0;
        pEnemyShot->param_i[0] = 0;                    // 自由パラメータ0番をフラグに使用 (0: スラッグ弾)

        // 双方向リストの末尾（ダミーヘッドのprev）に繋ぐ
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 2. 弾の移動および炸裂判定（リストを巡回）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // ループ中に自身を画面外へ飛ばしたり、末尾に弾を追加したりするため、予め次のポインタを保持する
        sEnemyShot* pNext = pEnemyShot->next;

        if (pEnemyShot->param_i[0] == 0) {
            // 【スラッグ弾の処理】
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // プレイヤーとの距離・位置関係の計算
            double dx = pEnemyShot->x - player.x;
            double dy = pEnemyShot->y - player.y;
            double dist = sqrt(dx * dx + dy * dy);

            // 近接炸裂（信管作動）条件の判定
            bool shouldBurst = false;
            if (dist < 120.0) {
                shouldBurst = true; // 条件A: プレイヤーとの距離が至近距離になった（近接信管）
            }
            else if (pEnemyShot->y >= player.y - 50.0) {
                shouldBurst = true; // 条件B: プレイヤーのY座標付近まで肉薄した
            }
            else if (pEnemyShot->count > 50) {
                shouldBurst = true; // 条件C: 寿命に達した（プレイヤーが横に大きく避けた際の画面外撃ち抜け防止）
            }
            else if (pEnemyShot->y > 440.0) {
                shouldBurst = true; // 条件D: 画面下部付近に到達した
            }

            // 条件を満たしたらその場で炸裂して散弾化
            if (shouldBurst) {
                // 素材選択：炸裂音として中程度の効果音を再生
                if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 散弾（ペレット）を進行方向を中心に扇状に16発ばら撒く
                int numPellets = 16;
                for (int i = 0; i < numPellets; i++) {
                    sEnemyShot* pPellet = new sEnemyShot;
                    pPellet->x = pEnemyShot->x;
                    pPellet->y = pEnemyShot->y;

                    // 元の進行方向を中心に±50度の範囲へランダムに拡散
                    // GetRand(100) - 50 により -50 〜 50 の範囲を生成
                    double angleOffset = (GetRand(100) - 50) / 180.0 * DX_PI;
                    pPellet->muki = pEnemyShot->muki + angleOffset;

                    // 散弾の初速にバラつきをもたせる (2.0 〜 4.5)
                    // GetRand(250) は 0〜250。100.0で割ることで 0.0〜2.5 の実数を生成
                    pPellet->speed = 2.0 + (GetRand(250) / 100.0);

                    pPellet->kind = img_enemyShotBullet[1]; // 素材選択：銃弾・黄(1)
                    pPellet->count = 0;
                    pPellet->param_i[0] = 1;               // 自由パラメータ0番をフラグに使用 (1: 散弾)

                    // 双方向リストの末尾（ダミーヘッドのprev）に繋ぐ
                    pPellet->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pPellet->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pPellet;
                    pEnemyShotSet->pEnemyShotHead->prev = pPellet;
                }

                // スラッグ弾自体は役割を終えたため、メインルーチンに自動消去させるため画面外の極地へ飛ばす
                pEnemyShot->x = -9999.0;
                pEnemyShot->y = -9999.0;
            }
        }
        else {
            // 【散弾の処理】
            // 分裂後の子弾は、割り当てられた個々の速度と向きでそのまま直進移動
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pNext;
    }
}

// 敵本体の行動パターン関数
void EnemyPat_Shotgun_Gemini()
{
    static int muki;
    static double kickback; // ショットガン発射時の反動（ノックバック）量

    if (count == 1) {
        // 初回フレーム：初期化処理（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // HPは200で固定
        muki = 1;
        kickback = 0.0;
    }
    else {
        // 通常時の左右往復移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // 撃った後の反動（ノックバック）を毎フレーム滑らかに減衰させて定位置(y=40.0)に戻す
        if (kickback > 0.0) {
            kickback -= 0.5; // 減衰スピード
            if (kickback < 0.0) kickback = 0.0;
        }
        // 反動によって一時的に敵本体が上方向（yのマイナス方向）へとノックバックする
        enemy.y = 140.0 - kickback;
    }

    // 120フレーム（約2秒）サイクルで、プレイヤーを狙ったショットガンを射出
    if (10 <= count % 120 && count % 120 <= 25 && count % 3 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCloseRangeBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0; // 銃口位置をやや下へオフセット

        // 自機（プレイヤー）の位置に応じた正確なターゲット角度を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        // 子弾リストのダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵全体の弾セットリスト(enemyShotSetHead)の末尾へ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 「ドン！」という発射と同時に、視覚的なノックバック（反動）の最大値をセット
        kickback += 10.0;

        // 素材選択：ショットガンに相応しい重厚な発射音(heavy)を再生
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}