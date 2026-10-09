#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// 補助:ショットセット生成
// ============================================================
static sEnemyShotSet * CreateShotSet(void (*func)(sEnemyShotSet*), double x, double y, int kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->kind = kind;
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// 補助:弾生成
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 補助:ボスの位置を画面内に収める
static void ClampEnemy()
{
    if (enemy.x < 40.0)   enemy.x = 40.0;
    if (enemy.x > 440.0)  enemy.x = 440.0;
    if (enemy.y < 40.0)   enemy.y = 40.0;
    if (enemy.y > 440.0)  enemy.y = 440.0;
}

// ============================================================
// 弾幕:土俵(画面を覆う円環)
//   外周=白の大玉(俵)、内周=橙の中玉(砂)
//   ゆっくり収縮し、フェーズ3では外周弾が6個ずつ分離して内側へカーブして飛来
// ============================================================
static void ShotDohyo(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 外周の俵(白・大玉) 40個
        for (int i = 0; i < 40; i++) {
            sEnemyShot* p = AddShot(pSet, 0, 0, 0, 0, img_enemyShotLargeBall[6]);
            p->param_d[0] = i / 40.0 * 2.0 * DX_PI;  // 中心からの角度
            p->param_d[1] = 230.0;                   // 中心からの距離
            p->param_d[2] = 0.05;                    // 収縮速度
            p->param_i[0] = i;                       // グループ分離用インデックス
            p->param_i[1] = 0;                       // 0:土俵として回転中 1:分離して飛来中
            p->param_i[2] = 185;                     // 収縮の下限半径
        }
        // 内周の砂(橙・中玉) 28個 ※フェーズ3でも固定障害物として残る
        for (int i = 0; i < 28; i++) {
            sEnemyShot* p = AddShot(pSet, 0, 0, 0, 0, img_enemyShotMediumBall[8]);
            p->param_d[0] = (i + 0.5) / 28.0 * 2.0 * DX_PI;
            p->param_d[1] = 190.0;
            p->param_d[2] = 0.04;
            p->param_i[0] = 100 + i;  // 外周と区別するため100番台
            p->param_i[1] = 0;
            p->param_i[2] = 140;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 1) {
            // 分離後:接線方向に飛び出し、徐々に内側へカーブする
            pShot->muki += pShot->param_d[3];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // フェーズ3:外周弾を6個ずつグループで分離させる(param_i[1]は分離グループ番号)
            if (pSet->param_i[0] == 1 && pShot->param_i[0] < 100 &&
                (pShot->param_i[0] % 6) == (pSet->param_i[1] % 6)) {
                pShot->param_i[1] = 1;
                pShot->speed = 2.0;
                pShot->muki = pShot->param_d[0] + DX_PI / 2.0;  // 接線方向へ
                pShot->param_d[3] = 0.02;                       // 内側へ曲がる角速度
            }
            else {
                // 土俵の収縮
                pShot->param_d[1] -= pShot->param_d[2];
                if (pShot->param_d[1] < pShot->param_i[2]) pShot->param_d[1] = pShot->param_i[2];
                pShot->x = pSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
                pShot->y = pSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);
            }
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕:四股(足踏み下ろしの瞬間、環状の衝撃波)
// ============================================================
static void ShotShiko(sEnemyShotSet* pSet)
{
    {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 24; i++) {
            double m = i / 24.0 * 2.0 * DX_PI;
            AddShot(pSet, pSet->x, pSet->y, m, 2.8, img_enemyShotSmallBall[6]);
        }
    }
}

// ============================================================
// 弾幕:張り手(自機へ向かう扇状の高速弾を3連射)
// ============================================================
static void ShotHariite(sEnemyShotSet* pSet)
{
    {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    // 3回に分けて扇状に発射
    if (pSet->count % 12 == 0 && pSet->count <= 24) {
        double base = atan2(player.y - enemy.y, player.x - enemy.x);
        int n = (pSet->kind == 1) ? 7 : 5;  // kind==1 はフェーズ3(弾数増)
        for (int i = 0; i < n; i++) {
            double m = base + (i - (n - 1) / 2.0) * 0.16;
            AddShot(pSet, enemy.x, enemy.y + 10.0, m, 6.5, img_enemyShotBullet[0]);
        }
    }
}

// ============================================================
// 弾幕:突進の残像(橙の小玉が突進軌跡にゆっくり残る)
//   param_i[0]==1 の間だけ生成、突進終了時に EnemyPat 側で 0 にする
// ============================================================
static void ShotChargeTrail(sEnemyShotSet* pSet)
{
    {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }

    if (pSet->param_i[0] == 0) return;
    if (pSet->count % 2 == 0) {
        // GetRand(628) は 0〜628 の 629種類を返すので 100.0 で割って角度にする
        AddShot(pSet, enemy.x, enemy.y, GetRand(628) / 100.0, 0.3, img_enemyShotSmallBall[8]);
    }

}

// ============================================================
// 敵本体のパターン「四股・土俵際の攻防」
//   フェーズ0:仕切り(土俵生成+四股)   t:    0〜599
//   フェーズ1:突っ張り(張り手+突進)   t:  600〜1799
//   フェーズ2:決まり手(土俵崩壊)      t: 1800〜
// ============================================================
void EnemyPat_Tmp()
{
    static int muki;
    static int dashState;      // 0:通常 1:予備動作 2:突進中 3: rest
    static int dashTimer;
    static int superDash;      // クライマックスの大突進フラグ
    static int finalDone;
    static double aimX, aimY;  // 突進方向
    static sEnemyShotSet* pDohyoSet = nullptr;
    static sEnemyShotSet* pTrailSet = nullptr;

    int t = count - 1;
    int phase = (t < 600) ? 0 : (t < 1800) ? 1 : 2;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        dashState = 0;
        dashTimer = 0;
        superDash = 0;
        finalDone = 0;
        pDohyoSet = nullptr;
        pTrailSet = nullptr;
        // 土俵の生成
        pDohyoSet = CreateShotSet(ShotDohyo, 240.0, 240.0, 0);
    }

    // ------------------------------------------------------------
    // フェーズ2:土俵割り(外周弾の分離グループを順送り)
    // ------------------------------------------------------------
    if (pDohyoSet != nullptr && phase == 2) {
        pDohyoSet->param_i[0] = 1;
        if (t % 60 == 0) pDohyoSet->param_i[1]++;
    }

    // ------------------------------------------------------------
    // クライマックス「寄り切り突進」(フェーズ2の後半で一度だけ)
    // ------------------------------------------------------------
    if (phase == 2 && t >= 2700 && finalDone == 0 && dashState != 2) {
        finalDone = 1;
        superDash = 1;
        dashState = 1;
        dashTimer = 90;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ------------------------------------------------------------
    // ボスの移動・突進ステートマシン
    // ------------------------------------------------------------
    if (dashState == 1) {
        // 予備動作:自機から離れるように後退して溜める
        double dx = enemy.x - player.x, dy = enemy.y - player.y;
        double d = sqrt(dx * dx + dy * dy);
        if (d > 0.001) {
            enemy.x += dx / d * 1.5;
            enemy.y += dy / d * 1.5;
        }
        ClampEnemy();
        dashTimer--;
        if (dashTimer <= 0) {
            // 突進開始:この瞬間の自機へ向かってまっすぐ突っ込む
            double m = atan2(player.y - enemy.y, player.x - enemy.x);
            aimX = cos(m);
            aimY = sin(m);
            dashState = 2;
            dashTimer = superDash ? 60 : 45;
            if (pTrailSet != nullptr) pTrailSet->param_i[0] = 0;
            pTrailSet = CreateShotSet(ShotChargeTrail, enemy.x, enemy.y, 0);
            pTrailSet->param_i[0] = 1;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }
    else if (dashState == 2) {
        // 突進中
        double spd = superDash ? 13.0 : (phase == 2 ? 9.0 : 7.5);
        enemy.x += aimX * spd;
        enemy.y += aimY * spd;
        dashTimer--;
        if (enemy.x < 40.0 || enemy.x > 440.0 || enemy.y < 40.0 || enemy.y > 440.0 || dashTimer <= 0) {
            ClampEnemy();
            dashState = 3;
            dashTimer = (phase == 2) ? 40 : 100;
            superDash = 0;
            if (pTrailSet != nullptr) pTrailSet->param_i[0] = 0;
        }
    }
    else if (dashState == 3) {
        // 突進後の間(フェーズ1の間は揺れながら休む)
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 70.0) muki = 1;
        if (enemy.x > 410.0) muki = -1;
        enemy.y += ((70.0 + 15.0 * sin(t * 0.03)) - enemy.y) * 0.05;
        dashTimer--;
        if (dashTimer <= 0) {
            dashState = 1;
            dashTimer = (phase == 2) ? 40 : 70;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // 通常時:左右に揺れながら張り手・四股の構え
        enemy.x += (phase == 2 ? 1.4 : 1.0) * (double)muki;
        if (enemy.x < 70.0) muki = 1;
        if (enemy.x > 410.0) muki = -1;
        enemy.y += ((70.0 + 15.0 * sin(t * 0.03)) - enemy.y) * 0.05;

        // フェーズ1以降は突進を仕掛けてくる
        if (phase >= 1) {
            dashState = 1;
            dashTimer = 60;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // ------------------------------------------------------------
    // 弾幕の生成
    // ------------------------------------------------------------
    // 四股:衝撃波(フェーズが進むと頻度アップ)
    if ((phase == 0 && t % 120 == 60) ||
        (phase == 1 && t % 240 == 60) ||
        (phase == 2 && t % 150 == 30)) {
        CreateShotSet(ShotShiko, enemy.x, enemy.y + 10.0, 0);
    }

    // 張り手:扇状高速弾
    if ((phase == 1 && t % 90 == 30) ||
        (phase == 2 && t % 60 == 10)) {
        CreateShotSet(ShotHariite, enemy.x, enemy.y, phase == 2 ? 1 : 0);
    }

    // ------------------------------------------------------------
    // 斥力フィールド「四十八手・跳ね」
    //   ボスと自機が近いときのみ両者の間に斥力が働く
    //   突進の直前にボスへ寄り、反動で横へはじかれる回避テクニックが生まれる
    // ------------------------------------------------------------
    {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 100.0 && dist > 0.001) {
            double angle = atan2(dy, dx);              // 押し出される向き(ボス→自機)
            double pullSpeed = (100.0 - dist) * 0.2;   // 斥力の強さ(距離が近いほど強い)
            double ratio = 3.0;                        // 比例定数
            spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);
            // 自機を実際に押し出す
            player.x += pullSpeed * cos(angle) * ratio * 0.02;
            player.y += pullSpeed * sin(angle) * ratio * 0.02;
            // ボスも反動で少し押し戻される(突進中を除く)
            if (dashState != 2) {
                enemy.x -= cos(angle) * 0.5;
                enemy.y -= sin(angle) * 0.5;
            }
        }
    }
}