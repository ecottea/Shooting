#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 切れかけの電球
//
// ・中央に電球を模した大玉を配置
// ・フィラメントを模した螺旋状の弾を発射
// ・点灯中は弾が外側へ進む
// ・消灯中は弾が停止
// ・再点灯時に弾の進行方向が反転
// ・後半ほど点滅が激しくなる
// ============================================================

static const double PI2 = DX_PI * 2.0;

// ------------------------------------------------------------
// 電球の弾幕本体
// ------------------------------------------------------------
static void ShotBulb(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    // --------------------------------------------------------
    // 点滅周期
    // 前半はゆっくり、後半は速く点滅
    // --------------------------------------------------------
    int flickerPeriod;

    if (pSet->count < 360)
        flickerPeriod = 120;
    else if (pSet->count < 360 + 360)
        flickerPeriod = 90;
    else
        flickerPeriod = 60;

    int phase = pSet->count % flickerPeriod;

    // 点灯状態
    bool lit = phase < flickerPeriod / 2;

    // --------------------------------------------------------
    // 点灯した瞬間に新しいフィラメント弾を生成
    // --------------------------------------------------------
    if (phase == 0)
    {
        // 効果音
        //if (pSet->count == 0)
        //{
        //    if (CheckSoundMem(sound_enemyShot_medium))
        //        StopSoundMem(sound_enemyShot_medium);

        //    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        //}

        // 8方向のフィラメント
        for (int i = 0; i < 8; i++)
        {
            pShot = new sEnemyShot;

            double a = PI2 * i / 8.0;

            pShot->x = pSet->x;
            pShot->y = pSet->y;

            pShot->muki = a;
            pShot->speed = 1.25;
            pShot->margin = 480;

            // フィラメント弾
            pShot->kind = img_enemyShotMediumBall[(i + 1) % 8];

            // 螺旋の番号
            pShot->param_i[0] = i;

            // 発生時の角度
            pShot->param_d[0] = a;

            // 反転回数
            pShot->param_i[1] = 0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 中央のフィラメント
        pShot = new sEnemyShot;

        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = 0.0;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotLargeBall[7];

        pShot->param_i[0] = 100;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // --------------------------------------------------------
    // 消灯→点灯の境界で、全弾の進行方向を反転
    // --------------------------------------------------------
    if (phase == 0 && pSet->count > 0)
    {
        pShot = pSet->pEnemyShotHead->next;

        while (pShot != pSet->pEnemyShotHead)
        {
            if (pShot->param_i[0] != 100)
            {
                pShot->muki += DX_PI;
                pShot->param_i[1]++;
            }

            pShot = pShot->next;
        }
    }

    if (phase % (flickerPeriod / 2) == 0)
    {
        pShot = pSet->pEnemyShotHead->next;

        while (pShot != pSet->pEnemyShotHead)
        {
            if (pShot->kind == img_enemyShotLargeBall[1])
            {
                if (CheckSoundMem(sound_enemyShot_light))
                    StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                pShot->kind = img_enemyShotLargeBall[7];
            }
            else if (pShot->kind == img_enemyShotLargeBall[7])
            {
                if (CheckSoundMem(sound_enemyShot_medium))
                    StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                pShot->kind = img_enemyShotLargeBall[1];
            }
            pShot = pShot->next;
        }
    }

    // --------------------------------------------------------
    // 弾の移動
    //
    // 消灯中は完全停止。
    // 点灯すると再び動き出す。
    // --------------------------------------------------------
    pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead)
    {
        if (pShot->param_i[0] == 100)
        {
            // 中央の大玉はその場に固定
            pShot->x = pSet->x;
            pShot->y = pSet->y;
        }
        else if (lit)
        {
            // ------------------------------------------------
            // フィラメント弾
            //
            // 真っ直ぐではなく、時間とともに少し揺れる。
            // これによってフィラメントが螺旋状に見える。
            // ------------------------------------------------
            double t = pShot->count * 0.035;
            double wave = sin(t + pShot->param_i[0] * 0.7) * 0.035;

            double dir = pShot->muki + wave;

            pShot->x += cos(dir) * pShot->speed;
            pShot->y += sin(dir) * pShot->speed;

            // 少しずつ外側へ速度上昇
            pShot->speed += 0.02;
        }

        pShot = pShot->next;
    }

    // --------------------------------------------------------
    // フィラメントの周囲に細い弾を追加
    //
    // 点灯中だけ発生するので、消灯すると弾幕の生成も
    // 一時的に止まる。
    // --------------------------------------------------------
    if (lit && pSet->count % 1 == 0)
    {
        int n = (pSet->count / 1) % 8;

        pShot = new sEnemyShot;

        double a =
            pSet->count * 0.045 +
            n * PI2 / 8.0;

        pShot->x = pSet->x;
        pShot->y = pSet->y;

        pShot->muki = a;
        pShot->speed = 2.0;

        pShot->kind = img_enemyShotDiamond[(n + 1) % 8];

        pShot->param_i[0] = 200;
        pShot->param_d[0] = a;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // --------------------------------------------------------
    // 外周へ進んだ菱形弾は、さらに角度を変えて
    // フィラメントの「切れかけた揺らぎ」を表現する。
    // --------------------------------------------------------
    pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead)
    {
        if (pShot->param_i[0] == 200 && lit)
        {
            double r = pShot->count * 0.025;

            pShot->muki =
                pShot->param_d[0] +
                sin(r) * 0.45;

            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体
// ============================================================
void EnemyPat_FlickeringLight_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1)
    {
        // 画面上部中央
        enemy.x = 240.0;
        enemy.y = 140.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else
    {
        // 電球そのものがわずかに揺れる
        enemy.x += 0.55 * muki;

        if (count % 180 == 90)
            muki *= -1;
    }

    // --------------------------------------------------------
    // 1つの電球弾幕セットを一定間隔で生成
    // --------------------------------------------------------
    if (count == 1)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotBulb;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 18.0;

        // プレイヤー方向を基本向きとして保持
        pSet->muki =
            atan2(
                player.y - pSet->y,
                player.x - pSet->x
            );

        pSet->kind = shot_count++;

        // 弾リストの番兵
        pSet->pEnemyShotHead = new sEnemyShot;

        pSet->pEnemyShotHead->prev =
            pSet->pEnemyShotHead;

        pSet->pEnemyShotHead->next =
            pSet->pEnemyShotHead;

        // セットを登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}