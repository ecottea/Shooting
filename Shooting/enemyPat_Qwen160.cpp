#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  Hyper-Tesseract Lattice (超立方体格子の舞)
//  4次元空間における正八胞体の回転を2D平面へ射影した弾幕
// ============================================================

// テッセラクトの16頂点 (x, y, z, w)
static const double tesseract_vertices[16][4] = {
    {-1, -1, -1, -1}, { 1, -1, -1, -1}, {-1,  1, -1, -1}, { 1,  1, -1, -1},
    {-1, -1,  1, -1}, { 1, -1,  1, -1}, {-1,  1,  1, -1}, { 1,  1,  1, -1},
    {-1, -1, -1,  1}, { 1, -1, -1,  1}, {-1,  1, -1,  1}, { 1,  1, -1,  1},
    {-1, -1,  1,  1}, { 1, -1,  1,  1}, {-1,  1,  1,  1}, { 1,  1,  1,  1}
};

// テッセラクトの32本の辺 (頂点インデックスのペア)
static const int tesseract_edges[32][2] = {
    {0,1}, {0,2}, {0,4}, {0,8},
    {1,3}, {1,5}, {1,9},
    {2,3}, {2,6}, {2,10},
    {3,7}, {3,11},
    {4,5}, {4,6}, {4,12},
    {5,7}, {5,13},
    {6,7}, {6,14},
    {7,15},
    {8,9}, {8,10}, {8,12},
    {9,11}, {9,13},
    {10,11}, {10,14},
    {11,15},
    {12,13}, {12,14},
    {13,15},
    {14,15}
};

static void ShotHyperTesseract(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 幾何学パターンの展開予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);


        // 生成時の count を基準に回転角を計算 (kind に生成時の count を格納している)
        double base_count = (double)pEnemyShotSet->kind;

        // 4次元回転: XY平面と ZW平面 で独立して回転させる
        // TASは この回転速度(0.015, 0.025) から未来の座標を正確に予測できる
        double theta = base_count * 0.015;
        double phi = base_count * 0.025;

        double cos_t = cos(theta), sin_t = sin(theta);
        double cos_p = cos(phi), sin_p = sin(phi);

        // 頂点の射影結果一時保存
        double proj_x[16], proj_y[16], w_val[16];

        for (int i = 0; i < 16; i++) {
            double x = tesseract_vertices[i][0];
            double y = tesseract_vertices[i][1];
            double z = tesseract_vertices[i][2];
            double w = tesseract_vertices[i][3];

            // XY平面の回転
            double nx = x * cos_t - y * sin_t;
            double ny = x * sin_t + y * cos_t;

            // ZW平面の回転
            double nz = z * cos_p - w * sin_p;
            double nw = z * sin_p + w * cos_p;

            // 2D画面への射影 (nw を奥行きパラメータとした簡易透視投影)
            // nw が大きいほど手前(拡大)、小さいほど奥(縮小)
            double scale = 140.0 / (2.5 - nw * 0.6);
            proj_x[i] = nx * scale + pEnemyShotSet->x;
            proj_y[i] = ny * scale + pEnemyShotSet->y;
            w_val[i] = nw;
        }

        // 32本の辺それぞれについて弾(レーザー)を生成
        for (int i = 0; i < 32; i++) {
            int v1 = tesseract_edges[i][0];
            int v2 = tesseract_edges[i][1];

            double mid_x = (proj_x[v1] + proj_x[v2]) / 2.0;
            double mid_y = (proj_y[v1] + proj_y[v2]) / 2.0;
            double mid_w = (w_val[v1] + w_val[v2]) / 2.0;

            // 【TAS向け超高難易度ギミック: 特異点の隙間】
            // 回転角が特定の値に近づくと、4次元的に「厚みが0」になり干渉しなくなる辺が発生する
            // 視覚的には格子が交差して危険に見えるが、数式上はここが唯一の安全地帯となる
            if (fmod(base_count, 180.0) > 172.0 && fmod(base_count, 180.0) < 188.0) {
                if (mid_w > -0.25 && mid_w < 0.25) {
                    continue; // あえて弾を生成しない = TASだけが知る絶対安全地帯
                }
            }

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = mid_x;
            pShot->y = mid_y;

            // レーザーの向きは辺の方向
            pShot->muki = atan2(proj_y[v2] - proj_y[v1], proj_x[v2] - proj_x[v1]);

            // 速度は奥行きに応じて変化させる (手前ほど速く拡散する視覚効果)
            pShot->speed = 0.3 + (mid_w + 1.0) * 0.4;

            // 【視覚的錯覚と実判定の表現】
            // mid_w の値によって色を変え、TASに「手前(危険)」と「奥(安全)」を判断させる
            // 色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            int color = 6; // デフォルト: 白
            if (mid_w > 0.5) {
                color = 0;     // 手前: 赤 (最高警戒)
            }
            else if (mid_w < -0.5) {
                color = 4;     // 奥: 青 (視覚的に交差していても、実際は奥なので安全地帯を縫える)
            }
            else if (mid_w > 0.0) {
                color = 8;     // 中手前: 橙
            }
            else {
                color = 3;     // 中奥: シアン
            }

            // 弾の種類に「短レーザー」を採用し、幾何学的な線分として表現する
            pShot->kind = img_enemyShotLaser[color];
            pShot->margin = 80;

            // 双方向リンクリストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン (TAS用 幾何学弾幕)
void EnemyPat_GeometryTAS_Qwen()
{
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // 格子が画面中央付近に展開されるように調整
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }
    else {
        // 敵本体はリサージュ曲線を描くようにゆっくりと動き、格子の起点をずらす
        // これにより、静止した格子ではなく「舞う」ような動態的な幾何学模様になる
        enemy.x = 240.0 + 80.0 * sin(count * 0.008);
        enemy.y = 60.0 + 30.0 * cos(count * 0.012);
    }

    // 6フレームごとに新しいテッセラクトのフレームを生成
    // 間隔を詰めることで、回転する超立方体の「残像」が重なり合い、
    // 複雑で美しいフラクタル的な干渉縞を画面に描き出す
    if (count % 6 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperTesseract;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // 生成時の count を記録し、回転角計算の基準とする
        pEnemyShotSet->kind = count;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }
}