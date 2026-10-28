#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// ============================================================
//  へのへのもへじ弾幕 1パターン
//  素材: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
//  色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
//  仕様: count, pEnemyShotSet->count, pEnemyShot->count の加算と画面外削除はメイン側
//  GetRand(x)は0〜xのx+1通り
// ============================================================

enum {
    PART_L_BROW = 0,
    PART_R_BROW = 1,
    PART_L_EYE_RING = 2,
    PART_R_EYE_RING = 3,
    PART_L_EYE_PUPIL = 4,
    PART_R_EYE_PUPIL = 5,
    PART_MO_BODY = 6,      // も の本体
    PART_MO_TIP = 7,        // も の鼻先（突進するクナイ）
    PART_MOUTH = 8,         // へ 口
    PART_JI_DOT = 9,        // じ の点
    PART_JI_STROKE = 10,    // じ のハネ
};

// 弾生成ヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* set, double x, double y, int kind, int part, int idx)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = 0.0;
    s->speed = 0.0;
    s->kind = kind;
    s->param_i[0] = part;
    s->param_i[1] = idx;
    s->param_i[2] = 0; // 初期化済みフラグ等に自由に使える
    s->param_d[0] = 0.0;
    s->param_d[1] = 0.0;
    s->param_d[2] = 0.0;
    s->param_d[3] = 0.0;
    // 連結リストに追加
    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
    return s;
}

// ============================================================
//  弾幕本体
// ============================================================
static void ShotHenoHenoMoheji(sEnemyShotSet* pSet)
{
    const double FACE_X = pSet->x;
    const double FACE_Y = pSet->y + 50.0; // 顔の中心を敵より少し下に

    // ---- 生成フェーズ count==0 で顔を描く ----
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 色付き小玉を用途で使い分け（既存素材のみ）
        int colYellow = 1;
        int colGreen = 2;
        int colCyan = 3;
        int colMagenta = 5;
        int colWhite = 6;
        int colOrange = 8;

        // 1) 眉 へ ×2 : 針代わりに鱗弾で方向感を出す。なければ小玉でもOKだがここでは小玉黄を使う
        // へ は頂点から2方向に弾を並べる
        auto makeHe = [&](double cx, double cy, double scale, int partId, int col) {
            int n = 8; // 片側の弾数
            double arm = 5.0 * scale; // 1粒の間隔
            // 左腕: 225度方向 (左下), 右腕: 315度方向 (右下)
            for (int i = 0; i < n; ++i) {
                double d = (i + 1) * arm;
                // 左側
                double lx = cx + d * cos(225.0 / 180.0 * DX_PI);
                double ly = cy + d * sin(225.0 / 180.0 * DX_PI);
                AddShot(pSet, lx, ly, img_enemyShotSmallBall[col], partId, i);
                // 右側
                double rx = cx + d * cos(315.0 / 180.0 * DX_PI);
                double ry = cy + d * sin(315.0 / 180.0 * DX_PI);
                AddShot(pSet, rx, ry, img_enemyShotSmallBall[col], partId, i + n);
            }
            // 頂点
            AddShot(pSet, cx, cy, img_enemyShotSmallBall[col], partId, 99);
        };

        makeHe(FACE_X - 38.0, FACE_Y - 50.0, 1.0, PART_L_BROW, colYellow);
        makeHe(FACE_X + 38.0, FACE_Y - 50.0, 1.0, PART_R_BROW, colYellow);
        makeHe(FACE_X + 0.0, FACE_Y + 40.0, 1.6, PART_MOUTH, colMagenta); // 口は大きく

        // 2) 目 の ×2 : 小玉リング + 中玉瞳
        auto makeNo = [&](double cx, double cy, int ringPart, int pupilPart, int colRing) {
            const int ringN = 16;
            const double R = 14.0;
            for (int i = 0; i < ringN; ++i) {
                double ang = 2.0 * DX_PI * i / ringN;
                double x = cx + R * cos(ang);
                double y = cy + R * sin(ang);
                sEnemyShot* s = AddShot(pSet, x, y, img_enemyShotSmallBall[colRing], ringPart, i);
                // 回転用パラメータを保存
                s->param_d[0] = cx; // center x
                s->param_d[1] = cy; // center y
                s->param_d[2] = R;
                s->param_d[3] = ang; // 現在角
            }
            // 瞳
            AddShot(pSet, cx, cy, img_enemyShotMediumBall[colWhite], pupilPart, 0);
        };
        makeNo(FACE_X - 32.0, FACE_Y - 18.0, PART_L_EYE_RING, PART_L_EYE_PUPIL, colCyan);
        makeNo(FACE_X + 32.0, FACE_Y - 18.0, PART_R_EYE_RING, PART_R_EYE_PUPIL, colCyan);

        // 3) 鼻 も : へのへのもへじで一番複雑な字を折れ線で近似
        // も の軌跡を手作り
        struct Pt { double x, y; };
        std::vector<Pt> moPts;
        // 上横線 -12,0 -> 12,0
        for (int i = 0; i <= 6; ++i) moPts.push_back({ -12.0 + i * 4.0, 0.0 });
        // 右縦 12,0 -> 12,12
        for (int i = 1; i <= 3; ++i) moPts.push_back({ 12.0, 0.0 + i * 4.0 });
        // 中横 12,12 -> -8,12
        for (int i = 1; i <= 5; ++i) moPts.push_back({ 12.0 - i * 4.0, 12.0 });
        // 左縦 -8,12 -> -8,26
        for (int i = 1; i <= 3; ++i) moPts.push_back({ -8.0, 12.0 + i * 4.5 });
        // 下横 -8,26 -> 12,26
        for (int i = 1; i <= 5; ++i) moPts.push_back({ -8.0 + i * 4.0, 26.0 });

        for (size_t i = 0; i < moPts.size(); ++i) {
            double wx = FACE_X + moPts[i].x;
            double wy = FACE_Y + moPts[i].y;
            if (i == moPts.size() - 1) {
                // 最後の点が鼻先＝突進弾。銃弾で尖らせる
                sEnemyShot* tip = AddShot(pSet, wx, wy, img_enemyShotBullet[colGreen], PART_MO_TIP, (int)i);
                tip->param_d[0] = FACE_X; // 顔中心を覚えておく（後で方向計算に使える）
                tip->param_d[1] = FACE_Y;
            }
            else {
                AddShot(pSet, wx, wy, img_enemyShotSmallBall[colGreen], PART_MO_BODY, (int)i);
            }
        }

        // 4) じ
        // 点
        AddShot(pSet, FACE_X + 36.0, FACE_Y + 42.0, img_enemyShotLargeBall[colOrange], PART_JI_DOT, 0);
        // ハネ : 口の右下から左下へ払う
        for (int i = 0; i < 7; ++i) {
            double t = i / 6.0;
            // ベジェっぽいカーブ: (32,52) -> (22,66) に少し膨らみ
            double x = FACE_X + 32.0 - t * 14.0 + sin(t * DX_PI) * 2.0;
            double y = FACE_Y + 52.0 + t * 14.0;
            AddShot(pSet, x, y, img_enemyShotSmallBall[colOrange], PART_JI_STROKE, i);
        }
    }

    // ---- 予告 90Fでチャージ音 ----
    if (pSet->count == 80) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 更新フェーズ ----
    // 顔中心（計算用）
    const double fcX = FACE_X;
    const double fcY = FACE_Y;

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next; // 途中で追加しても安全なように先に保存
        int part = p->param_i[0];

        if (pSet->count < 80) {
            // 0〜79F: 完全固定で顔を見せる。speed 0 のまま
        }
        else if (pSet->count < 240) {
            // 80〜239F: パーツごとに動き出す
            switch (part) {
            case PART_L_EYE_RING:
            case PART_R_EYE_RING: {
                // くるくる回転
                p->param_d[3] += 0.06; // 角速度
                double cx = p->param_d[0];
                double cy = p->param_d[1];
                double R = p->param_d[2];
                double ang = p->param_d[3];
                // 少しずつ縮む演出
                if (pSet->count > 180) R *= 0.995;
                p->param_d[2] = R;
                p->x = cx + R * cos(ang);
                p->y = cy + R * sin(ang);

                // 180F以降は外側に拡散弾として飛ばす
                if (pSet->count == 180) {
                    p->muki = ang;
                    p->speed = 1.2 + GetRand(80) / 100.0;
                }
                if (pSet->count > 180) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                break;
            }
            case PART_L_EYE_PUPIL:
                if (pSet->count == 95) {
                    p->muki = atan2(player.y - p->y, player.x - p->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
                    p->speed = 2.8;
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
                if (p->speed > 0) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                break;
            case PART_R_EYE_PUPIL:
                if (pSet->count == 110) {
                    p->muki = atan2(player.y - p->y, player.x - p->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
                    p->speed = 2.8;
                }
                if (p->speed > 0) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                break;
            case PART_L_BROW:
                if (pSet->count == 90) {
                    p->muki = DX_PI * 0.5 + (GetRand(30) - 15) / 180.0 * DX_PI;
                    p->speed = 1.0 + GetRand(60) / 100.0;
                }
                if (p->speed > 0) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                    p->speed += 0.02; // 少し加速
                }
                break;
            case PART_R_BROW:
                if (pSet->count == 100) {
                    p->muki = DX_PI * 0.5 + (GetRand(30) - 15) / 180.0 * DX_PI;
                    p->speed = 1.0 + GetRand(60) / 100.0;
                }
                if (p->speed > 0) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                    p->speed += 0.02;
                }
                break;
            case PART_MO_TIP: // 鼻先だけプレイヤーへ突進
                if (pSet->count == 130) {
                    p->muki = atan2(player.y - p->y, player.x - p->x);
                    p->speed = 4.2;
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
                if (p->speed > 0) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                break;
            case PART_MO_BODY:
                if (pSet->count >= 140) {
                    // も の本体はゆっくり落下
                    if (p->param_i[2] == 0) { p->param_i[2] = 1; p->muki = DX_PI * 0.5; p->speed = 0.3 + GetRand(40) / 100.0; }
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                break;
            case PART_MOUTH:
                if (pSet->count == 120) {
                    // 口からブレス: 顔中心から外へ扇状
                    double angFromCenter = atan2(p->y - fcY, p->x - fcX);
                    p->muki = angFromCenter + (GetRand(30) - 15) / 180.0 * DX_PI;
                    p->speed = 1.8 + GetRand(80) / 100.0;
                }
                if (p->speed > 0) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                // 追加で口の中央から米粒をばら撒く（ブレス演出）
                if (pSet->count >= 120 && pSet->count % 6 == 0 && p->param_i[1] == 99) {
                    // 頂点弾だけが追加弾を産む
                    for (int k = 0; k < 2; ++k) {
                        sEnemyShot* b = new sEnemyShot;
                        b->x = p->x;
                        b->y = p->y;
                        b->muki = DX_PI * 0.5 + (GetRand(80) - 40) / 180.0 * DX_PI + (k == 0 ? -0.25 : 0.25);
                        b->speed = 2.0 + GetRand(60) / 100.0;
                        b->kind = img_enemyShotSmallBall[5]; // マゼンタ小玉
                        b->param_i[0] = 100; // 追加弾識別
                        b->prev = pSet->pEnemyShotHead->prev;
                        b->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = b;
                        pSet->pEnemyShotHead->prev = b;
                    }
                }
                break;
            case PART_JI_DOT:
            case PART_JI_STROKE:
                if (pSet->count >= 150) {
                    if (p->param_i[2] == 0) {
                        p->param_i[2] = 1;
                        p->muki = atan2(p->y - fcY, p->x - fcX);
                        p->speed = 0.8 + GetRand(60) / 100.0;
                    }
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                    p->speed += 0.01;
                }
                break;
            default: // 追加のブレス弾など
                if (p->param_i[0] == 100) {
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                break;
            }
        }
        else {
            // 240F以降は全弾ゆっくり落下させて画面外へ
            if (p->speed < 0.1) {
                p->muki = DX_PI * 0.5 + (GetRand(60) - 30) / 180.0 * DX_PI;
                p->speed = 0.5 + GetRand(100) / 100.0;
            }
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            p->speed += 0.01;
        }

        p = pNext;
    }
}

// ============================================================
//  敵本体: 仕様通り void EnemyPat_Henohenomoheji_MetaAI()
// ============================================================
void EnemyPat_Henohenomoheji_MetaAI()
{
    static int dir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }
    else {
        // ゆっくり左右往復、顔が描きやすいように控えめな動き
        enemy.x += 0.6 * (double)dir;
        if (enemy.x < 120.0) dir = 1;
        if (enemy.x > 360.0) dir = -1;
    }

    // 180Fごとに顔を再描画
    if (count % 120 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHenoHenoMoheji;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = (count / 180) % 8; // 使わないが一応回す

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}
