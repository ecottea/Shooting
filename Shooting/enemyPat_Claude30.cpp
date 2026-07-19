// enemyPat_Tmp.cpp
// モチーフ：水平二連式ショットガン（ダブルバレル・ストーム）
//
// 【弾幕構造（1サイクル = 115f）】
//   フェーズ①  0〜19f  照準（待機）
//   フェーズ②  20f     第一射：密散弾 / ±15度 / 13発 / 速7.0 / 橙銃弾
//   フェーズ③  21〜44f コッキング（待機）
//   フェーズ④  45f     第二射：疎散弾 / ±35度 /  9発 / 速4.0±ブレ / 黄鱗弾
//   フェーズ⑤  46〜114f リロード（完全無弾・プレイヤーの動き直しの隙）
//
// 【素材選択】
//   第一射 : img_enemyShotBullet[8]  橙色銃弾  … 弾頭形状でショットガン感、橙で火薬の熱さを表現
//   第二射 : img_enemyShotScale[1]   黄色鱗弾  … 弾種を変えて二連射感を演出、黄色で低速弾を視認しやすく
//   SE一射 : sound_enemyShot_heavy   重い射撃音 … 第一射の「ドン！」感
//   SE二射 : sound_enemyShot_medium  中程度の音 … 第二射の「トン」で重さを差別化

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ─────────────────────────────────────────────────
//  弾幕関数
// ─────────────────────────────────────────────────
static void ShotDoubleBarrel(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ── フェーズ② 第一射 ──────────────────────────────
    // 密散弾：±15度の扇に13発を等間隔配置、速7.0、橙銃弾
    // 第一射の角度範囲が狭いためプレイヤーは端へ逃げたくなる
    // → その逃げ先を第二射の広い扇がカバーする設計
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double aim = pEnemyShotSet->muki;
        for (int i = 0; i < 13; i++) {
            // -15度〜+15度を12分割（2.5度間隔）
            double angle = aim + (-15.0 + 2.5 * i) * DX_PI / 180.0;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 7.0;
            pEnemyShot->kind = img_enemyShotBullet[8]; // 橙色銃弾

            // 双方向リストへ追加（末尾）
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ── フェーズ④ 第二射 ──────────────────────────────
    // 疎散弾：±35度の扇に9発を等間隔配置、速4.0+ランダムブレ、黄鱗弾
    // 第一射を避けて斜め方向へ逃げたプレイヤーをカバー
    // ブレにより「ここなら絶対安全」なルートを消す
    if (pEnemyShotSet->count == 15) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double aim = pEnemyShotSet->muki;
        for (int i = 0; i < 9; i++) {
            // -35度〜+35度を8分割（8.75度間隔）
            double angle = aim + (-35.0 + 8.75 * i) * DX_PI / 180.0;

            // GetRand(10) = 0〜10（11種類）→ -5〜+5 → ×0.1 = -0.5〜+0.5のブレ
            double speed = 4.0 + (GetRand(10) - 5) * 0.1;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotScale[1]; // 黄色鱗弾

            // 双方向リストへ追加（末尾）
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ── 毎フレーム：弾の移動処理 ──────────────────────
    // count/pEnemyShotSet->count のインクリメントと画面外弾消去はメインルーチンが行う
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ─────────────────────────────────────────────────
//  敵本体のパターン
// ─────────────────────────────────────────────────
void EnemyPat_Shotgun_Claude()
{
    static int muki; // 移動方向（+1:右 / -1:左）

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右移動：120fで折り返し
        enemy.x += 1.2 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ── 115fごとに弾幕セットを生成 ──────────────────
    // count % 115 == 1 とすることで count==1（初期化直後）から弾幕開始
    // 照準20f + コッキング25f + リロード70f = 115f/サイクル
    if (count % 55 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoubleBarrel;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // 生成時点のプレイヤー方向を向きとして固定（第一射・第二射で共有）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        // センチネルノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストへ追加（末尾）
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}