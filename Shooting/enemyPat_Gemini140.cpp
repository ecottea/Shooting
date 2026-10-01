#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：ペーパー・シュレッド（機密文書細断）
static void ShotPaperShred(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 1. 書類の投入フェーズ（未裁断の「紙」を作成）
    if (pEnemyShotSet->count == 0) {
        // 投入音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int rows = 8;   // 書類の縦方向の弾数
        const int cols = 12;  // 書類の横方向の弾数
        const double spaceX = 10.0; // 横間隔
        const double spaceY = 10.0; // 縦間隔

        // 長方形ブロックの中心が pEnemyShotSet の位置になるよう計算
        double startX = pEnemyShotSet->x - ((cols - 1) * spaceX) / 2.0;
        double startY = pEnemyShotSet->y - ((rows - 1) * spaceY) / 2.0;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = startX + c * spaceX;
                pEnemyShot->y = startY + r * spaceY;
                pEnemyShot->muki = DX_PI / 2.0; // 真下へ滑り落ちる
                pEnemyShot->speed = 1.5;         // 低速で下降

                // 未裁断の書類：白い小丸弾
                pEnemyShot->kind = img_enemyShotSmallBall[6];

                // 内部パラメータ設定
                pEnemyShot->param_i[0] = 0; // 状態 (0:未裁断の書類, 1:裁断された紙片)
                pEnemyShot->param_i[1] = c; // 列番号（散らばる方向の制御用）
                pEnemyShot->param_d[0] = GetRand(314) / 100.0; // 左右揺れ用の位相（0.0～3.14）

                // リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 裁断ラインの設定（画面中央付近 Y=220）
    const double shredLineY = 220.0-50;
    bool playShredSound = false;

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 状態0：未裁断（書類の塊として落下）
        if (pShot->param_i[0] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 裁断ラインに到達した瞬間、紙片（銃弾）に変化して分裂
            if (pShot->y >= shredLineY) {
                pShot->param_i[0] = 1; // 裁断済み状態へ変化

                // 素材の変更：白い銃弾（細長い紙片に見立てる）
                pShot->kind = img_enemyShotBullet[6];

                // 2. 細断の瞬間（列ごとに左右へ拡散）
                int col = pShot->param_i[1];
                double angleOffset = 0.0;

                if (col % 2 == 0) {
                    // 偶数列：左斜め下方向へ飛散
                    angleOffset = -0.05 - (GetRand(25) / 100.0);
                }
                else {
                    // 奇数列：右斜め下方向へ飛散
                    angleOffset = 0.05 + (GetRand(25) / 100.0);
                }

                pShot->muki = (DX_PI / 2.0) + angleOffset;
                pShot->speed = 2.8 + (GetRand(120) / 100.0); // 裁断の勢いで加速

                playShredSound = true;
            }
        }
        // 状態1：裁断後（ひらひら舞う紙片）
        else {
            // 基本的な直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 3. 紙屑の飛散フェーズ（風圧による左右の揺れを付加）
            pShot->x += sin(pShot->count * 0.15 + pShot->param_d[0]) * 0.7;
        }

        pShot = pShot->next;
    }

    // 裁断音を再生（同一フレーム内で重ならないよう1回のみ鳴らす）
    if (playShredSound) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
}

// 敵本体のパターン
void EnemyPat_Shredder_Gemini()
{
    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0; // 画面中央 (480x480)
        enemy.y = 60.0;  // 画面上部
        enemy.maxHp = enemy.hp = 200; // HP規定値
    }
    else {
        // 敵の移動：画面上部を左右にゆったり往復
        enemy.x = 240.0 + sin(count * DX_PI / 120.0) * 150.0;
    }

    // 160フレーム周期で「書類」を1枚ずつ投入
    if (count % 50 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPaperShred;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;

        // ダミーヘッドノード生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}