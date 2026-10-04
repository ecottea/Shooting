#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  四天王奥義「三歩必殺」 (星熊勇仪 / 東方地霊殿 3面3符)
// ------------------------------------------------------------
//  原作仕様の再現:
//   ・勇儀が画面中央で3回足を踏み鳴らすたびに、
//     「彼女が一歩で行ける範囲」に弾幕が高密度で埋め尽くされる。
//   ・弾は3層(内/中/外のリング)に分けて出現し、出現後しばらく静止
//     する(罠の設置フェーズ)。
//   ・その後「内側+外側 → 中間」の順で一斉に飛散する。
//   ・奇数波: 青(内/右カーブ)・シアン(中/左カーブ)・白小玉(外/2 groups spin)
//     偶数波: 赤(内/左カーブ)・マゼンタ(中/右カーブ)・紫の泡弾(外/全て右回転)
//   ・外側の弾は自機位置と重なる場所には出現しない (Easy/Normal仕様)。
//   ・Hard/Lunaticにする場合は線をランダムな塊(clump)に変え、
//     自機位置の判定を外せばよい(コメント参照)。
//
//  注意:
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//     画面外の弾の消去はメインルーチンで行われる。
//   ・GetRand(x) は 0〜x の x+1 種類を返すので、±Nの揺らぎは
//     GetRand(2*N) - N と書くこと。
// ============================================================

// ---- 調整用定数 ----
#define SANPO_WAVE_PERIOD   220    // 波と波の間隔(フレーム)
#define SANPO_STOMP1        0      // 一歩目(内側リング出現)
#define SANPO_STOMP2        40     // 二歩目(中間リング出現)
#define SANPO_STOMP3        80     // 三歩目(外側リング出現・最密)
#define SANPO_RING0_R       70.0   // 内側リングの半径
#define SANPO_RING1_R       150.0  // 中間リングの半径
#define SANPO_RING2_R       290.0  // 外側リングの半径(勇儀の三歩は広い)
#define SANPO_RING2_BAND    45.0   // 外側リングの帯の間隔
#define SANPO_MOVE_COMMON   140    // 内側+外側が動き出すSetカウント
#define SANPO_MOVE_LATE     175    // 中間が動き出すSetカウント(原作同様ひと拍遅い)
#define SANPO_PELLET_SPEED  3.5    // 外側弾の飛散速度
#define SANPO_RING0_SPEED   2.5    // 内側弾の飛散速度
#define SANPO_RING1_SPEED   2.0    // 中間弾の飛散速度

// ---- 弾を追加するヘルパー ----
static sEnemyShot* SanpoAddShot(sEnemyShotSet* pSet, double x, double y,
    double speed, int img)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = 0.0;
    p->speed = speed;
    p->kind = img;
    // リング状リストの末尾に接続
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ---- リング上に「短い線状の弾の列」を1本出現させる ----
// baseAngle : 列の中心角度  radius : リング半径
// lineLen   : 列の長さ(弾数)  arcStep : 列内の弾の角間隔(カーブした弧になる)
// spin      : 飛散時の回転(カーブ)の強さと向き
static void SanpoSpawnLine(sEnemyShotSet* pSet, double baseAngle,
    double radius, int lineLen, double arcStep,
    double speed, double spin, int img,
    int moveAtSetCount)
{
    // 弾自身のcountは出現時0から始まるため、出現タイミング分を差し引く
    int bulletMoveStart = moveAtSetCount - pSet->count;

    for (int k = 0; k < lineLen; k++) {
        double a = baseAngle + (k - (lineLen - 1) / 2.0) * arcStep;
        sEnemyShot* p = SanpoAddShot(pSet,
            pSet->x + radius * cos(a),
            pSet->y + radius * sin(a),
            speed, img);
        p->param_d[0] = a;                     // 飛び出す基準方向(中心から放射状)
        p->param_d[1] = spin;                  // カーブの向き・強さ
        p->param_d[2] = (double)bulletMoveStart; // 静止している時間
        p->param_i[0] = 0;                     // 飛散開始フラグ
    }
}

// ---- 外側リング: 弾数最多・最密の帯を敷き詰める ----
static void SanpoSpawnPellets(sEnemyShotSet* pSet, int parity,
    int moveAtSetCount)
{
    int bulletMoveStart = moveAtSetCount - pSet->count;
    int img;
    if (parity == 0) {
        img = img_enemyShotSmallBall[6];   // 奇数波: 白い小弾(ペレット)
    }
    else {
        img = img_enemyShotMediumOval[5];  // 偶数波: 紫の泡弾(中楕円弾)
    }

    // 帯状に3周分敷き詰める(内側ほど周長が短いので弾数も減らす)
    for (int ring = 0; ring < 3; ring++) {
        double r = SANPO_RING2_R - ring * SANPO_RING2_BAND;
        int n = 36 + ring * 6;
        for (int i = 0; i < n; i++) {
            double a = i * (2.0 * DX_PI / n) + (GetRand(40) - 20) / 100.0;
            double rr = r + GetRand(100) / 10.0;   // 半径方向のジッタ 0.0〜10.0
            double x = pSet->x + rr * cos(a);
            double y = pSet->y + rr * sin(a);

            // Easy/Normal仕様: 自機と重なる位置には出現しない
            // (Hard/Lunaticではこの判定を消す = 「容赦なく出現させる」)
            double dx = x - player.x;
            double dy = y - player.y;
            if (dx * dx + dy * dy < 40.0 * 40.0) continue;

            sEnemyShot* p = SanpoAddShot(pSet, x, y, SANPO_PELLET_SPEED, img);
            p->param_d[0] = a;
            if (parity == 0) {
                // ランダムに2グループに分かれ、左右どちらかに回転しながら飛ぶ
                p->param_d[1] = (GetRand(1) == 0) ? 0.02 : -0.02;
            }
            else {
                // 偶数波の泡はすべて右へ回転しながら飛ぶ
                p->param_d[1] = 0.02;
            }
            p->param_d[2] = (double)bulletMoveStart;
            p->param_i[0] = 0;
        }
    }
}

// ---- 弾幕本体: 3歩 → 設置 → 一斉飛散 ----
static void ShotSanpoHissatsu(sEnemyShotSet* pSet)
{
    int parity = pSet->kind;   // 0:奇数波 / 1:偶数波

    // ===== 一歩目: 内側リング(短い線状弾) =====
    if (pSet->count == SANPO_STOMP1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);   // 予告音

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int img = (parity == 0) ? img_enemyShotSmallBall[4]   // 青
            : img_enemyShotSmallBall[0];  // 赤
        double spin = (parity == 0) ? 0.012 : -0.012;  // 青=右カーブ / 赤=左カーブ
        for (int j = 0; j < 16; j++) {
            double base = j * (2.0 * DX_PI / 16.0);
            SanpoSpawnLine(pSet, base, SANPO_RING0_R, 3, 0.10,
                SANPO_RING0_SPEED, spin, img, SANPO_MOVE_COMMON);
        }
    }
    // ===== 二歩目: 中間リング(長めの線状弾) =====
    else if (pSet->count == SANPO_STOMP2) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int img = (parity == 0) ? img_enemyShotMediumBall[3]   // シアン
            : img_enemyShotMediumBall[5];  // マゼンタ
        double spin = (parity == 0) ? -0.015 : 0.015;  // シアン=左 / マゼンタ=右
        for (int j = 0; j < 16; j++) {
            double base = j * (2.0 * DX_PI / 16.0) + (DX_PI / 16.0); // 内側と半ピッチずらす
            SanpoSpawnLine(pSet, base, SANPO_RING1_R, 4, 0.12,
                SANPO_RING1_SPEED, spin, img, SANPO_MOVE_LATE);
        }
    }
    // ===== 三歩目: 外側リング(最も多く&密) =====
    else if (pSet->count == SANPO_STOMP3) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        SanpoSpawnPellets(pSet, parity, SANPO_MOVE_COMMON);
    }
    // ===== 一斉飛散の瞬間 =====
    else if (pSet->count == SANPO_MOVE_COMMON) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ===== 弾の更新(静止 → 放射状に飛散 + カーブ) =====
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // まだ飛散していない弾は、指定カウントまで原地に留まる(罠フェーズ)
        if (pShot->param_i[0] == 0 && pShot->count >= (int)pShot->param_d[2]) {
            pShot->param_i[0] = 1;
            pShot->muki = pShot->param_d[0];   // 勇儀の中心から放射状に飛び出す
        }
        if (pShot->param_i[0] != 0) {
            pShot->muki += pShot->param_d[1];  // カーブ(回転)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン: 星熊勇儀
//  原作通り画面中央に留まり、3歩のリズムで弾幕の波を繰り出す
// ============================================================
void EnemyPat_SanpoHissatsu_Zai()
{
    static int wave;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        wave = 0;
    }

    // 中央をキープしつつ、踏み鳴らしの重みを体の揺れで表現
    enemy.y = 60.0 + sin(count * 0.05) * 3.0;

    // 30フレーム後に最初の波、以後300フレームごとに次の波
    if (count >= 30 && (count - 30) % SANPO_WAVE_PERIOD == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanpoHissatsu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = wave++ % 2;   // 0:奇数波 / 1:偶数波(弾色とカーブが反転)
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}