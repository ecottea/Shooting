#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ゆっくり広がる螺旋の波紋（低速弾 ≤1px/frame のみ）
// 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
// 弾の種類: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
// 色: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
static void ShotSlowSpiralWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ効果音を鳴らす（軽めの音で「ゆっくり」感を出す）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 一定間隔で新しい層（8方向の螺旋弾）を追加
    // 層の追加タイミング: 0, 90, 180, 270 フレーム（約1.5秒間隔）
    // 最大4層まで
    const int layerInterval = 90;
    const int maxLayers = 4;
    int layer = pEnemyShotSet->count / layerInterval;

    if (pEnemyShotSet->count % layerInterval == 0 && layer < maxLayers) {
        // この層の基本角度オフセット（層ごとに少しずらす）
        double baseAngle = pEnemyShotSet->muki + layer * (DX_PI / 16.0);

        // 回転方向を層ごとに交互に切り替える
        double omegaSign = (layer % 2 == 0) ? 1.0 : -1.0;

        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;

            // 初期位置は敵位置からわずかにオフセット
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 8方向に均等配置
            pEnemyShot->muki = baseAngle + i * (DX_PI * 2.0 / 8.0);

            // 弾速 0.55〜0.85 px/frame（層が外側になるほど少し遅くして層がすれ違うように）
            // GetRand(x) は 0〜x の整数を返すので注意
            double speedBase = 0.85 - layer * 0.08;
            pEnemyShot->speed = speedBase - GetRand(20) / 100.0;  // 0.65〜0.85 程度に収まる
            if (pEnemyShot->speed < 0.50) pEnemyShot->speed = 0.50;  // 下限保証（1px以下を厳守）

            // 角速度（ゆっくり回転）を param_d[0] に保存
            // 0.25〜0.45 度/frame 相当 → ラジアンに変換
            double omegaDeg = 0.30 + GetRand(15) / 100.0;  // 0.30〜0.45
            pEnemyShot->param_d[0] = omegaSign * (omegaDeg * DX_PI / 180.0);

            // 色はシアン〜青系を中心に（ゆっくり感を強調）
            // 小玉を使用（低速なので視認性を確保しつつ密度を上げやすくする）
            int color = 3 + (layer % 2);  // 3:シアン or 4:青
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム弾を更新（移動＋ゆっくり回転）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 角速度で向きを少しずつ変える（螺旋を描く）
        pShot->muki += pShot->param_d[0];

        // 低速で直進成分を進める
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
// 新しく作成する場合、名前は void EnemyPat_SlowBullet_Grok() にすること。
void EnemyPat_SlowBullet_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 250; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動（低速弾パターンに合わせて動きも穏やかに）
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) {
            muki *= -1;
        }
        // 周期的に反転も入れて変化を付ける
        if (count % 180 == 90) muki *= -1;
    }

    // 一定間隔で新しいショットセットを生成（層の追加はショットセット内で行う）
    // 約4秒ごとに新しい波紋を開始
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlowSpiralWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // 自機方向を基準にするが、固定でも可
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}