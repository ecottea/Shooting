// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：スネーク・トラップ（白蛇の檻）
// 左右のワインダーで壁を作り、狭まったタイミングで自機狙い3WAYを放つ
static void ShotSnakeTrap(sEnemyShotSet* pEnemyShotSet)
{
    // --- 1. ワインダー（壁）の生成 (2フレームに1回) ---
    if (pEnemyShotSet->count % 2 == 0) {
        // 連射音がうるさくならないよう、6フレームに1回だけ軽いSEを再生
        if (pEnemyShotSet->count % 6 == 0) {
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 【檻の収縮】全体のうねり振幅を大きなサイン波（周期約314フレーム）で変化させる
        // 振幅が周期的に増減することで、安全地帯が狭まったり広がったりします
        double waveRange = 0.25 + 0.15 * sin(pEnemyShotSet->count * 0.02);

        // 【ワインダーのうねり】発射角度を滑らかに変形させる（周期約90フレーム）
        double angleOffset = sin(pEnemyShotSet->count * 0.07) * waveRange;

        // 弾の発射基点は、敵本体（enemy）の現在位置に完全追従させる
        double spawnX = enemy.x;
        double spawnY = enemy.y + 10.0;

        // ── 左側のワインダー生成 ──
        sEnemyShot* pLeft = new sEnemyShot;
        pLeft->x = spawnX - 50.0; // 中央から左にオフセットした砲台
        pLeft->y = spawnY;
        pLeft->muki = (DX_PI / 2.0) + angleOffset; // 真下（90度）を基準にうねらせる
        pLeft->speed = 2.4;
        pLeft->kind = img_enemyShotScale[6]; // 素材：白蛇をイメージして「鱗弾の白」を選択

        // 共通リストの末尾に追加
        pLeft->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pLeft->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLeft;
        pEnemyShotSet->pEnemyShotHead->prev = pLeft;

        // ── 右側のワインダー生成 ──
        sEnemyShot* pRight = new sEnemyShot;
        pRight->x = spawnX + 50.0; // 中央から右にオフセットした砲台
        pRight->y = spawnY;
        pRight->muki = (DX_PI / 2.0) - angleOffset; // 逆位相（マイナス）にすることで、左右対称に挟み込む
        pRight->speed = 2.4;
        pRight->kind = img_enemyShotScale[6]; // 素材：同じく「鱗弾の白」

        // 共通リストの末尾に追加
        pRight->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pRight->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pRight;
        pEnemyShotSet->pEnemyShotHead->prev = pRight;
    }

    // --- 2. 追い詰めの狙い撃ち弾（90フレーム周期） ---
    // ワインダーの周期（約90フレーム）に同期させ、最も檻が狭まるタイミング（45フレーム時点）で発射
    if (pEnemyShotSet->count % 90 == 45) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double spawnX = enemy.x;
        double spawnY = enemy.y + 10.0;

        // 自機（プレイヤー）への基準角度を計算
        double baseMuki = atan2(player.y - spawnY, player.x - spawnX);

        // 自機狙い 3WAY 弾
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pAim = new sEnemyShot;
            pAim->x = spawnX;
            pAim->y = spawnY;
            pAim->muki = baseMuki + i * (12.0 / 180.0 * DX_PI); // 12度間隔の3WAY
            pAim->speed = 3.8; // ワインダーの壁を追い抜いていくように、少し速めの速度を設定
            pAim->kind = img_enemyShotLargeBall[0]; // 素材：危険を知らせるため視認性の高い「大玉の赤」を選択

            // 共通リストの末尾に追加
            pAim->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pAim->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pAim;
            pEnemyShotSet->pEnemyShotHead->prev = pAim;
        }
    }

    // --- 3. 全弾の移動処理 ---
    // ※ count のインクリメントや画面外の消去はメイン側で行われるため、純粋な座標移動のみ記述
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Winder_Gemini()
{
    static int muki;

    // 開幕フレーム（count == 1）での初期化
    if (count == 1) {
        // ゲーム画面 480x480 に対する初期配置
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // HP固定
        muki = 1;

        // ワインダー弾幕全体を管理する sEnemyShotSet を1つ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnakeTrap;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 真下向き基準
        pEnemyShotSet->kind = 0;

        // 弾リストのダミーヘッド（循環構造）を初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体の敵弾セットリスト（enemyShotSetHead）の末尾にリンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体の緩やかな左右移動
        // 固定砲台にするよりも微小に動かすことで、ワインダーの軌道がさらに美しく有機的になります
        enemy.x += 0.4 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }
}