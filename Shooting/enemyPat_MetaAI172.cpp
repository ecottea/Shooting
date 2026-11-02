#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ヘリンボーン弾幕 Herringbone Loom
// 素材選定理由:
//   - 小玉(白) img_enemyShotSmallBall[6] : 円形で向きに依存しないため、斜めに並べても
//     綺麗な織り目の線として見える。サイズ2.5で布の経糸の細かさを表現。
//   - 菱形弾(橙) img_enemyShotDiamond[8] : 折り返しの「節」を強調するアクセント。
//     サイズ4.5x2.5で小玉より一回り大きく、橙色でツイードのネップのように目立つ。
//     2種類だけで柄を成立させる。
// ============================================================
static void ShotHerringboneRow(sEnemyShotSet* pEnemyShotSet)
{
    // 生成フレーム
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, medium, heavy, extreme, sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int dir = pEnemyShotSet->param_i[0]; // 1 = 右上がり "/" , -1 = 左上がり "\"
        // dirが0の保険
        const int safeDir = (dir == 0) ? 1 : dir;

        // 5発の地の糸 = 小玉白
        for (int idx = -2; idx <= 2; ++idx) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ヘリンボーンの斜めの線を作るオフセット
            // "/" の時: xが増えるほどyは減る
            // "\" の時: xが増えるほどyは増える
            // 式: offsetY = -dir * offsetX * tan(30deg) ≒ -dir * offsetX * 0.577
            double offsetX = idx * 22.0;
            double offsetY = -safeDir * offsetX * 0.55;

            // GetRand(x)は0〜xのx+1種類を返すので、-2〜+2の乱数は GetRand(4)-2
            double jitterX = (double)(GetRand(4) - 2)/10;
            double jitterY = (double)(GetRand(4) - 2)/10;

            pEnemyShot->x = pEnemyShotSet->x + offsetX + jitterX;
            pEnemyShot->y = pEnemyShotSet->y + offsetY + jitterY;

            // 進行方向はほぼ真下。dirで少しだけ外側に開き、idxで扇状に広がる
            // これで布が下に流れる動きになる
            double baseMuki = DX_PI / 2.0; // 90度 = 下
            pEnemyShot->muki = baseMuki + safeDir * (6.0 * DX_PI / 180.0) + idx * (1.8 * DX_PI / 180.0);
            pEnemyShot->speed = 2.15 + GetRand(30) / 100.0/10; // 2.15〜2.45

            // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 折り返しの節 = 菱形弾橙を中央に1発。速度を遅くして背骨を形成
        {
            sEnemyShot* pKnot = new sEnemyShot;
            pKnot->x = pEnemyShotSet->x;
            pKnot->y = pEnemyShotSet->y;
            pKnot->muki = DX_PI / 2.0; // 真下
            pKnot->speed = 1.55 + GetRand(10) / 100.0; // 1.55〜1.65 遅い
            pKnot->kind = img_enemyShotDiamond[8]; // 橙菱形

            pKnot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pKnot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pKnot;
            pEnemyShotSet->pEnemyShotHead->prev = pKnot;
        }
    }

    // 移動処理（メイン側でcountインクリメント、画面外削除は行う）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Herringbone_MetaAI()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // ヘリンボーンを織るようにゆっくり左右移動
        enemy.x += 0.55 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 14Fごとに1段を織る。交互に "/" と "\" を生成してヘリンボーンを積み上げる
    if (count % 14 == 1) for (int i = -2; i <= 2; i++) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringboneRow;
        pEnemyShotSet->x = enemy.x + i * 80;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;

        // param_i[0]に斜め方向を渡す。偶数段は右上がり、奇数段は左上がり
        pEnemyShotSet->param_i[0] = (shot_count % 2 == 0) ? 1 : -1;
        pEnemyShotSet->param_i[1] = shot_count; // デバッグ用に行番号保持

        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }
}