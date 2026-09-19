// 禁忌「レーヴァテイン」
// 赤色の短レーザーを並べて長いレーザーとして扱い、
// 速めの平行移動 または 遅めの平行移動＋速めの回転 を交互に行う。
// 移動中、レーザーを等分する点から赤菱形弾を垂直方向（移動方向との内積が正の側）に射出。
// 菱形弾は初速0から加速し、終端速度到達後は等速直線運動。
// ボス位置は常にレーザーの一端に一致させる。

#include "gv.h"  // 必要に応じて既存ヘッダをインクルード

// レーザー1本を構成する短レーザーの本数
static const int LASER_SEGMENTS = 12;
// 短レーザー1本の長さ（ピクセル相当）
static const double LASER_SEG_LEN = 64.0;
// 全体の長さ
static const double LASER_TOTAL_LEN = LASER_SEG_LEN * LASER_SEGMENTS;

// 菱形弾のパラメータ
static const double DIAMOND_ACCEL = 0.12;      // 加速度
static const double DIAMOND_TERMINAL = 4.5;    // 終端速度
static const int    DIAMOND_FIRE_INTERVAL = 3; // 射出間隔（フレーム）

// 1周期の長さ（フレーム）
static const int CYCLE_FRAMES = 180;

// ------------------------------------------------------------
// レーザー本体の弾幕パターン
// ------------------------------------------------------------
static void ShotLaevateinn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ：短レーザーを並べて長いレーザーを生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_d[0] : レーザーの向き（ラジアン）
        // param_d[1] : 現在の平行移動速度
        // param_d[2] : 現在の回転角速度
        // param_i[0] : 周期番号（動きの種類切り替え用）
        // param_i[1] : 射出カウンタ

        double baseAngle = pEnemyShotSet->muki;
        pEnemyShotSet->param_d[0] = baseAngle;
        pEnemyShotSet->param_d[1] = 0.0;
        pEnemyShotSet->param_d[2] = 0.0;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;

        // 短レーザーを等間隔に配置
        for (int i = 0; i < LASER_SEGMENTS; i++) {
            pEnemyShot = new sEnemyShot;
            double t = (i + 0.5) / (double)LASER_SEGMENTS; // 0〜1の中心位置
            pEnemyShot->x = pEnemyShotSet->x + t * LASER_TOTAL_LEN * cos(baseAngle);
            pEnemyShot->y = pEnemyShotSet->y + t * LASER_TOTAL_LEN * sin(baseAngle);
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤色短レーザー
            pEnemyShot->count = 0;
            pEnemyShot->margin = 999;

            // 自身がレーザーの何番目か
            pEnemyShot->param_i[0] = i;
            // 所属するShotSetのアドレスを一応保持（必要なら）
            pEnemyShot->param_d[0] = 0.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 周期ごとの動き決定 ----
    int cycle = pEnemyShotSet->count / CYCLE_FRAMES;
    int localFrame = pEnemyShotSet->count % CYCLE_FRAMES;

    // 周期が変わったら動きの種類を切り替える
    if (localFrame == 0 && pEnemyShotSet->count > 0) {
        pEnemyShotSet->param_i[0] = cycle;
        // 効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 動きの種類（周期番号 % 4）
    // 0,2 : 速めの平行移動
    // 1,3 : 遅めの平行移動 + 速めの回転
    int moveType = pEnemyShotSet->param_i[0] % 4;

    double& laserAngle = pEnemyShotSet->param_d[0];
    double  parallelSpeed = 0.0;
    double  rotSpeed = 0.0;

    if (moveType == 0 || moveType == 2) {
        // 速めの平行移動のみ
        parallelSpeed = 3.8;
        rotSpeed = 0.0;
        // 平行移動方向はレーザーの向きに垂直（左右交互）
        if (moveType == 2) parallelSpeed = -parallelSpeed;
    }
    else {
        // 遅めの平行移動 + 速めの回転
        parallelSpeed = 1.2 * ((moveType == 1) ? 1.0 : -1.0);
        rotSpeed = 0.045 * ((moveType == 1) ? 1.0 : -1.0); // 約2.5度/frame
    }

    // レーザーの一端（ボス側）の位置を更新
    // 平行移動はレーザーの向きに垂直な方向
    double perpX = -sin(laserAngle);
    double perpY = cos(laserAngle);

    pEnemyShotSet->x += parallelSpeed * perpX;
    pEnemyShotSet->y += parallelSpeed * perpY;

    // 回転
    laserAngle += rotSpeed;

    // 画面端で跳ね返らないようにクランプ（簡易）
    if (pEnemyShotSet->x < 20.0)  pEnemyShotSet->x = 20.0;
    if (pEnemyShotSet->x > 460.0) pEnemyShotSet->x = 460.0;
    if (pEnemyShotSet->y < 20.0)  pEnemyShotSet->y = 20.0;
    if (pEnemyShotSet->y > 200.0) pEnemyShotSet->y = 200.0; // 上寄りに制限

    // ---- 短レーザーの位置・向きを更新 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotLaser[0]) {
            int idx = pShot->param_i[0];
            double t = (idx + 0.5) / (double)LASER_SEGMENTS;
            pShot->x = pEnemyShotSet->x + t * LASER_TOTAL_LEN * cos(laserAngle);
            pShot->y = pEnemyShotSet->y + t * LASER_TOTAL_LEN * sin(laserAngle);
            pShot->muki = laserAngle;
        }
        pShot = pShot->next;
    }

    // ---- 菱形弾の射出 ----
    // 移動している間だけ射出
    bool isMoving = (fabs(parallelSpeed) > 0.01 || fabs(rotSpeed) > 0.001);
    if (isMoving && (pEnemyShotSet->count % DIAMOND_FIRE_INTERVAL == 0)) {
        // レーザーを等分する点から射出
        for (int i = 1; i < LASER_SEGMENTS; i++) { // 端は除外して密度調整
            double t = (double)i / (double)LASER_SEGMENTS;
            double px = pEnemyShotSet->x + t * LASER_TOTAL_LEN * cos(laserAngle);
            double py = pEnemyShotSet->y + t * LASER_TOTAL_LEN * sin(laserAngle);

            // レーザーの向きに垂直な2方向
            double vx1 = -sin(laserAngle);
            double vy1 = cos(laserAngle);
            double vx2 = sin(laserAngle);
            double vy2 = -cos(laserAngle);

            // レーザーの移動方向ベクトル（平行移動成分）
            double moveDirX = parallelSpeed * perpX;
            double moveDirY = parallelSpeed * perpY;
            // 回転による接線方向も考慮（簡易）
            // 回転中心は一端なので、点の位置での接線速度を追加
            double radius = t * LASER_TOTAL_LEN;
            double tangX = -radius * sin(laserAngle) * rotSpeed; // 近似
            double tangY = radius * cos(laserAngle) * rotSpeed;
            moveDirX += tangX;
            moveDirY += tangY;

            // 内積が正の方向を選ぶ
            double dot1 = vx1 * moveDirX + vy1 * moveDirY;
            double outX, outY;
            if (dot1 > 0.0) {
                outX = vx1; outY = vy1;
            }
            else {
                outX = vx2; outY = vy2;
            }

            // 菱形弾生成
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = px;
            pEnemyShot->y = py;
            pEnemyShot->muki = atan2(outY, outX);
            pEnemyShot->speed = 0.0; // 初速0
            pEnemyShot->kind = img_enemyShotDiamond[0]; // 赤菱形弾
            pEnemyShot->count = 0;
            // 加速用パラメータ
            pEnemyShot->param_d[0] = DIAMOND_ACCEL;     // 加速度
            pEnemyShot->param_d[1] = DIAMOND_TERMINAL;  // 終端速度
            pEnemyShot->param_i[0] = 1;                 // 加速中フラグ
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        if (pEnemyShotSet->count % (DIAMOND_FIRE_INTERVAL * 3) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // ---- 全弾の移動処理 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotDiamond[0]) {
            // 菱形弾：加速 → 等速
            if (pShot->param_i[0] == 1) {
                pShot->speed += pShot->param_d[0];
                if (pShot->speed >= pShot->param_d[1]) {
                    pShot->speed = pShot->param_d[1];
                    pShot->param_i[0] = 0; // 加速終了
                }
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // 短レーザーは上で位置を直接更新しているのでここでは何もしない
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Levatain_Grok()
{
    static int phase = 0;          // 0: 待機, 1: レーザー展開中
    static int shotSetCreated = 0;

    if (count == 1) {
        // 初期位置（画面上部中央寄り）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        shotSetCreated = 0;
    }

    // レーザー用 ShotSet を1つだけ生成し、以降はそれを使い続ける
    if (count == 30 && shotSetCreated == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLaevateinn;

        // 初期レーザー向き：右下方向
        pEnemyShotSet->muki = DX_PI * 0.25; // 45度
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shotSetCreated = 1;
        phase = 1;
    }

    // ボス位置をレーザーの一端に常に一致させる
    // （ShotLaevateinn 内で pEnemyShotSet->x/y を更新しているので、
    //   毎フレームその値を敵位置にコピーする）
    if (phase == 1) {
        // 最新の ShotSet を探す（このパターンでは1つだけ）
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotLaevateinn) {
                enemy.x = pSet->x;
                enemy.y = pSet->y;
                break;
            }
            pSet = pSet->next;
        }
    }
}