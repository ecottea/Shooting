// ============================================================
//  超新星爆発 弾幕パターン  『レムナント・ノヴァ』
//  EnemyPat_Supernova_DeepSeek()
//
//  既存の敵弾素材のみで
//    重力崩壊 → 閃光／衝撃波 → 残骸降着 → 中性子星／パルサー
//  を表現する 12 秒周期のパターン。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  効果音を頭から鳴らすヘルパー
// ------------------------------------------------------------
static void PlaySE(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// ------------------------------------------------------------
//  弾を弾幕セットに追加するヘルパー
//  （param_i[0] 等はプール再利用時に前の値が残るため必ず初期化）
// ------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->count = 0;
    p->kind = kind;
    p->margin = 220.0;
    p->param_i[0] = 0;
    p->param_i[1] = 0;
    p->param_d[0] = 0.0;
    p->param_d[1] = 0.0;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ------------------------------------------------------------
//  弾幕セットを生成するヘルパー
// ------------------------------------------------------------
static sEnemyShotSet* AddSet(sEnemyShotSet::PatternFunc func,
    double x, double y, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = kind;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ============================================================
//  1) 重力崩壊
//     中弾8個＋小弾16個が外側から中心へ収束。
//     中心到達後は逆方向へ弾けて画面外へ抜ける。
// ============================================================
static void ShotCollapse(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySE(sound_enemyCharge);
        // 中弾8個：半径200の円周上から中心へ
        for (int i = 0; i < 8; i++) {
            double ang = i * DX_PI * 2.0 / 8.0;
            sEnemyShot* p = AddShot(pSet,
                pSet->x + 300.0 * cos(ang),
                pSet->y + 300.0 * sin(ang),
                ang + DX_PI, 3.0,
                img_enemyShotMediumBall[8]);          // 橙
            p->param_d[0] = pSet->x;                  // 中心X
            p->param_d[1] = pSet->y;                  // 中心Y
        }
        // 小弾16個：半径220の円周上から中心へ
        for (int i = 0; i < 16; i++) {
            double ang = i * DX_PI * 2.0 / 16.0 + DX_PI / 16.0;
            sEnemyShot* p = AddShot(pSet,
                pSet->x + 320.0 * cos(ang),
                pSet->y + 320.0 * sin(ang),
                ang + DX_PI, 4.0,
                img_enemyShotSmallBall[1]);           // 黄
            p->param_d[0] = pSet->x;
            p->param_d[1] = pSet->y;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) {
            // 収束中
            double dx = p->param_d[0] - p->x;
            double dy = p->param_d[1] - p->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < 30.0) {
                // 中心到達 → 外側へ反転（爆発）
                p->param_i[0] = 1;
                p->muki = atan2(p->y - p->param_d[1],
                    p->x - p->param_d[0]);
                p->speed = 5.0;
            }
            else {
                p->x += p->speed * dx / dist;
                p->y += p->speed * dy / dist;
            }
        }
        else {
            // 拡散中
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  2) 衝撃波と爆風
//     45F周期で3回、
//       大弾24方向（低速・外殻）→
//       中弾36方向（中速）→
//       小弾72方向（高速・デブリ）
//     を角度をずらしながら重ねる。
// ============================================================
static void ShotShockwave(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySE(sound_enemyShot_extreme);
    }
    for (int w = 0; w < 3; w++) {
        int base = w * 45;
        if (pSet->count == base) {
            // 大弾（衝撃波の外殻）
            for (int i = 0; i < 24; i++) {
                double ang = i * DX_PI * 2.0 / 24.0 + w * DX_PI / 24.0;
                AddShot(pSet, pSet->x, pSet->y,
                    ang, 1.6 + w * 0.2,
                    img_enemyShotLargeBall[8]);       // 橙
            }
        }
        if (pSet->count == base + 15) {
            // 中弾（爆風の主成分）
            for (int i = 0; i < 36; i++) {
                double ang = i * DX_PI * 2.0 / 36.0
                    + DX_PI / 36.0 + w * DX_PI / 36.0;
                AddShot(pSet, pSet->x, pSet->y,
                    ang, 2.6 + w * 0.2,
                    img_enemyShotMediumBall[0]);      // 赤
            }
        }
        if (pSet->count == base + 30) {
            // 小弾（飛び散る残骸。角度・速度に揺らぎ）
            for (int i = 0; i < 72; i++) {
                double ang = i * DX_PI * 2.0 / 72.0
                    + (GetRand(6) - 3) * DX_PI / 180.0
                    + w * DX_PI / 72.0;
                AddShot(pSet, pSet->x, pSet->y,
                    ang, 4.0 + GetRand(80) / 100.0,
                    img_enemyShotSmallBall[6]);       // 白
            }
        }
    }
    // 移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  3) 残骸降着
//     中弾12個が左右の逆回転ペアで円盤状に周回。
//     外側から小弾がランダムな角度で中心へ降り注ぐ。
// ============================================================
static void ShotRemnant(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySE(sound_enemyShot_medium);
        // 中弾12個：半径180に配置し、接線方向へ
        for (int i = 0; i < 12; i++) {
            double ang = i * DX_PI * 2.0 / 12.0;
            double dir = (i % 2 == 0) ? 1.0 : -1.0;
            sEnemyShot* p = AddShot(pSet,
                pSet->x + 280.0 * cos(ang),
                pSet->y + 280.0 * sin(ang),
                ang + dir * DX_PI / 2.0,
                1.8,
                img_enemyShotMediumBall[8]);          // 橙
            p->param_d[0] = pSet->x;
            p->param_d[1] = pSet->y;
            p->param_i[0] = (int)dir;                 // 回転方向
        }
    }
    // 外側から降り注ぐ小弾
    if (pSet->count < 200 && pSet->count % 8 == 0) {
        double ang = GetRand(359) / 180.0 * DX_PI;
        double r = 330.0;
        double sx = pSet->x + r * cos(ang);
        double sy = pSet->y + r * sin(ang);
        AddShot(pSet, sx, sy,
            atan2(pSet->y - sy, pSet->x - sx),
            2.0 + GetRand(100) / 100.0,
            img_enemyShotSmallBall[0]);           // 赤
    }
    // 移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] != 0) {
            // 周回：常に中心からの接線方向へ向き直す
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            double dx = p->x - p->param_d[0];
            double dy = p->y - p->param_d[1];
            p->muki = atan2(dy, dx) + p->param_i[0] * DX_PI / 2.0;
        }
        else {
            // 直進（降り注ぐ残骸）
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  4) 中性子星／パルサー
//     24Fごとに30度ずつ回転する二極ビーム（銃弾）と
//     上下コーンの小弾。最後に全方位＋安全地帯4箇所。
// ============================================================
static void ShotPulsar(sEnemyShotSet* pSet)
{
    // 二極ビーム
    if (pSet->count < 120 && pSet->count % 24 == 0) {
        int step = pSet->count / 24;
        double baseAng = step * 30.0 / 180.0 * DX_PI;
        PlaySE(sound_enemyShot_light);
        for (int s = 0; s < 6; s++) {
            double dist = s * 20.0;
            double angU = baseAng + DX_PI / 2.0;
            double angD = baseAng - DX_PI / 2.0;
            AddShot(pSet,
                pSet->x + dist * cos(angU),
                pSet->y + dist * sin(angU),
                angU, 6.0, img_enemyShotBullet[3]);   // シアン
            AddShot(pSet,
                pSet->x + dist * cos(angD),
                pSet->y + dist * sin(angD),
                angD, 6.0, img_enemyShotBullet[3]);
        }
    }
    // 上下コーンの小弾
    if (pSet->count < 120 && pSet->count % 12 == 0) {
        int step = pSet->count / 24;
        double baseAng = step * 30.0 / 180.0 * DX_PI;
        for (int i = 0; i < 4; i++) {
            double off = (i - 1.5) * 0.15;
            AddShot(pSet, pSet->x, pSet->y,
                baseAng + DX_PI / 2.0 + off, 3.0,
                img_enemyShotSmallBall[3]);        // シアン
            AddShot(pSet, pSet->x, pSet->y,
                baseAng - DX_PI / 2.0 + off, 3.0,
                img_enemyShotSmallBall[3]);
        }
    }
    // フィナーレ：全方位小弾（4箇所の安全地帯）
    if (pSet->count == 144) {
        PlaySE(sound_enemyShot_extreme);
        for (int i = 0; i < 48; i++) {
            int deg = i * 360 / 48;
            if (deg % 90 < 15) continue;              // 安全地帯
            double ang = i * DX_PI * 2.0 / 48.0;
            AddShot(pSet, pSet->x, pSet->y, ang, 4.0,
                img_enemyShotSmallBall[6]);        // 白
        }
    }
    // 移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Supernova_DeepSeek()
{
    if (count == 1) {
        // 画面は480x480。中心で星核を表現
        enemy.maxHp = enemy.hp = 200;   // 200から変更しない
    }

    // 中央でごく僅かに脈動（星核の鼓動）
    enemy.x = 240.0;
    enemy.y = 200.0 + sin(count * 0.04) * 6.0;

    // 12秒(720F)周期でフェーズ進行
    int t = (count - 1) % 720;

    if (t == 0) {
        // 重力崩壊
        AddSet(ShotCollapse, enemy.x, enemy.y, 0);
    }
    if (t == 120) {
        // 閃光 → 衝撃波
        AddSet(ShotShockwave, enemy.x, enemy.y, 0);
    }
    if (t == 270) {
        // 残骸降着
        AddSet(ShotRemnant, enemy.x, enemy.y, 0);
    }
    if (t == 540) {
        // 中性子星／パルサー
        AddSet(ShotPulsar, enemy.x, enemy.y, 0);
    }
}