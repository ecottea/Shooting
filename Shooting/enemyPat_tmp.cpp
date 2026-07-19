#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// テトリス盤面管理用 (10列 x 16行)
// 1マスのサイズを25pxとし、X: 115～365, Y: 60～460 を盤面とする
// ---------------------------------------------------------
static int t_field[16][10];
static sEnemyShot* t_field_shots[16][10];
static sEnemyShotSet* active_mino = nullptr;

// テトリミノの形状データ (4つのブロックの相対X, Y)
static const int mino_shape[7][4][2] = {
    {{0,-1}, {0,0}, {0,1}, {0,2}},  // 0: I (シアン)
    {{0,0}, {1,0}, {0,1}, {1,1}},   // 1: O (黄)
    {{0,0}, {-1,0}, {1,0}, {0,1}},  // 2: T (マゼンタ)
    {{0,0}, {-1,0}, {1,0}, {1,1}},  // 3: J (青)
    {{0,0}, {-1,0}, {1,0}, {-1,1}}, // 4: L (橙)
    {{0,0}, {1,0}, {0,1}, {-1,1}},  // 5: S (緑)
    {{0,0}, {-1,0}, {0,1}, {1,1}}   // 6: Z (赤)
};

// 弾の色インデックス: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
static const int mino_color[7] = { 3, 1, 5, 4, 8, 2, 0 };

// 回転後のブロックの相対座標を取得する関数
static void get_block_pos(int type, int rot, int b_idx, int* bx, int* by) {
    int x = mino_shape[type][b_idx][0];
    int y = mino_shape[type][b_idx][1];
    switch (rot % 4) {
    case 0: *bx = x; *by = y; break;
    case 1: *bx = -y; *by = x; break;
    case 2: *bx = -x; *by = -y; break;
    case 3: *bx = y; *by = -x; break;
    }
}

// 衝突判定 (true なら衝突)
static bool check_collision(int cx, int cy, int type, int rot) {
    for (int i = 0; i < 4; i++) {
        int bx, by;
        get_block_pos(type, rot, i, &bx, &by);
        int nx = cx + bx;
        int ny = cy + by;

        if (nx < 0 || nx >= 10) return true; // 左右の壁
        if (ny >= 16) return true; // 床
        if (ny >= 0 && t_field[ny][nx] != 0) return true; // 既存ブロック
    }
    return false;
}

// ---------------------------------------------------------
// ミノ単体の挙動
// ---------------------------------------------------------
static void ShotTetrimino(sEnemyShotSet* pSet) {
    if (pSet->param_i[3] == 1) return; // 固定済みなら処理終了

    int type = pSet->param_i[0];
    int& cx = pSet->param_i[1];
    double& cy = pSet->param_d[0];
    int& rot = pSet->param_i[2];

    // 初回のみ弾(ブロック)を4つ生成
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 4; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            // ブロックとして20x20の大玉を採用し、テトリスカラーに合わせる
            pShot->kind = img_enemyShotLargeBall[mino_color[type]];
            pShot->param_i[0] = i; // 自身が4つのうちどのブロックかを記憶

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 自機への誘導 (30フレーム毎に1マス、自機のX座標へ寄る)
    if (pSet->count % 30 == 0) {
        int target_cx = (int)((player.x - 115.0) / 25.0);
        if (target_cx < 0) target_cx = 0;
        if (target_cx > 9) target_cx = 9;

        if (cx < target_cx) {
            if (!check_collision(cx + 1, (int)cy, type, rot)) cx++;
        }
        else if (cx > target_cx) {
            if (!check_collision(cx - 1, (int)cy, type, rot)) cx--;
        }
    }

    // 自動回転 (90フレーム毎に90度回転)
    if (pSet->count > 0 && pSet->count % 90 == 0) {
        if (!check_collision(cx, (int)cy, type, rot + 1)) {
            rot = (rot + 1) % 4;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // 落下処理 (ボスの残りHPに応じて落下速度が上昇)
    double speed = 0.05 + (double)(enemy.maxHp - enemy.hp) * 0.00003;
    if (speed > 0.3) speed = 0.3;
    cy += speed;

    // 接地判定 (次の位置にブロックや床があるか)
    if (check_collision(cx, (int)cy + 1, type, rot)) {
        int icy = (int)cy;
        // めり込み補正
        while (icy >= 0 && check_collision(cx, icy, type, rot)) icy--;

        // 固定処理
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            int bx, by;
            get_block_pos(type, rot, pShot->param_i[0], &bx, &by);
            int nx = cx + bx;
            int ny = icy + by;

            if (nx >= 0 && nx < 10 && ny >= 0 && ny < 16) {
                t_field[ny][nx] = type + 1;
                t_field_shots[ny][nx] = pShot; // ポインタを記憶しておく
                // 座標をグリッドの中央に完全に合わせる
                pShot->x = 115.0 + nx * 25.0 + 12.5;
                pShot->y = 60.0 + ny * 25.0 + 12.5;
            }
            else {
                // 盤面外にはみ出たブロックは消去 (Y=1000に飛ばして自動削除させる)
                pShot->y = 1000.0;
            }
            pShot = pShot->next;
        }

        pSet->param_i[3] = 1; // 自身を固定済み状態へ移行
        active_mino = nullptr; // 次のミノ生成を許可
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        return;
    }

    // 落下中：各ブロック(弾)の座標更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int bx, by;
        get_block_pos(type, rot, pShot->param_i[0], &bx, &by);
        pShot->x = 115.0 + (cx + bx) * 25.0 + 12.5;
        pShot->y = 60.0 + (cy + by) * 25.0 + 12.5;
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// ライン消去時の反撃弾幕
// ---------------------------------------------------------
static void ShotRevenge(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + (GetRand(40) - 20) / 100.0; // 狙いから少しバラけさせる
            pShot->speed = 3.0 + GetRand(20) / 10.0;
            pShot->kind = img_enemyShotDiamond[7]; // 黒い菱形弾で反撃

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン (テトラ・フォール・プレッシャー)
// ---------------------------------------------------------
void EnemyPat_Tmp()
{
    static int muki;
    static int next_mino_timer;
    static int attack_timer;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 3000;
        muki = 1;
        next_mino_timer = 60;
        attack_timer = 0;
        active_mino = nullptr;

        // 盤面リセット
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 10; x++) {
                t_field[y][x] = 0;
                t_field_shots[y][x] = nullptr;
            }
        }
    }
    else {
        // ボス本体はゆっくり左右移動
        enemy.x += 0.3 * (double)muki;
        if (count % 300 == 150) muki *= -1;
    }

    // ライン消去判定 (毎フレーム下からチェック)
    int lines_cleared = 0;
    for (int y = 15; y >= 0; y--) {
        bool is_full = true;
        for (int x = 0; x < 10; x++) {
            if (t_field[y][x] == 0) {
                is_full = false;
                break;
            }
        }

        if (is_full) {
            lines_cleared++;
            // その行の弾を画面外へ飛ばして自動消去させる
            for (int x = 0; x < 10; x++) {
                if (t_field_shots[y][x]) {
                    t_field_shots[y][x]->y = 1000.0;
                }
            }
            // 上にある行を1段下げる
            for (int uy = y; uy > 0; uy--) {
                for (int ux = 0; ux < 10; ux++) {
                    t_field[uy][ux] = t_field[uy - 1][ux];
                    t_field_shots[uy][ux] = t_field_shots[uy - 1][ux];
                    if (t_field_shots[uy][ux]) {
                        t_field_shots[uy][ux]->y += 25.0; // 弾の見た目も下げる
                    }
                }
            }
            // 最上段をクリア
            for (int ux = 0; ux < 10; ux++) {
                t_field[0][ux] = 0;
                t_field_shots[0][ux] = nullptr;
            }
            y++; // 行が落ちてきたので、もう一度同じY行をチェックする
        }
    }

    // ラインを消したらプレッシャー反撃発動
    if (lines_cleared > 0) {
        attack_timer += lines_cleared * 20; // 消したライン数に応じて反撃時間延長
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // プレッシャー反撃中 (一定間隔で自機狙い弾を撃つ)
    if (attack_timer > 0) {
        if (attack_timer % 6 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->patternFunc = ShotRevenge;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        attack_timer--;
    }

    // 次のミノ生成
    if (active_mino == nullptr) {
        next_mino_timer--;
        if (next_mino_timer <= 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->patternFunc = ShotTetrimino;
            pSet->param_i[0] = GetRand(6); // ミノの種類 (0～6)
            pSet->param_i[1] = 4;          // 出現X座標(グリッド中央)
            pSet->param_d[0] = -3.0;       // 出現Y座標(画面外上部から)
            pSet->param_i[2] = 0;          // 初期回転
            pSet->param_i[3] = 0;          // 固定フラグ

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            active_mino = pSet;
            next_mino_timer = 20; // ミノ固定後、次が出るまでのインターバル
        }
    }
}