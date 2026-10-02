#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：緩慢なる満潮 (Slow Tide)
static void ShotSlowTide(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回呼び出し時（弾生成時）
    if (pEnemyShotSet->count == 0) {
        // 予告音と発射音で「緩慢だが確実に迫りくる」緊張感を演出
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1セットあたり12発の弾を配置
        int numShots = 12;
        for (int i = 0; i < numShots; i++) {
            pEnemyShot = new sEnemyShot;

            // pEnemyShotSet->muki は自機への角度。
            // そこから円周上に広げ、かつセットごとに位相をずらして渦巻きを形成する
            double baseAngle = pEnemyShotSet->muki + (DX_PI * 2.0 / numShots) * i + (pEnemyShotSet->kind * 0.2);

            // 微細なばらつきを持たせる (GetRand(20)は0〜20の21種類)
            pEnemyShot->muki = baseAngle + (GetRand(20) - 10) / 100.0;

            // 速度は 0.2 〜 0.6 px/frame (60fps環境で1秒間に12px〜36px)
            pEnemyShot->speed = 0.2 + GetRand(40) / 100.0;

            // 初期位置は敵の中心から少しずらし、重なりを解消して「胞子」のような見た目にする
            pEnemyShot->x = pEnemyShotSet->x + cos(baseAngle) * 12.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(baseAngle) * 12.0;

            // 弾の種類：小玉(2.5x2.5) を使用。色は初期状態で青(4)
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 螺旋運動：角度をゆっくりと変化させる (60fpsで1秒間に約50度回転)
        // これにより、自機方向へ進みつつも渦を巻くような軌道になる
        pShot->muki += 0.015;

        // 位置更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 時間経過による色の変化（満潮の表現）
        // pShot->count はメインルーチンで自動インクリメントされる仕様を利用
        if (pShot->count < 120) {
            pShot->kind = img_enemyShotSmallBall[4]; // 青 (発生直後、比較的安全)
        }
        else if (pShot->count < 240) {
            pShot->kind = img_enemyShotSmallBall[5]; // マゼンタ (警告色)
        }
        else if (pShot->count < 360) {
            pShot->kind = img_enemyShotSmallBall[0]; // 赤 (危険)
        }
        else {
            pShot->kind = img_enemyShotSmallBall[8]; // 橙 (極めて危険な滞留弾)
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SlowBullet_Qwen()
{
    static int shot_cycle;
    
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0; // 弾が広がるスペースを確保するため、やや下めに配置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        shot_cycle = 0;
    }
    else {
        // 敵は画面上部でゆっくりと左右に往復運動（周期を長くして「緩慢」を強調）
        // 微妙な上下動も加え、生きているような不規則さを出す
        enemy.x = 240.0 + 160.0 * sin(count * 0.008);
        enemy.y = 240.0 + 220.0 * sin(count * 0.012);
    }

    // 弾幕の発射タイミング
    // 15フレームごと（1秒に4回）に新しい弾幕セットを生成し、密度を徐々に高める
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlowTide;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // 自機への角度を計算し、muki に格納する
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // kind をセットの識別子（螺旋の位相ずれ）として使用
        pEnemyShotSet->kind = shot_cycle++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}