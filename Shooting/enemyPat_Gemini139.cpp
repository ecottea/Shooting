#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// 第1層：台風の目（安全地帯とアイウォール）
// ---------------------------------------------------------
static void ShotEyeWall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 36-5; // リングの密度
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (DX_PI * 2.0 / num) * i;

            pShot->param_d[0] = pSet->x; // 中心X
            pShot->param_d[1] = pSet->y; // 中心Y
            pShot->param_d[2] = 40.0;    // 初期の半径
            pShot->param_d[3] = angle;   // 初期の角度

            pShot->kind = img_enemyShotLargeBall[4]; // 青の大弾
            pShot->margin = 240;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 徐々に半径を拡大させながら、反時計回りに回転させる
        pShot->param_d[2] += 0.8;
        pShot->param_d[3] -= 0.01;

        pShot->x = pShot->param_d[0] + pShot->param_d[2] * cos(pShot->param_d[3]);
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * sin(pShot->param_d[3]);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 第2層：暴風域（反時計回りの渦状弾幕）
// ---------------------------------------------------------
static void ShotStormVortex(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (count % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int num = 4; // 4条のらせん
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;

            // pSet->kind を連番として受け取り、発射角度をずらして渦を形成
            double angle = -(pSet->kind * 0.1) + (DX_PI * 2.0 / num) * i;
            pShot->muki = angle;
            pShot->speed = 0.5; // 初速は遅め

            // 水色と青を交互に
            pShot->kind = (pSet->kind % 2 == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[4];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 外側にいくほど風速が上がる（加速）＆反時計回りにカーブ
        pShot->speed += 0.015;
        pShot->muki -= 0.005;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 第3層：強風・突風（曲がる高速弾）
// ---------------------------------------------------------
static void ShotGust(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // dir: 1 なら右から左下へ、-1 なら左から右下へ
        int dir = (GetRand(1) == 0) ? 1 : -1;

        int num = 5 + GetRand(3); // 5〜8発の束
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // ボスのやや上、左右に離れた位置から発射
            pShot->x = pSet->x + dir * (100 + GetRand(60));
            pShot->y = pSet->y - 50 + GetRand(40);

            // やや内側（下方向）を向かせる
            pShot->muki = DX_PI / 2 + dir * (DX_PI / 6);
            pShot->speed = 3.0 + i * 0.4;

            pShot->kind = img_enemyShotMediumOval[6]; // 白の中楕円弾
            pShot->param_d[0] = dir * -0.015; // カーブの強さと方向

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 大きな弧を描いて曲がる
        pShot->muki += pShot->param_d[0];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン：暴風域の眼（アイ・オブ・ザ・タイフーン）
// ---------------------------------------------------------
void EnemyPat_Typhoon_Gemini()
{
    static int muki;
    static int vortex_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0; // 画面やや上部に陣取る
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        vortex_count = 0;
    }
    else {
        // 台風自体がゆっくりと揺れ動くように左右へ移動
        enemy.x += 0.3 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // --- 第2層：暴風域（渦）の生成 ---
    // 常に降り注ぐ雨風を表現するため、4フレームに1回発射し続ける
    if (count % 4 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStormVortex;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->kind = vortex_count++; // 渦の角度をずらすための連番

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- 第1層：台風の目の壁（アイウォール）の生成 ---
    // 160フレームごとに大弾のリングを生成し、外側に押し出していく
    if (count > 60 && count % 160 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotEyeWall;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- 第3層：突風の生成 ---
    // ランダム性を伴いながら、90フレーム付近の間隔で不意打ちのように襲い来る
    if (count > 60 && count % 90 == 45) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGust;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}