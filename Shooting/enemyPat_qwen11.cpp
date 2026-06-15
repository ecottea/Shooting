// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：はかいこうせん（チャージ → 掃射ビーム → スパーク）
static void ShotHakaiKosen(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射開始時に重い効果音を鳴らす
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (pEnemyShotSet->count == 30) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ==========================================
    // フェーズ1：チャージ (0～29フレーム)
    // エネルギーが敵の中心へ収束していくエフェクト
    // ==========================================
    if (pEnemyShotSet->count < 30) {
        if (pEnemyShotSet->count % 2 == 0) {
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                // 円周上に配置
                double angle = (DX_PI * 2.0 / 4.0) * i + pEnemyShotSet->count * 0.2;
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 150.0;
                pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 150.0;

                pEnemyShot->muki = angle; // 初期角度として保存 (旋回計算に使用)
                pEnemyShot->speed = 0.0;  // 0.0 にすることで特別移動モード(後述)へ
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン色の小玉

                // リストに挿入
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // ==========================================
    // フェーズ2：発射 (30～90フレーム)
    // 太いビームを左右に掃射する
    // ==========================================
    else if (pEnemyShotSet->count < 90) {
        // ビームの角度を sin カーブで左右に掃射
        double sweep = sin(pEnemyShotSet->count * 0.05) * 0.8;
        double beam_muki = pEnemyShotSet->muki + sweep;

        // 1. ビームの芯 (白・銃弾・超高速)
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = beam_muki;
        pEnemyShot->speed = 8.0;
        pEnemyShot->kind = img_enemyShotBullet[6]; // 白色
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 2. ビームの外装 (マゼンタ・大玉・少し遅くして角度をずらす)
        for (int i = -1; i <= 1; i += 2) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = beam_muki + i * 0.06; // 芯から少しずらす
            pEnemyShot->speed = 7.0;
            pEnemyShot->kind = img_enemyShotLargeBall[5]; // マゼンタ色
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 3. スパーク (ビームの途中から溢れ出るエネルギー)
        if (pEnemyShotSet->count % 3 == 0) {
            for (int i = 0; i < 4; i++) {
                pEnemyShot = new sEnemyShot;
                // ビームの根元から 40～100 ドットほど離れた位置から生成
                double dist = 40.0 + GetRand(60);
                pEnemyShot->x = pEnemyShotSet->x + cos(beam_muki) * dist;
                pEnemyShot->y = pEnemyShotSet->y + sin(beam_muki) * dist;

                // ビームに対して垂直方向(±90度)へ撒き散らす
                int sign = (GetRand(1) == 0) ? 1 : -1;
                pEnemyShot->muki = beam_muki + (DX_PI / 2.0) * sign + (GetRand(100) - 50) / 200.0;
                pEnemyShot->speed = 2.0 + GetRand(200) / 100.0;
                pEnemyShot->kind = img_enemyShotScale[3]; // シアン色の鱗弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ==========================================
    // 弾の移動処理
    // ==========================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->speed == 0.0) {
            // チャージ弾専用の移動処理 (旋回しながら中心へ縮小)
            double radius = 150.0 - pEnemyShot->count * 5.0;
            if (radius < 0.0) radius = 9999.0; // 中心に到達したらそれ以降は半径0を維持
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(pEnemyShot->muki + pEnemyShot->count * 0.15);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(pEnemyShot->muki + pEnemyShot->count * 0.15);
        }
        else {
            // 通常の移動処理
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hakaikousen_Qwen()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 120フレームごとに新しい弾幕セットを生成
    if (count % 130 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHakaiKosen;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        // プレイヤー方向を基準角度とする
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}