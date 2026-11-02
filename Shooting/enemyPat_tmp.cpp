#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：ヘリンボーン・ストリーム（綾織り模様）
// ============================================================
static void ShotHerringboneStream(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->count はメインルーチンで自動インクリメントされる
    // 初回(count == 0)のみ、帯状に並んだ弾をまとめて生成する
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧から軽量なものを選択
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int streamLength = 40;          // 帯を構成する弾の数
        double angle = pEnemyShotSet->muki;
        double widthOffset = pEnemyShotSet->param_d[0]; // 帯の幅方向へのオフセット値
        double spacing = 10.0;          // 弾同士の配置間隔

        for (int i = 0; i < streamLength; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 帯の進行方向に沿った距離
            double dist = i * spacing;

            // 帯の幅方向（進行方向に対して垂直）のオフセットベクトル
            // ヘリンボーンの「噛み合い」を表現するためにこの値をずらす
            double perpX = cos(angle + DX_PI / 2.0) * widthOffset;
            double perpY = sin(angle + DX_PI / 2.0) * widthOffset;

            // 初期位置の計算
            pEnemyShot->x = pEnemyShotSet->x + dist * cos(angle) + perpX;
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(angle) + perpY;

            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.5; // やや遅めで模様が画面に定着しやすくする

            // 【使用素材】銃弾(5.0x2.0)を「米弾」として使用
            // 細長い形状が方向性を強調し、帯として並んだときに織物のような質感を出す
            // 色は pEnemyShotSet->kind に格納されたインデックス(3:シアン, 5:マゼンタ)を使用
            pEnemyShot->kind = img_enemyShotBullet[pEnemyShotSet->kind];
            pEnemyShot->margin = 500;

            // 両方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        if (pShot->y > 500) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：ヘリンボーン・ストリーム
// ============================================================
void EnemyPat_Tmp()
{
    static int phase = 0; // 0: 45度帯発射フェーズ, 1: 135度帯発射フェーズ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
    }
    else {
        // 敵は画面中央上部でゆっくりと左右に揺れる
        enemy.x = 240.0 + sin(count * 0.015) * 120.0;
    }

    // 30フレームごとに帯状の弾幕を発射（角度とオフセットを交互に切り替え）
    if (count % 30 == 1) for (int i = -5; i <= 5; i++) {
        phase = (phase + 1) % 2;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringboneStream;
        pEnemyShotSet->x = enemy.x + i * 70;
        pEnemyShotSet->y = enemy.y + 10.0;

        if (phase == 0) {
            // 【右斜め下の帯】 (\)
            pEnemyShotSet->muki = DX_PI / 4.0;       // 45度
            pEnemyShotSet->kind = 3;                 // 色: 3(シアン)
            pEnemyShotSet->param_d[0] = 0.0;         // 幅方向オフセット: なし
        }
        else {
            // 【左斜め下の帯】 (/)
            pEnemyShotSet->muki = DX_PI * 3.0 / 4.0; // 135度
            pEnemyShotSet->kind = 5;                 // 色: 5(マゼンタ)
            // 帯の間隔(spacing=10.0)の約半分(6.0)だけ垂直方向にずらす
            // これにより、V字の頂点が一直線に並ぶのではなく、斜めに段差ができる
            // 「本格的なヘリンボーン（綾織り）」の噛み合いが表現される
            pEnemyShotSet->param_d[0] = 6.0;
        }

        // 弾リストのヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}