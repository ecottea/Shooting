// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：永劫吹雪・無限結晶陣
static void ShotEternalForceBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    // 結晶弾の展開（Set生成直後の1回のみ）
    if (pEnemyShotSet->count == 0) {
        // 結晶展開時の重い効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 発射ごとに全体の回転角度を少しずつずらす（kindを連番として利用）
        double baseAngle = pEnemyShotSet->kind * 0.05;

        // 氷の結晶イメージで「菱形弾(シアン)」を使用
        int kind = img_enemyShotDiamond[3];

        // ボスを中心に、間隔30ピクセルの直交座標格子状に生成
        for (int dx = -210; dx <= 210; dx += 30) {
            for (int dy = -210; dy <= 210; dy += 30) {
                // 中心(0,0)はボスの位置と重なるため除外
                if (dx == 0 && dy == 0) continue;

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + dx;
                pEnemyShot->y = pEnemyShotSet->y + dy;

                // 各座標への角度に、全体の回転角度を加算
                double angle = atan2((double)dy, (double)dx) + baseAngle;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 1.8; // ゆっくりと拡大する速度
                pEnemyShot->kind = kind;

                // 双方向リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 吹雪弾の発射（結晶展開の0.5秒後[30フレーム後]から毎フレーム実行）
    if (pEnemyShotSet->count >= 30 && pEnemyShotSet->count % 2 == 0 && pEnemyShotSet->count < 140) {
        // 吹雪イメージで「小玉(白)」を使用
        int kind = img_enemyShotSmallBall[6];

        // 1回につき4発の吹雪を生成
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボスの中心付近からランダムにばらけさせて発射
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 逆時計回りに渦を巻く角度を計算
            // GetRand(100)は0〜100の101通りを返すので、-50〜50の範囲にしてから角度に変換
            double baseBlizzardAngle = -pEnemyShotSet->count * 0.12;
            double offsetAngle = (GetRand(100) - 50) / 100.0 * 0.6;
            pEnemyShot->muki = baseBlizzardAngle + offsetAngle;

            // 3.5 〜 5.5 のランダムな速度で流す
            // GetRand(20)は0〜20の21通りを返す
            pEnemyShot->speed = 3.5 + GetRand(20) / 10.0;
            pEnemyShot->kind = kind;

            // 双方向リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_EternalForceBlizzard_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        // 結晶弾は全方位に広がるため、やや上部に配置すると画面全体を綺麗に埋められる
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 結晶展開の0.5秒前(30フレーム前)に予告音を鳴らす
    // count % 90 == 1 でセットを生成するため、その前のループで鳴らす
    if (count % 90 == 61) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 90フレーム(1.5秒)ごとに弾幕セットを生成
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEternalForceBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // kindを連番(0, 1, 2...)にすることで、展開するたびに結晶の回転角度をずらす
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}