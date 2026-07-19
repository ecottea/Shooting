// enemyPat_bomberman.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言
static void BombPattern(sEnemyShotSet* p);
static void CreateExplosionShotEx(sEnemyShotSet* pParent, double cx, double cy, double angle, int delay);
static void CheckBombKick(sEnemyShotSet* pExploding);

// 爆風弾を生成するヘルパー
static void CreateExplosionShotEx(sEnemyShotSet* pParent, double cx, double cy, double angle, int delay)
{
    sEnemyShot* pShot = new sEnemyShot;
    double dist = delay * 6.0;
    pShot->x = cx + cos(angle) * dist;
    pShot->y = cy + sin(angle) * dist;
    pShot->muki = angle + (GetRand(200) - 100) * 0.001;
    pShot->speed = 6.0;
    pShot->kind = img_enemyShotScale[0]; // 赤い鱗弾を爆風として使用
    pShot->param_i[0] = 0; // 本体フラグ(0: 爆風弾)

    // 親Setの弾リストに繋ぐ
    pShot->prev = pParent->pEnemyShotHead->prev;
    pShot->next = pParent->pEnemyShotHead;
    pParent->pEnemyShotHead->prev->next = pShot;
    pParent->pEnemyShotHead->prev = pShot;
}

// ボムキック判定（他の爆弾を弾き飛ばす）
static void CheckBombKick(sEnemyShotSet* pExploding)
{
    double ex = pExploding->x;
    double ey = pExploding->y;

    // 全ShotSetを走査
    sEnemyShotSet* p = enemyShotSetHead.next;
    while (p != &enemyShotSetHead) {
        if (p != pExploding) {
            // 相手が停止中 (State 1) であること
            if (p->param_i[0] == 1) {
                double tx = p->x;
                double ty = p->y;

                // 十字線上にいるかチェック (判定幅 30px)
                bool onCross = false;
                double kickAngle = 0.0;

                if (fabs(ex - tx) < 30.0 && fabs(ey - ty) > 30.0) {
                    onCross = true;
                    kickAngle = (ey < ty) ? DX_PI / 2.0 : -DX_PI / 2.0;
                }
                else if (fabs(ey - ty) < 30.0 && fabs(ex - tx) > 30.0) {
                    onCross = true;
                    kickAngle = (ex < tx) ? 0.0 : DX_PI;
                }

                if (onCross) {
                    // キック状態へ遷移
                    p->param_i[0] = 3; // State: Kick
                    p->param_d[0] = 10.0; // Speed
                    p->param_d[1] = kickAngle; // Angle

                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
            }
        }
        p = p->next;
    }
}

// 爆弾の挙動パターン
static void BombPattern(sEnemyShotSet* p)
{
    sEnemyShot* pShot;

    // 【重要】count == 0 のとき、爆弾本体の弾を生成してリストに繋ぐ
    if (p->count == 0) {
        pShot = new sEnemyShot;
        pShot->x = p->x;
        pShot->y = p->y;
        pShot->muki = 0.0;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotLargeBall[7]; // 黒爆弾
        pShot->param_i[0] = 1; // 本体フラグ(1: 爆弾本体)

        // リストに繋ぐ
        pShot->prev = p->pEnemyShotHead->prev;
        pShot->next = p->pEnemyShotHead;
        p->pEnemyShotHead->prev->next = pShot;
        p->pEnemyShotHead->prev = pShot;

        p->param_i[0] = 0; // State: Fall
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    int state = p->param_i[0];

    // 状態更新
    if (state == 0) { // 落下
        p->y += p->param_d[2];
        if (p->y >= p->param_d[3]) {
            p->y = p->param_d[3];
            p->param_i[0] = 1; // State: Stop
            p->param_i[1] = 90; // 爆発タイマー
        }
    }
    else if (state == 1) { // 停止・点滅
        p->param_i[1]--;

        // 点滅表現 (本体の弾の種類を切り替える)
        pShot = p->pEnemyShotHead->next;
        while (pShot != p->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                if (p->count % 10 < 5) {
                    pShot->kind = img_enemyShotLargeBall[7];
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[7];
                }
                break;
            }
            pShot = pShot->next;
        }

        if (p->param_i[1] <= 0) {
            p->param_i[0] = 2; // State: Explode
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            CheckBombKick(p);
        }
    }
    else if (state == 2) { // 爆発
        int t = p->param_i[1];
        if (t <= 20 && t % 2 == 0) {
            int delay = t / 2;
            // 十字方向に爆風弾を生成
            CreateExplosionShotEx(p, p->x, p->y, -DX_PI / 2.0, delay); // 上
            CreateExplosionShotEx(p, p->x, p->y, DX_PI / 2.0, delay);  // 下
            CreateExplosionShotEx(p, p->x, p->y, DX_PI, delay);        // 左
            CreateExplosionShotEx(p, p->x, p->y, 0.0, delay);          // 右
        }
        p->param_i[1]++;
        if (t > 30) {
            // 爆発終了、ShotSetを削除
            //p->prev->next = p->next;
            //p->next->prev = p->prev;
            //delete p;
            pShot = p->pEnemyShotHead->next;
            while (pShot != p->pEnemyShotHead) {
                pShot->x = 9999;
                pShot = pShot->next;
            }
            return;
        }
    }
    else if (state == 3) { // キック中
        double speed = p->param_d[0];
        double angle = p->param_d[1];
        p->x += cos(angle) * speed;
        p->y += sin(angle) * speed;

        // 壁・地面判定
        bool hitWall = false;
        if (p->x < 20.0 || p->x > 460.0) hitWall = true;
        if (p->y > 460.0) {
            p->y = 460.0;
            hitWall = true;
        }

        if (hitWall) {
            p->param_i[0] = 1; // 停止状態へ
            p->param_i[1] = 60; // 再爆発タイマー
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // 爆弾本体の座標を p->x, p->y に合わせる
    pShot = p->pEnemyShotHead->next;
    while (pShot != p->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x = p->x;
            pShot->y = p->y;
        }
        pShot = pShot->next;
    }

    // 爆風弾の座標更新（自力で移動させる）
    pShot = p->pEnemyShotHead->next;
    while (pShot != p->pEnemyShotHead) {
        if (pShot->param_i[0] != 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Bomberman_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.5 * (double)muki;
        if (enemy.x < 40.0 || enemy.x > 440.0) muki *= -1;
    }

    // 爆弾生成 (60フレームに1個)
    if (count % 15 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = BombPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->param_i[0] = 0;
        pSet->param_i[1] = 0;
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = 0.0;
        pSet->param_d[2] = 2.0 + GetRand(3);
        pSet->param_d[3] = 100.0 + GetRand(7) * 50.0;

        // 弾リストの初期化（ダミーのヘッドを作成）
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // Setリストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}