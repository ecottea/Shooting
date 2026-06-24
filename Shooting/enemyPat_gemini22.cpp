// enemyPat_Rhythm.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：音ゲー風パターン (BPM120想定)
static void ShotRhythmGame(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // --- 弾の生成 ---

    // 【1】 四つ打ちのキック（ドンッ）：30フレーム(0.5秒)ごとの全方位リング弾
    if (c % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 拍ごとに少しずつ角度をズラして視覚的な回転を演出
        double baseAngle = (c / 30) * 0.15;
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * (DX_PI * 2.0 / 16.0);
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotLargeBall[5]; // マゼンタの大玉（目立つノーツ）

            // リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 【2】 タップノーツ（裏拍）：15フレーム周期（キックと被らないタイミング）のレーン落下弾
    if (c % 15 == 0 && c % 30 != 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 5レーン（x = 80, 160, 240, 320, 400）のうち、ランダムに3レーンへノーツを降らせる
        int lanes[5] = { 0 };
        int spawned = 0;
        while (spawned < 3) {
            int r = GetRand(4); // 0〜4の乱数
            if (lanes[r] == 0) {
                lanes[r] = 1;
                spawned++;

                sEnemyShot* pEnemyShot = new sEnemyShot;
                // 敵のY座標から発射するが、X座標はレーンに依存させる
                pEnemyShot->x = 80.0 + r * 80.0;
                pEnemyShot->y = pEnemyShotSet->y - 10.0;
                pEnemyShot->muki = DX_PI / 2.0; // 真下へ落下
                pEnemyShot->speed = 5.0; // ノーツらしいハイスピード
                pEnemyShot->kind = img_enemyShotDiamond[3]; // シアンの菱形弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 【3】 ロングノーツ / ストリーム：自機周辺を波打つように狙う細かい連続弾
    if (c % 4 == 0) {
        // 効果音が鳴りすぎないよう間引く
        if (c % 16 == 0) {
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // 自機を狙う角度に、サイン波で揺らぎを加える（レーザー状の弾幕が蛇行する）
        double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->muki = targetAngle + sin(c * 0.1) * 0.35;
        pEnemyShot->speed = 3.5;
        pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄色の小玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- 弾の移動更新 ---
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定関数名）
void EnemyPat_RhythmGame_Gemini()
{
    // 振り子のようにメトロノーム的な動きをするための角度状態
    static double moveAngle = 0.0;

    if (count == 1) {
        enemy.x = 240.0; // 画面中央
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        moveAngle = 0.0;
    }
    else {
        // メトロノームのように左右に揺れる（BPMに同期させるとより音ゲーらしくなる）
        // 1往復120フレーム（2秒 = BPM120の1小節に相当）
        moveAngle += (DX_PI * 2.0) / 120.0;
        enemy.x = 240.0 + sin(moveAngle) * 100.0;
    }

    // 開始から60フレーム後に「音ゲー弾幕の管理セット」を1つだけ生成する
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRhythmGame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ShotSet（発射元）が敵に追従するように座標を毎フレーム更新
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->patternFunc == ShotRhythmGame) {
            pSet->x = enemy.x;
            pSet->y = enemy.y;
        }
        pSet = pSet->next;
    }
}