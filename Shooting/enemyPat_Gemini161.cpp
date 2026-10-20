#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕A: 間隔が狭い箇所と広い箇所が交互に並んだ10way全方位弾を時計回りに回転しながら撃つ
static void ShotLunaticGunA(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base_angle = pEnemyShotSet->param_d[0]; // 時計回りに回転する基準角度
        double speed = 3.0;

        // 10way全方位弾（間隔が狭い箇所と広い箇所が交互に並ぶ）
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 偶数番目と奇数番目でオフセットを加え、間隔の狭い・広いを交互に作る
            double angle_step = 2.0 * DX_PI / 10.0;
            double offset = (i % 2 == 0) ? 0.045 : -0.045;
            pEnemyShot->muki = base_angle + i * angle_step + offset;
            pEnemyShot->speed = speed;

            // 「銃符」にふさわしいシアン色の銃弾を使用
            // 弾の色一覧: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
            pEnemyShot->kind = img_enemyShotBullet[3];

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
        pShot = pShot->next;
    }
}

// 弾幕B: ボスの位置から自機に向かって、1辺の弾数が8の逆三角形を成した密な弾列を撃つ
static void ShotLunaticGunB(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_angle = pEnemyShotSet->param_d[0]; // 自機方向
        double speed = 2.4;

        // 1辺の弾数が8の逆三角形（密な弾列）の生成
        int max_row = 8;
        double spacing = 5.0*2; // 弾同士の間隔

        for (int r = 0; r < max_row; r++) {
            int count_in_row = r + 1; // 行が進むにつれて広がる逆三角形の形

            for (int c = 0; c < count_in_row; c++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // 進行方向（forward）と横方向（right）のオフセット計算
                double forward_offset = r * spacing;
                double right_offset = (c - (count_in_row - 1) / 2.0) * spacing;

                // ボス位置を基準に自機方向へ回転・配置
                pEnemyShot->x = pEnemyShotSet->x + forward_offset * cos(base_angle) - right_offset * sin(base_angle);
                pEnemyShot->y = pEnemyShotSet->y + forward_offset * sin(base_angle) + right_offset * cos(base_angle);

                pEnemyShot->muki = base_angle;
                pEnemyShot->speed = speed;
                // 赤色の銃弾を使用
                pEnemyShot->kind = img_enemyShotBullet[0];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
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
void EnemyPat_LunaticGun_Gemini()
{
    static int muki;
    static double rot_angle; // 弾幕Aの時計回り回転用角度

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // HPは200から変更しない
        muki = 1;
        rot_angle = 0.0;
    }
    else {
        // ボスの移動パターン（左右に往復移動）
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // A: 10way全方位弾を時計回りに回転しながら常時撃ち続ける（例：12フレーム毎）
    if (count % 12 == 0) {
        sEnemyShotSet* pSetA = new sEnemyShotSet;
        pSetA->count = 0;
        pSetA->patternFunc = ShotLunaticGunA;
        pSetA->x = enemy.x;
        pSetA->y = enemy.y;
        pSetA->param_d[0] = rot_angle;
        pSetA->pEnemyShotHead = new sEnemyShot;
        pSetA->pEnemyShotHead->prev = pSetA->pEnemyShotHead;
        pSetA->pEnemyShotHead->next = pSetA->pEnemyShotHead;
        pSetA->prev = enemyShotSetHead.prev;
        pSetA->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetA;
        enemyShotSetHead.prev = pSetA;

        // 時計回りに回転させるため角度を増加
        rot_angle += 0.12;
    }

    // B: 逆三角形の弾列を5つ僅かに時間差を付けて撃つ（1セット発射 → 間髪入れずもう1セット → 少し間を空ける）
    int t = count % 240;
    // セット1 (t = 30, 36, 42, 48, 54) と セット2 (t = 60, 66, 72, 78, 84)
    if ((t >= 30 && t <= 54 && (t - 30) % 6 == 0) ||
        (t >= 60 && t <= 84 && (t - 60) % 6 == 0)) {

        sEnemyShotSet* pSetB = new sEnemyShotSet;
        pSetB->count = 0;
        pSetB->patternFunc = ShotLunaticGunB;
        pSetB->x = enemy.x;
        pSetB->y = enemy.y;
        // 自機方向を計算して渡す
        pSetB->param_d[0] = atan2(player.y - enemy.y, player.x - enemy.x);
        pSetB->pEnemyShotHead = new sEnemyShot;
        pSetB->pEnemyShotHead->prev = pSetB->pEnemyShotHead;
        pSetB->pEnemyShotHead->next = pSetB->pEnemyShotHead;
        pSetB->prev = enemyShotSetHead.prev;
        pSetB->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetB;
        enemyShotSetHead.prev = pSetB;
    }
}