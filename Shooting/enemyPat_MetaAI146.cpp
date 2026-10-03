#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 共有で使うプリズム情報
static double g_prismX = 240.0;
static double g_prismY = 180.0;
static double g_prismR = 68.0;
static double g_prismRot = 0.0; // 現在の回転角度

// 色マップ: 虹 7色 -> 弾色 0:赤,8:橙,1:黄,2:緑,3:シアン,4:青,5:マゼンタ
static const int RAINBOW_COLOR[7] = { 0, 8, 1, 2, 3, 4, 5 };

static void ShotSpectrum(sEnemyShotSet* pSet);
static void ShotWhiteBeam(sEnemyShotSet* pSet);
static void ShotPrismFrame(sEnemyShotSet* pSet);

// ------------------------------------------------------------
// 1. プリズムの枠 - 短レーザーで三角形を作る
// ------------------------------------------------------------
static void ShotPrismFrame(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->param_d[0]: 回転角度
    // pEnemyShotSet->param_d[1]: 外接円半径R
    // pEnemyShotSet->param_d[2]: 回転速度
    if (pEnemyShotSet->count == 0) {
        const int DIV_PER_EDGE = 3; // 1辺あたり3本のレーザーで隙間なく覆う
        for (int edge = 0; edge < 3; edge++) {
            for (int k = 0; k < DIV_PER_EDGE; k++) {
                sEnemyShot* p = new sEnemyShot;
                // 素材選定: 短レーザー(64x4.0) 3:シアン = ガラスの質感に最も近い
                p->kind = img_enemyShotLaser[3];
                p->speed = 0.0;
                p->param_i[0] = edge; // どの辺か
                p->param_d[0] = (k + 0.5) / (double)DIV_PER_EDGE; // 辺上の位置t 0-1
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 回転更新
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[2];
    g_prismRot = pEnemyShotSet->param_d[0];
    g_prismX = pEnemyShotSet->x;
    g_prismY = pEnemyShotSet->y;
    g_prismR = pEnemyShotSet->param_d[1];

    double rot = pEnemyShotSet->param_d[0];
    double R = pEnemyShotSet->param_d[1];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int edge = pShot->param_i[0];
        double t = pShot->param_d[0];

        double ang0 = rot + edge * (2.0 * DX_PI / 3.0);
        double ang1 = rot + (edge + 1) * (2.0 * DX_PI / 3.0);
        double x0 = R * cos(ang0);
        double y0 = R * sin(ang0);
        double x1 = R * cos(ang1);
        double y1 = R * sin(ang1);

        pShot->x = pEnemyShotSet->x + (1.0 - t) * x0 + t * x1;
        pShot->y = pEnemyShotSet->y + (1.0 - t) * y0 + t * y1;
        pShot->muki = atan2(y1 - y0, x1 - x0);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 2. 入射光 - 白い小玉の重ね撃ちで太いビームに見せる
// ------------------------------------------------------------
static void ShotWhiteBeam(sEnemyShotSet* pEnemyShotSet)
{
    // param_d[2]=prismX, [3]=prismY, [4]=R
    // param_i[0]=分光スポーン済みフラグ
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        pEnemyShotSet->alive = 999;

        double prismX = pEnemyShotSet->param_d[2];
        double prismY = pEnemyShotSet->param_d[3];

        // 太さ演出: 中心線と左右に少しずらした3ライン
        for (int side = -1; side <= 1; side++) {
            for (int k = 0; k < 28; k++) {
                sEnemyShot* p = new sEnemyShot;
                // 素材選定: 小玉(2.5x2.5) 6:白 - 重ねるとビームに見える最小の弾
                p->kind = img_enemyShotSmallBall[6];
                p->muki = pEnemyShotSet->muki;
                p->speed = 7.5;
                double dist = k * 8.0;
                double perp = side * 4.0;
                p->x = pEnemyShotSet->x + cos(p->muki) * dist + cos(p->muki + DX_PI / 2) * perp;
                p->y = pEnemyShotSet->y + sin(p->muki) * dist + sin(p->muki + DX_PI / 2) * perp;
                p->margin = 480;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    double prismX = pEnemyShotSet->param_d[2];
    double prismY = pEnemyShotSet->param_d[3];
    double prismR = pEnemyShotSet->param_d[4];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    bool needSpawn = false;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        double dx = pShot->x - prismX;
        double dy = pShot->y - prismY;
        if (dx * dx + dy * dy < (prismR + 4.0) * (prismR + 4.0)) {
            // プリズムに当たったら消す = 屈折演出
            if (pEnemyShotSet->param_i[0] == 0) needSpawn = true;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }

    // 初めて当たったフレームで分光を生成
    if (needSpawn && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1;

        sEnemyShotSet* pSpec = new sEnemyShotSet;
        pSpec->count = 0;
        pSpec->patternFunc = ShotSpectrum;
        pSpec->x = prismX;
        pSpec->y = prismY;
        pSpec->muki = pEnemyShotSet->muki; // 入射角をそのまま渡す
        pSpec->param_d[0] = g_prismRot; // スポーン時のプリズム回転
        pSpec->param_d[1] = pEnemyShotSet->muki;
        pSpec->param_d[2] = prismX;
        pSpec->param_d[3] = prismY;
        pSpec->param_d[4] = prismR;
        pSpec->pEnemyShotHead = new sEnemyShot;
        pSpec->pEnemyShotHead->prev = pSpec->pEnemyShotHead;
        pSpec->pEnemyShotHead->next = pSpec->pEnemyShotHead;

        pSpec->prev = enemyShotSetHead.prev;
        pSpec->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSpec;
        enemyShotSetHead.prev = pSpec;
    }
}

// ------------------------------------------------------------
// 3. 分光 - プリズムから7色に分かれる
// ------------------------------------------------------------
static void ShotSpectrum(sEnemyShotSet* pEnemyShotSet)
{
    // param_d[1]=入射角, [4]=R
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double R = pEnemyShotSet->param_d[4];
        double inMuki = pEnemyShotSet->param_d[1];

        for (int c = 0; c < 7; c++) {
            // 屈折角: 赤が小さく、紫が大きく曲がる = (c-3)*8度ずらす
            // GetRandの仕様注意: 0〜xのx+1通りなので微小ランダムを加える
            double spread = (c - 3) * (8.0 * DX_PI / 180.0);
            spread += (GetRand(20) - 10) / 180.0 * DX_PI * 0.15; // 揺らぎ
            double outMuki = inMuki + spread + g_prismRot * 0.25 * 5; // プリズム回転で扇全体が首振り

            // 速度: 赤が速く紫が遅い = 虹の帯が時間差で広がる
            double speed = 3.8 - c * 0.28;
            int col = RAINBOW_COLOR[c];

            for (int k = 0; k < 18; k++) {
                sEnemyShot* p = new sEnemyShot;
                // 素材選定: 小玉 7色 = 分光の筋を最も綺麗に見せられる
                p->kind = img_enemyShotSmallBall[col];
                p->muki = outMuki;
                p->speed = speed;
                double d0 = R + 8.0 + k * 14.0 -50;
                p->x = pEnemyShotSet->x + cos(outMuki) * d0;
                p->y = pEnemyShotSet->y + sin(outMuki) * d0;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Prism_MetaAI()
{
    static int dir;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
        g_prismX = 240.0;
        g_prismY = 180.0;
        g_prismR = 68.0;
        g_prismRot = -DX_PI / 2; // 尖った頂点を上に

        // プリズム枠生成（一度だけ）
        sEnemyShotSet* pFrame = new sEnemyShotSet;
        pFrame->count = 0;
        pFrame->patternFunc = ShotPrismFrame;
        pFrame->x = g_prismX;
        pFrame->y = g_prismY;
        pFrame->param_d[0] = g_prismRot;
        pFrame->param_d[1] = g_prismR;
        pFrame->param_d[2] = 0.6 * DX_PI / 180.0; // 0.6度/フレームでゆっくり回転
        pFrame->pEnemyShotHead = new sEnemyShot;
        pFrame->pEnemyShotHead->prev = pFrame->pEnemyShotHead;
        pFrame->pEnemyShotHead->next = pFrame->pEnemyShotHead;

        pFrame->prev = enemyShotSetHead.prev;
        pFrame->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFrame;
        enemyShotSetHead.prev = pFrame;
    }
    else {
        // 敵本体は左右にゆっくり
        enemy.x += 0.6 * (double)dir;
        if (count % 180 == 0) dir *= -1;
    }

    // 150フレームごとに白い入射光を撃つ
    if (count % 15 == 0) {
        sEnemyShotSet* pWB = new sEnemyShotSet;
        pWB->count = 0;
        pWB->patternFunc = ShotWhiteBeam;
        pWB->x = enemy.x;
        pWB->y = enemy.y + 12.0;
        pWB->muki = atan2(g_prismY - pWB->y, g_prismX - pWB->x);
        pWB->param_d[2] = g_prismX;
        pWB->param_d[3] = g_prismY;
        pWB->param_d[4] = g_prismR;
        pWB->param_i[0] = 0;
        pWB->pEnemyShotHead = new sEnemyShot;
        pWB->pEnemyShotHead->prev = pWB->pEnemyShotHead;
        pWB->pEnemyShotHead->next = pWB->pEnemyShotHead;

        pWB->prev = enemyShotSetHead.prev;
        pWB->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWB;
        enemyShotSetHead.prev = pWB;
    }
}