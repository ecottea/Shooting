#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：ステージ道中（ザコ隊列 × 大玉HP制）
//
//  ・ボスは enemy.y=-80.0 の画面外に固定。ダメージ源は「大玉ザコの撃破」のみ（1体撃破でボスに1ダメージ）
//  ・ザコ = 大玉(sEnemyShot)。param_i[15] を HP として使い、残HPに応じて色を変える
//  ・ザコは 1ウェーブ = 1つの sEnemyShotSet に隊列でまとめて入っている。
//    ザコが撃つ弾も同じ sEnemyShotSet に追加する（大玉以外の弾種を使用）
//  ・ウェーブは6種類をループし、6ウェーブごとに難易度レベル(lv)が上がる（最大 TMP_MAX_LEVEL）
//
//   0: 直下縦列      … 蛇行しながら降下、自機狙い扇弾(鱗弾×3層)
//   1: 横断縦列      … 画面横から波打って通過、左右に曲がり込む楕円弾
//   2: V字編隊       … 斜めに降下、ばら撒き小玉
//   3: U字旋回縦列   … 弧を描いて旋回、加速する二重リング(中玉)
//   4: 左右挟み撃ち  … 斜めに交差、減速→停止→再自機狙いの菱形弾
//   5: 横一列停止    … 画面上部で停止して射撃、回転螺旋(銃弾)＋自機狙い連射(小玉)
// ============================================================

// ---- 調整用定数 --------------------------------------------
static const int    TMP_WAVE_TYPES = 6;      // ウェーブの種類数
static const int    TMP_FIRST_WAVE = 90;     // 最初のウェーブが出るcount
static const int    TMP_WAVE_INTERVAL = 150;    // ウェーブ間隔(フレーム)
static const int    TMP_MAX_LEVEL = 3;      // 難易度レベルの上限
static const double TMP_GRUNT_MARGIN = 240.0;  // ザコ(大玉)の画面外消去マージン
static const double TMP_GRUNT_R = 20.0;   // 大玉の半径
static const double TMP_PSHOT_R = 10.0;   // 自機ショットの当たり判定半径（実際の見た目に合わせて調整）

// ---- パラメータの割り当て ----------------------------------
// 弾の役割 (sEnemyShot::param_i[14])
enum {
    TMP_ROLE_STRAIGHT = 0,  // 等速直進
    TMP_ROLE_GRUNT,         // ザコ(大玉)
    TMP_ROLE_ACCEL,         // 加速弾        param_d[0]=加速度 param_d[1]=最高速
    TMP_ROLE_REAIM,         // 減速→再自機狙い param_d[0]=減速度 param_d[1]=再発射速度 param_i[0]=再狙いフレーム param_i[1]=済フラグ
    TMP_ROLE_CURVE          // 曲がり弾      param_d[0]=旋回量/f param_i[0]=旋回終了フレーム
};
// ザコ(大玉)の param_i
enum {
    TMP_P_DELAY = 0,        // 出現待ちフレーム
    TMP_P_IDX = 1,        // 隊列内の番号
    TMP_P_ROLE = 14,       // 役割
    TMP_P_HP = 15        // HP
};
// ウェーブ(sEnemyShotSet)の param_i / param_d。 kind = ウェーブ種別
enum {
    TMP_W_N = 0,          // 隊列の人数
    TMP_W_DIR = 1,          // 左右反転用 (+1 / -1)
    TMP_W_LV = 2           // 難易度レベル
};                          // param_d[0]=基準x, param_d[1]=基準y

// ---- 共通ヘルパー ------------------------------------------
// 弾を1つ生成して sEnemyShotSet の末尾に繋ぐ
static sEnemyShot* TmpAddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[TMP_P_ROLE] = role;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

static bool TmpInScreen(double x, double y)
{
    return x >= 0.0 && x <= 480.0 && y >= 0.0 && y <= 480.0;
}

// 残HPに応じた大玉の画像（1:赤 2:橙 3:黄 4:緑 5:シアン 6:青 7:マゼンタ）
static int TmpGruntKind(int hp)
{
    static const int colorOfHp[8] = { 0, 0, 8, 1, 2, 3, 4, 5 };
    if (hp < 1) hp = 1;
    if (hp > 7) hp = 7;
    return img_enemyShotLargeBall[colorOfHp[hp]];
}

// ---- ザコの飛行経路 ----------------------------------------
// 出現してからの経過フレームだけで位置が決まる（待機中は画面外の出現位置で止まる）
static void TmpGruntPos(const sEnemyShotSet* pSet, const sEnemyShot* g, double* pX, double* pY)
{
    int t = g->count - g->param_i[TMP_P_DELAY];
    if (t < 0) t = 0;
    const double dt = (double)t;
    const int    i = g->param_i[TMP_P_IDX];
    const int    n = pSet->param_i[TMP_W_N];
    const int    dir = pSet->param_i[TMP_W_DIR];
    const double ax = pSet->param_d[0];
    const double ay = pSet->param_d[1];
    double x = 0.0, y = 0.0;

    switch (pSet->kind) {
    case 0: // 直下縦列：蛇行しながら降下
        x = ax + dir * 38.0 * sin(0.05 * dt);
        y = -30.0 + 2.0 * dt;
        break;
    case 1: // 横断縦列：横から波打って通過
        x = ((dir > 0) ? -30.0 : 510.0) + dir * 2.8 * dt;
        y = ay + 45.0 * sin(0.05 * dt);
        break;
    case 2: { // V字編隊：先頭が最も低い
        const double k = (double)(i - (n - 1) / 2);
        x = ax + k * 46.0 + dir * 0.35 * dt;
        y = -30.0 - fabs(k) * 40.0 + 1.5 * dt;
        break;
    }
    case 3: { // U字旋回縦列：等速で旋回する円弧
        const double v = 3.1, w = 0.017, r = v / w;
        x = ((dir > 0) ? -30.0 : 510.0) + dir * r * sin(w * dt);
        y = ay + r * (1.0 - cos(w * dt));
        break;
    }
    case 4: { // 左右挟み撃ち：偶数番と奇数番が逆の端から斜めに交差
        const int s = ((i % 2 == 0) ? 1 : -1) * dir;
        x = ((s > 0) ? -30.0 : 510.0) + s * 2.7 * dt;
        y = ay + 1.6 * dt;
        break;
    }
    default: { // 横一列停止：降下(60f) → 停止(150f) → 上へ退場
        const double hy = 90.0 + (((i + ((dir > 0) ? 0 : 1)) % 2) * 45.0);
        x = 60.0 + i * 60.0;
        if (dt < 60.0) {
            const double r = dt / 60.0;
            const double e = 1.0 - (1.0 - r) * (1.0 - r);
            y = -30.0 + (hy + 30.0) * e;
        }
        else if (dt < 210.0) {
            y = hy;
        }
        else {
            y = hy - 2.4 * (dt - 210.0);
        }
        break;
    }
    }
    *pX = x;
    *pY = y;
}

// ---- ザコの射撃 --------------------------------------------
// 戻り値: 鳴らす効果音 (0:なし 1:light 2:medium)
static int TmpGruntFire(sEnemyShotSet* pSet, sEnemyShot* g)
{
    const int t = g->count - g->param_i[TMP_P_DELAY];
    if (t < 0) return 0;
    if (!TmpInScreen(g->x, g->y)) return 0;   // 画面外では撃たない

    const int    lv = pSet->param_i[TMP_W_LV];
    const int    i = g->param_i[TMP_P_IDX];
    const double aim = atan2(player.y - g->y, player.x - g->x);
    int se = 0;

    switch (pSet->kind) {
    case 0: { // 自機狙い扇弾（鱗弾×3層、速度違い）
        const int interval = 84 - 10 * lv;
        if (t % interval != 30 || g->y < 20.0) break;
        const int way = 5 + lv;
        static const int layerColor[3] = { 0, 8, 1 };
        for (int layer = 0; layer < 3; layer++) {
            for (int j = 0; j < way; j++) {
                const double a = aim + (j - (way - 1) / 2.0) * 0.13;
                TmpAddShot(pSet, g->x, g->y, a, 2.3 + 0.6 * layer, img_enemyShotScale[layerColor[layer]], TMP_ROLE_STRAIGHT);
            }
        }
        se = 1;
        break;
    }
    case 1: { // 左右から自機の前方で交差する曲がり弾（中楕円弾）
        const int interval = 54 - 6 * lv;
        if (t % interval != 14) break;
        const int m = 3 + lv;
        for (int side = -1; side <= 1; side += 2) {
            for (int j = 0; j < m; j++) {
                sEnemyShot* p = TmpAddShot(pSet, g->x, g->y, aim + side * 0.55, 2.6 + 0.3 * j,
                    img_enemyShotMediumOval[(side < 0) ? 2 : 8], TMP_ROLE_CURVE);
                p->param_d[0] = -side * 0.016;   // 内側へ曲がる
                p->param_i[0] = 70;              // 70フレームで旋回終了
            }
        }
        se = 1;
        break;
    }
    case 2: { // ばら撒き小玉（自機方向±30度）
        const int interval = 14 - 2 * lv;
        if ((t + i * 3) % interval != 0 || g->y < 10.0) break;
        for (int j = 0; j < 3; j++) {
            const double a = aim + (GetRand(60) - 30) / 180.0 * DX_PI;
            const double sp = 2.0 + GetRand(160) / 100.0;
            TmpAddShot(pSet, g->x, g->y, a, sp, img_enemyShotSmallBall[3 + (i % 3)], TMP_ROLE_STRAIGHT);
        }
        if (i == (pSet->param_i[TMP_W_N] - 1) / 2) se = 1;   // 効果音は中央の1体だけ
        break;
    }
    case 3: { // 加速する二重リング（中玉）。自機方向は隙間になるリングと、真っ直ぐ向かう弾を含むリング
        const int interval = 60 - 4 * lv;
        if (t % interval != 25) break;
        const int    N = 12 + 2 * lv;
        const double step = 2.0 * DX_PI / N;
        for (int ring = 0; ring < 2; ring++) {
            const double base = (ring == 0) ? (aim + step * 0.5) : aim;
            for (int j = 0; j < N; j++) {
                sEnemyShot* p = TmpAddShot(pSet, g->x, g->y, base + step * j, (ring == 0) ? 1.0 : 0.6,
                    img_enemyShotMediumBall[(ring == 0) ? 3 : 4], TMP_ROLE_ACCEL);
                p->param_d[0] = 0.04;
                p->param_d[1] = (ring == 0) ? 3.0 : 2.4;
            }
        }
        se = 2;
        break;
    }
    case 4: { // 扇状に広げて減速・停止 → 端から順に自機狙いで再発射（菱形弾）
        const int interval = 80 - 8 * lv;
        if (t % interval != 20) break;
        const int way = 7 + 2 * lv;
        for (int j = 0; j < way; j++) {
            const double a = aim + (j - (way - 1) / 2.0) * (1.7 / (way - 1));
            sEnemyShot* p = TmpAddShot(pSet, g->x, g->y, a, 3.8,
                img_enemyShotDiamond[(j % 2 == 0) ? 1 : 8], TMP_ROLE_REAIM);
            p->param_d[0] = 3.8 / 38.0;
            p->param_d[1] = 3.0 + 0.1 * lv;
            p->param_i[0] = 40 + j * 3;
        }
        se = 2;
        break;
    }
    default: { // 停止中のみ射撃。偶数番＝回転螺旋(銃弾)、奇数番＝自機狙い連射(小玉)
        if (t < 60 || t >= 210) break;
        if (i % 2 == 0) {
            if (t % 3 != 0) break;
            const int    arms = 2 + lv / 2;
            const double sgn = (i % 4 == 0) ? 1.0 : -1.0;
            const double base = 0.15 * (t - 60) * sgn + i * 0.5;
            for (int a = 0; a < arms; a++) {
                const double ang = base + a * 2.0 * DX_PI / arms;
                TmpAddShot(pSet, g->x, g->y, ang, 2.6, img_enemyShotBullet[(i % 4 == 0) ? 5 : 6], TMP_ROLE_STRAIGHT);
            }
            if (i == 0 && t % 12 == 0) se = 1;
        }
        else {
            if (t % 12 != 6) break;
            const int way = (lv >= 2) ? 3 : 1;
            for (int j = 0; j < way; j++) {
                const double a = aim + (j - (way - 1) / 2.0) * 0.2;
                TmpAddShot(pSet, g->x, g->y, a, 3.4, img_enemyShotSmallBall[6], TMP_ROLE_STRAIGHT);
            }
        }
        break;
    }
    }
    return se;
}

// ---- ザコ1体の更新（移動・被弾・射撃）。撃破されたら true --------
static bool TmpUpdateGrunt(sEnemyShotSet* pSet, sEnemyShot* g, int* pSe)
{
    // 移動（経路から位置を求め、進行方向を muki に反映）
    double nx, ny;
    TmpGruntPos(pSet, g, &nx, &ny);
    const double dx = nx - g->x, dy = ny - g->y;
    g->speed = sqrt(dx * dx + dy * dy);
    if (g->speed > 0.0) g->muki = atan2(dy, dx);
    g->x = nx;
    g->y = ny;

    // 自機ショットとの当たり判定（画面内にいる間だけ有効）
    if (TmpInScreen(g->x, g->y)) {
        const double hitR = TMP_GRUNT_R + TMP_PSHOT_R;
        sPlayerShot* ps = playerShotHead.next;
        while (ps != &playerShotHead) {
            sPlayerShot* psNext = ps->next;
            const double ddx = ps->x - g->x, ddy = ps->y - g->y;
            if (ddx * ddx + ddy * ddy <= hitR * hitR) {
                if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
                PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);
                
                // 命中した自機ショットは消す
                ps->prev->next = ps->next;
                ps->next->prev = ps->prev;
                delete ps;

                if (--g->param_i[TMP_P_HP] <= 0) {
                    // 撃破：ボスに1ダメージ
                    if (enemy.hp > 0) enemy.hp--;
                    return true;
                }
                g->kind = TmpGruntKind(g->param_i[TMP_P_HP]);
            }
            ps = psNext;
        }
    }

    // 射撃
    const int se = TmpGruntFire(pSet, g);
    if (se > *pSe) *pSe = se;
    return false;
}

// ---- ウェーブ本体（ザコ＋ザコの弾を管理する弾幕関数） -----------
static void TmpShotGruntWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShotSet* pSet = pEnemyShotSet;

    // 出現：隊列を生成（位置は経路の t=0 地点＝画面外）
    if (pSet->count == 0) {
        const int n = pSet->param_i[TMP_W_N];
        const int hp = 3 + pSet->param_i[TMP_W_LV]-2;
        for (int i = 0; i < n; i++) {
            int delay = 0;
            switch (pSet->kind) {
            case 0: delay = i * 12;       break;
            case 1: delay = i * 10;       break;
            case 2: delay = 0;            break;
            case 3: delay = i * 9;        break;
            case 4: delay = (i / 2) * 16; break;
            default: delay = i * 4;       break;
            }
            sEnemyShot* g = TmpAddShot(pSet, 0.0, 0.0, DX_PI * 0.5, 0.0, TmpGruntKind(hp), TMP_ROLE_GRUNT);
            g->margin = TMP_GRUNT_MARGIN;
            g->param_i[TMP_P_DELAY] = delay;
            g->param_i[TMP_P_IDX] = i;
            g->param_i[TMP_P_HP] = hp;
            TmpGruntPos(pSet, g, &g->x, &g->y);
        }
    }

    int se = 0;
    sEnemyShot* pHead = pSet->pEnemyShotHead;
    sEnemyShot* pShot = pHead->next;
    while (pShot != pHead) {
        sEnemyShot* pNext = pShot->next;   // 撃破による削除に備えて先に保存

        const int role = pShot->param_i[TMP_P_ROLE];
        if (role == TMP_ROLE_GRUNT) {
            if (TmpUpdateGrunt(pSet, pShot, &se)) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            switch (role) {
            case TMP_ROLE_ACCEL:
                pShot->speed += pShot->param_d[0];
                if (pShot->speed > pShot->param_d[1]) pShot->speed = pShot->param_d[1];
                break;
            case TMP_ROLE_REAIM:
                if (pShot->count < pShot->param_i[0]) {
                    pShot->speed -= pShot->param_d[0];
                    if (pShot->speed < 0.0) pShot->speed = 0.0;
                }
                else if (pShot->param_i[1] == 0) {
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = pShot->param_d[1];
                    pShot->param_i[1] = 1;
                }
                break;
            case TMP_ROLE_CURVE:
                if (pShot->count < pShot->param_i[0]) pShot->muki += pShot->param_d[0];
                break;
            default:
                break;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pNext;
    }

    // 効果音（1フレームに1回だけ）
    if (se > 0) {
        const int handle = (se >= 2) ? sound_enemyShot_medium : sound_enemyShot_light;
        if (CheckSoundMem(handle)) StopSoundMem(handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK);
    }
}

// ---- ウェーブ生成 ------------------------------------------
static void TmpSpawnWave(int waveNo)
{
    static const int waveSize[TMP_WAVE_TYPES] = { 8, 8, 7, 7, 8, 7 };
    const int type = waveNo % TMP_WAVE_TYPES;
    int lv = waveNo / TMP_WAVE_TYPES;
    if (lv > TMP_MAX_LEVEL) lv = TMP_MAX_LEVEL;

    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = TmpShotGruntWave;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = DX_PI * 0.5;
    pEnemyShotSet->kind = type;
    pEnemyShotSet->param_i[TMP_W_N] = waveSize[type];
    pEnemyShotSet->param_i[TMP_W_DIR] = (GetRand(1) == 0) ? -1 : 1;
    pEnemyShotSet->param_i[TMP_W_LV] = lv;
    switch (type) {
    case 0: pEnemyShotSet->param_d[0] = 90.0 + GetRand(300);  break;  // 縦列のx
    case 1: pEnemyShotSet->param_d[1] = 70.0 + GetRand(100);  break;  // 横断の高さ
    case 2: pEnemyShotSet->param_d[0] = 150.0 + GetRand(180); break;  // V字の中心x
    case 3: pEnemyShotSet->param_d[1] = 30.0 + GetRand(60);   break;  // 旋回の開始高さ
    case 4: pEnemyShotSet->param_d[1] = 20.0 + GetRand(40);   break;  // 挟み撃ちの開始高さ
    default:
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);            // 停止射撃ウェーブの予告音
        break;
    }
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ---- 敵本体のパターン --------------------------------------
void EnemyPat_OnTheWay_Claude()
{
    static int waveNo;
    if (count == 1) {
        // ゲーム画面は 480x480。ボスは自機ショットの届かない画面外に置く
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 70; // 200から変更しないこと
        waveNo = 0;
    }
    // ボスが生きている間、一定間隔でザコの隊列を送り込む
    if (enemy.hp > 0 && count >= TMP_FIRST_WAVE && (count - TMP_FIRST_WAVE) % TMP_WAVE_INTERVAL == 0) {
        TmpSpawnWave(waveNo++);
    }
}