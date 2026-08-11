// enemyPat_WatermelonGame.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 指定された sEnemyShotSet が持つすべての弾を削除するヘルパー関数
static void ClearAllShots(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        // リストから外す
        pShot->prev->next = pShot->next;
        pShot->next->prev = pShot->prev;
        delete pShot;
        pShot = pNext;
    }
}

// 果実を構成する個々の弾を生成するヘルパー関数
static void AddFruitPart(sEnemyShotSet* pSet, double x, double y, double speed, double muki, int sizeType, int color) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;

    // 弾の種類と色に応じた画像ハンドルを設定
    // sizeType: 0=Small, 1=Medium, 2=Large, 3=Scale(種用)
    // color: 0=赤, 1=黄, 2=緑, 3=シアン, 4=青, 5=マゼンタ, 6=白, 7=黒, 8=橙
    switch (sizeType) {
    case 0: pShot->kind = img_enemyShotSmallBall[color]; break;
    case 1: pShot->kind = img_enemyShotMediumBall[color]; break;
    case 2: pShot->kind = img_enemyShotLargeBall[color]; break;
    case 3: pShot->kind = img_enemyShotScale[color]; break;
    default: pShot->kind = img_enemyShotSmallBall[color]; break;
    }

    // 双方向連結リストに追加
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕：スイカゲーム風合成弾幕
static void ShotWatermelonGame(sEnemyShotSet* pEnemyShotSet) {
    // 90フレーム(約1.5秒)ごとにフェーズが進行し、6フェーズで1サイクル
    int phase = pEnemyShotSet->count / 60;
    int frameInPhase = pEnemyShotSet->count % 60;
    if (phase >= 5) frameInPhase = pEnemyShotSet->count - 60 * 5;

    // ボスのX座標に追従させる
    pEnemyShotSet->x = enemy.x;

    if (frameInPhase == 1) {
        // フェーズ切り替え時に前の弾をすべて消去（合体演出）
        ClearAllShots(pEnemyShotSet);

        if (phase < 5) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 生成基準座標（だんだん下に出現させる）
        double baseX = pEnemyShotSet->x;
        double baseY = pEnemyShotSet->y + 40.0 + phase * 60.0;
        double speed = 1.8 - phase * 0.15;
        double muki = DX_PI / 2.0; // 真下へ落下

        switch (phase) {
        case 0: // チェリー (赤・小)
            AddFruitPart(pEnemyShotSet, baseX - 20, baseY, speed, muki, 0, 0);
            AddFruitPart(pEnemyShotSet, baseX + 20, baseY, speed, muki, 0, 0);
            break;
        case 1: // グレープ (マゼンタ・中 + 小)
            AddFruitPart(pEnemyShotSet, baseX, baseY, speed, muki, 1, 5);
            for (int i = 0; i < 6; i++) {
                double ang = i * DX_PI / 3.0;
                AddFruitPart(pEnemyShotSet, baseX + cos(ang) * 15, baseY + sin(ang) * 15, speed, muki, 0, 5);
            }
            break;
        case 2: // オレンジ (橙・中)
            AddFruitPart(pEnemyShotSet, baseX, baseY, speed, muki, 1, 8);
            for (int i = 0; i < 4; i++) {
                double ang = i * DX_PI / 2.0;
                AddFruitPart(pEnemyShotSet, baseX + cos(ang) * 20, baseY + sin(ang) * 20, speed, muki, 1, 8);
            }
            break;
        case 3: // メロン (緑・大 + 黄・小)
            AddFruitPart(pEnemyShotSet, baseX, baseY, speed, muki, 2, 2);
            for (int i = 0; i < 8; i++) {
                double ang = i * DX_PI / 4.0;
                AddFruitPart(pEnemyShotSet, baseX + cos(ang) * 30, baseY + sin(ang) * 30, speed, muki, 0, 1);
            }
            break;
        case 4: // スイカ (緑・大 + 赤・中)
            AddFruitPart(pEnemyShotSet, baseX, baseY, speed, muki, 2, 2);
            for (int i = 0; i < 6; i++) {
                double ang = i * DX_PI / 3.0;
                AddFruitPart(pEnemyShotSet, baseX + cos(ang) * 35, baseY + sin(ang) * 35, speed, muki, 1, 0);
            }
            break;
        case 5: // スイカ割り (種まき + 破片)
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // 種弾 (黒・鱗弾)
            for (int i = 0; i < 24; i++) {
                double ang = i * DX_PI / 12.0;
                AddFruitPart(pEnemyShotSet, baseX, baseY, 3.5, ang, 3, 7);
            }
            // 果肉の破片 (赤・中玉と緑・大玉)
            for (int i = 0; i < 12; i++) {
                double ang = i * DX_PI / 6.0 + 0.2;
                AddFruitPart(pEnemyShotSet, baseX, baseY, 2.5, ang, 1, 0);
            }
            for (int i = 0; i < 8; i++) {
                double ang = i * DX_PI / 4.0 + 0.4;
                AddFruitPart(pEnemyShotSet, baseX, baseY, 2.0, ang, 2, 2);
            }
            break;
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
void EnemyPat_SuikaGame_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ボスは左右にゆっくり揺れる
        enemy.x += 1.8 * cos(count * 0.03);
    }

    if (count % 150 == 1) {
        // スイカゲーム弾幕を管理する親セットを1つだけ生成
        sEnemyShotSet* pMainSet = new sEnemyShotSet;
        pMainSet->count = 0;
        pMainSet->patternFunc = ShotWatermelonGame;
        pMainSet->x = enemy.x;
        pMainSet->y = enemy.y;
        pMainSet->muki = DX_PI / 2.0;
        pMainSet->kind = 0;

        pMainSet->pEnemyShotHead = new sEnemyShot;
        pMainSet->pEnemyShotHead->prev = pMainSet->pEnemyShotHead;
        pMainSet->pEnemyShotHead->next = pMainSet->pEnemyShotHead;

        pMainSet->prev = enemyShotSetHead.prev;
        pMainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMainSet;
        enemyShotSetHead.prev = pMainSet;
    }
}