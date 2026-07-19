// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace
{

    constexpr double PI = DX_PI;

    //------------------------------------------------------------
    // EnemyShot を生成
    //------------------------------------------------------------
    static sEnemyShot* CreateShot(
        sEnemyShotSet* pEnemyShotSet,
        double x,
        double y,
        double muki,
        double speed,
        int kind)
    {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = speed;
        pEnemyShot->kind = kind;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        return pEnemyShot;
    }

    //------------------------------------------------------------
    // ShotSet生成
    //------------------------------------------------------------
    static sEnemyShotSet* CreateShotSet(
        sEnemyShotSet::PatternFunc func,
        double x,
        double y)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = func;

        pEnemyShotSet->x = x;
        pEnemyShotSet->y = y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        for (int i = 0; i < 4; i++)
        {
            pEnemyShotSet->param_i[i] = 0;
            pEnemyShotSet->param_d[i] = 0.0;
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        return pEnemyShotSet;
    }

    //------------------------------------------------------------
    // 全弾共通移動
    //------------------------------------------------------------
    static void MoveShot(sEnemyShotSet* pEnemyShotSet)
    {
        sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
        {
            pEnemyShot->x +=
                cos(pEnemyShot->muki) * pEnemyShot->speed;

            pEnemyShot->y +=
                sin(pEnemyShot->muki) * pEnemyShot->speed;

            pEnemyShot = pEnemyShot->next;
        }
    }

    //------------------------------------------------------------
    // 色→画像
    //------------------------------------------------------------
    static int GetMediumBall(int color)
    {
        return img_enemyShotMediumBall[color % COL_VAR];
    }

    static int GetLargeBall(int color)
    {
        return img_enemyShotLargeBall[color % COL_VAR];
    }

    static int GetBullet(int color)
    {
        return img_enemyShotBullet[color % COL_VAR];
    }

    static int GetDiamond(int color)
    {
        return img_enemyShotDiamond[color % COL_VAR];
    }

    //------------------------------------------------------------
    // 角度正規化
    //------------------------------------------------------------
    static double NormalizeAngle(double rad)
    {
        while (rad > PI)
            rad -= PI * 2.0;

        while (rad < -PI)
            rad += PI * 2.0;

        return rad;
    }

    //------------------------------------------------------------
    // プレイヤー方向
    //------------------------------------------------------------
    static double AimPlayer(double x, double y)
    {
        return atan2(
            player.y - y,
            player.x - x);
    }

} // namespace

//------------------------------------------------------------
// 金閣寺の一枚天井
//------------------------------------------------------------
static void ShotGoldenCeiling(sEnemyShotSet* pEnemyShotSet)
{
    //--------------------------------------------------------
    // 初回のみ生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);

        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        //----------------------------------------------------
        // 横方向に壁を並べる
        //----------------------------------------------------
        constexpr int CELL = 18;
        constexpr int GAP_WIDTH = 4;

        // 画面幅480なので約27個
        const int NUM = 27;

        // 通路開始位置
        int gap = GetRand(NUM - GAP_WIDTH);

        for (int i = 0; i < NUM; i++)
        {
            if (i >= gap && i < gap + GAP_WIDTH)
                continue;

            sEnemyShot* pEnemyShot =
                CreateShot(
                    pEnemyShotSet,
                    i * CELL + CELL * 0.5,
                    -16.0,
                    PI * 0.5,
                    1.9,
                    GetLargeBall(1));      // 黄色

            // 念のため少し余裕を持たせる
            pEnemyShot->margin = 48.0;
        }
    }

    //--------------------------------------------------------
    // 落下
    //--------------------------------------------------------
    sEnemyShot* pEnemyShot =
        pEnemyShotSet->pEnemyShotHead->next;

    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->y += pEnemyShot->speed;
        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------------------
// 金閣寺 回転弾
//
// param_i[0] : レイヤー番号 (0～3)
// param_i[1] : 有効レイヤー数
//------------------------------------------------------------
static void ShotGoldenRotate(sEnemyShotSet* pEnemyShotSet)
{
    //--------------------------------------------------------
    // 初期設定
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        switch (pEnemyShotSet->param_i[0])
        {
        case 0:     // 緑
            pEnemyShotSet->param_d[0] = 0.0;
            break;

        case 1:     // シアン
            pEnemyShotSet->param_d[0] = DX_PI / 2.0;
            break;

        case 2:     // 青
            pEnemyShotSet->param_d[0] = DX_PI;
            break;

        default:    // 紫
            pEnemyShotSet->param_d[0] = DX_PI * 1.5;
            break;
        }
    }

    //--------------------------------------------------------
    // 一定周期で発射
    //--------------------------------------------------------
    if ((pEnemyShotSet->count % 5) == 0 && pEnemyShotSet->count <= 10) // ※ナーフ
    {
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10;

        //----------------------------------------------------
        // レイヤー毎の設定
        //----------------------------------------------------
        double angle = pEnemyShotSet->param_d[0];

        double angleSpeed;
        double shotSpeed;
        int color;

        switch (pEnemyShotSet->param_i[0])
        {
        default:
        case 0:         // 緑
            angleSpeed = 0.050;
            shotSpeed = 2.2;
            color = 2;
            break;

        case 1:         // シアン
            angleSpeed = -0.070;
            shotSpeed = 2.5;
            color = 3;
            break;

        case 2:         // 青
            angleSpeed = 0.095;
            shotSpeed = 2.9;
            color = 4;
            break;

        case 3:         // 紫
            angleSpeed = -0.130;
            shotSpeed = 3.2;
            color = 5;
            break;
        }

        //----------------------------------------------------
        // 8方向リング
        //----------------------------------------------------
        for (int i = 0; i < 8; i++)
        {
            double a = angle + DX_PI * 2.0 * i / 8.0;

            CreateShot(
                pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                a,
                shotSpeed,
                GetMediumBall(color));
        }

        //----------------------------------------------------
        // 回転継続
        //----------------------------------------------------
        pEnemyShotSet->param_d[0] += angleSpeed;
        pEnemyShotSet->param_d[0] =
            NormalizeAngle(pEnemyShotSet->param_d[0]);
    }

    //--------------------------------------------------------
    // 移動
    //--------------------------------------------------------
    MoveShot(pEnemyShotSet);
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Kinkakuji_ChatGPT()
{
    static int moveDir;
    static int phase;

    //--------------------------------------------------------
    // 初期化
    //--------------------------------------------------------
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 48.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        moveDir = 1;
        phase = 1;
    }

    //--------------------------------------------------------
    // 左右移動
    //--------------------------------------------------------
    enemy.x += moveDir * 1.0;

    if (enemy.x < 80.0)
        moveDir = 1;

    if (enemy.x > 400.0)
        moveDir = -1;

    //--------------------------------------------------------
    // フェーズ進行
    //--------------------------------------------------------
    if (count == 240)
        phase = 2;

    if (count == 480)
        phase = 3;

    if (count == 720)
        phase = 4;

    //--------------------------------------------------------
    // 一枚天井
    //--------------------------------------------------------
    if (count % 90 == 0)
    {
        CreateShotSet(
            ShotGoldenCeiling,
            enemy.x,
            enemy.y);
    }

    //--------------------------------------------------------
    // 回転弾
    //--------------------------------------------------------
    if (count % 50 == 0) // ※ナーフ
    {
        for (int layer = 0; layer < phase; layer++)
        {
            sEnemyShotSet* pSet =
                CreateShotSet(
                    ShotGoldenRotate,
                    enemy.x,
                    enemy.y);

            pSet->param_i[0] = layer;
            pSet->param_i[1] = phase;
        }
    }
}
