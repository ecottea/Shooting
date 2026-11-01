#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// バーンズリーのシダ：IFSで葉の点群を形成し、完成後に葉先から弾を放つ。
static void AddFernShot(sEnemyShotSet* pSet, double x, double y, int kind,
    double muki, int role, double fernX)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = 0.0;
    pShot->kind = kind;
    pShot->param_i[0] = role; // 0:葉の点、1:葉先の胞子、2:幹
    pShot->param_i[1] = 0;    // 発射開始済みフラグ
    pShot->param_d[0] = fernX;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// バーンズリーのシダの反復関数系。
static void BarnsleyStep(double& x, double& y)
{
    const int r = GetRand(99); // 0～99の100通り
    const double oldX = x;
    const double oldY = y;

    if (r == 0) {
        // 幹
        x = 0.0;
        y = 0.16 * oldY;
    }
    else if (r <= 85) {
        // 主な葉の形
        x = 0.85 * oldX + 0.04 * oldY;
        y = -0.04 * oldX + 0.85 * oldY + 1.60;
    }
    else if (r <= 92) {
        // 左側の小葉
        x = 0.20 * oldX - 0.26 * oldY;
        y = 0.23 * oldX + 0.22 * oldY + 1.60;
    }
    else {
        // 右側の小葉
        x = -0.15 * oldX + 0.28 * oldY;
        y = 0.26 * oldX + 0.24 * oldY + 0.44;
    }
}

static void ShotBarnsleyFern(sEnemyShotSet* pEnemyShotSet)
{
    // 初期化と、幹に相当する中玉を配置
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 0.0; // IFSの現在X
        pEnemyShotSet->param_d[1] = 0.0; // IFSの現在Y
        pEnemyShotSet->param_i[0] = 0;   // 生成した葉の点の数

        // 最初の過渡状態を捨て、シダの形が安定してから描画する
        for (int i = 0; i < 24; i++) {
            BarnsleyStep(pEnemyShotSet->param_d[0], pEnemyShotSet->param_d[1]);
        }

        // 中央の幹。中玉を連ねてシダの軸をはっきり見せる
        for (int i = 0; i <= 24; i++) {
            const double y = 365.0 - i * 11.0;
            AddFernShot(pEnemyShotSet, pEnemyShotSet->x, y,
                img_enemyShotMediumBall[2], DX_PI / 2.0, 2, 0.0);
        }
    }

    // 6発ずつ追加し、点群が少しずつシダの形に成長する
    if (pEnemyShotSet->count < 90) {
        for (int i = 0; i < 6; i++) {
            BarnsleyStep(pEnemyShotSet->param_d[0], pEnemyShotSet->param_d[1]);

            const double fernX = pEnemyShotSet->param_d[0];
            const double fernY = pEnemyShotSet->param_d[1];
            const double x = pEnemyShotSet->x + fernX * 41.0;
            const double y = 365.0 - fernY * 27.5;
            const int index = pEnemyShotSet->param_i[0]++;

            // 外側の小葉の一部を胞子に見立て、完成後にプレイヤーへ射出する
            const bool isTip = (fabs(fernX) > 1.55 && GetRand(3) == 0);

            int kind;
            if (isTip) {
                kind = img_enemyShotSmallBall[1]; // 黄：葉先から飛ぶ胞子
            }
            else if (index % 7 == 0) {
                kind = img_enemyShotScale[2]; // 緑の鱗弾：細長い小葉
            }
            else {
                kind = img_enemyShotSmallBall[2]; // 緑の小玉：葉の密度
            }

            // 鱗弾の長軸を左右の小枝に沿わせる
            const double leafAngle = (fernX < 0.0) ? -2.35 : -0.79;
            AddFernShot(pEnemyShotSet, x, y, kind, leafAngle,
                isTip ? 1 : 0, fernX);
        }
    }

    // 完成したシダを少し見せてから、葉先の胞子を発射し、葉全体を流す
    if (pEnemyShotSet->count == 150) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count >= 150) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1;

                if (pShot->param_i[0] == 1) {
                    // 黄色い葉先の胞子は、その時点のプレイヤー位置を狙う
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x)
                        + (GetRand(12) - 6) * DX_PI / 180.0;
                    pShot->speed = 2.5 + GetRand(70) / 100.0;
                }
                else if (pShot->param_i[0] == 2) {
                    // 幹は中央を保ったまま下方へ流れる
                    pShot->muki = DX_PI / 2.0;
                    pShot->speed = 1.35;
                }
                else {
                    // 緑の葉は左右へわずかに広がりながら落ちていく
                    double spread = pShot->param_d[0] * 0.10;
                    if (spread > 0.24) spread = 0.24;
                    if (spread < -0.24) spread = -0.24;
                    pShot->muki = DX_PI / 2.0 + spread;
                    pShot->speed = 1.25 + GetRand(35) / 100.0;
                }
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 敵本体の弾幕パターン
void EnemyPat_BarnsleyFern_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        enemy.x = 240.0 + 45.0 * sin(count * 0.004);
        enemy.y = 45.0 + 8.0 * sin(count * 0.008);
    }

    // シダの成長・発射・消滅を繰り返す
    if (count % 120 == 1) {
        int i = (count / 120) % 3 - 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBarnsleyFern;
        pEnemyShotSet->x = enemy.x + 150 * i;;
        pEnemyShotSet->y = enemy.y + 10.0 + GetRand(100) - 50;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
