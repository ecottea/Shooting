// ============================================================
//  飛鉢「伝説の飛空円盤」 実装
//  enemyPat_sampleForAI.cpp の作法に準拠
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 円盤（リング）弾幕の更新関数
// sEnemyShotSet 1つが「1つの円」に対応する
static void ShotFlyingSaucer(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    // 初回のみ：リングを構成する札弾を生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_light / medium / heavy / extreme / charge
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int bulletCount = pEnemyShotSet->param_i[0]; // この円の弾数
        int colorStage = pEnemyShotSet->param_i[1]; // 0:青 1:水色 2:黄 3:紫 4:赤
        int mode = pEnemyShotSet->param_i[2]; // 0:固定円 1:自機狙い円
        double baseAngle = pEnemyShotSet->param_d[9]; // 自機狙い時の基準角度

        // 段階に応じた弾の色を選択
        // 色一覧: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
        int colorIndex = 0;
        switch (colorStage) {
        case 0: colorIndex = 4; break; // 青
        case 1: colorIndex = 3; break; // シアン（水色）
        case 2: colorIndex = 1; break; // 黄
        case 3: colorIndex = 5; break; // マゼンタ（紫）
        case 4: colorIndex = 0; break; // 赤
        default: colorIndex = 0; break;
        }

        // 札状の弾として「鱗弾(img_enemyShotScale)」を使用
        // 弾の種類一覧: 小玉, 中玉, 大玉, 銃弾, 鱗弾, 菱形弾, 中楕円弾, 短レーザー
        for (int i = 0; i < bulletCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 位置は毎フレーム再計算するため、初期値はリング中心でよい
            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = pEnemyShotSet->param_d[1];
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotScale[colorIndex];
            pEnemyShot->margin = 240;

            // リング内での角度を保存
            double angle = (2.0 * DX_PI / bulletCount) * i;
            if (mode == 1) {
                // 自機狙い円：生成瞬間の自機方向を基準に角度を割り当てる
                angle += baseAngle;
            }
            pEnemyShot->param_d[0] = angle; // リング内角度

            // 消去フラグ（0:消去不可 / 1:消去可能）
            // 輪が広がりきるまでボムでも消えない原作仕様を再現するための目印
            pEnemyShot->param_i[3] = 0;

            // 双方向リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 毎フレームの更新：膨張・収縮しながら降下
    // --------------------------------------------------------
    // 位相を進める
    pEnemyShotSet->param_d[5] += pEnemyShotSet->param_d[6];

    // 現在の半径 = 基準半径 + 振幅 * sin(位相)
    double radius = pEnemyShotSet->param_d[2]
        + pEnemyShotSet->param_d[4] * sin(pEnemyShotSet->param_d[5]);

    // 円全体をゆっくり降下させる
    pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[7];

    // 円自体の回転（0なら回転なし）
    double rotation = pEnemyShotSet->param_d[8] * pEnemyShotSet->count;

    // 各弾の位置をリングの中心・半径・角度から計算
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0] + rotation;
        pShot->x = pEnemyShotSet->param_d[0] + radius * cos(angle);
        pShot->y = pEnemyShotSet->param_d[1] + radius * sin(angle);
        pShot->muki = angle + DX_PI / 2;

        // 輪が十分に広がったら消去可能フラグを立てる
        if (radius > pEnemyShotSet->param_d[2] * 1.5) {
            pShot->param_i[3] = 1;
        }

        pShot = pShot->next;
    }

    // この円に属する弾がすべて消えたら、円（セット）自体をリストから外して削除
    if (pEnemyShotSet->pEnemyShotHead->next == pEnemyShotSet->pEnemyShotHead) {
        pEnemyShotSet->prev->next = pEnemyShotSet->next;
        pEnemyShotSet->next->prev = pEnemyShotSet->prev;
        delete pEnemyShotSet;
    }
}

// ============================================================
//  敵本体のパターン：飛鉢「伝説の飛空円盤」
// ============================================================
void EnemyPat_HikouEnban_DeepSeek()
{
    static int muki;       // ボスの左右移動方向
    static int ringIndex;  // 生成する円の順番（固定→自機狙い→自機狙い→固定）

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        ringIndex = 0;
    }
    else {
        // ボス本体は左右に往復移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // --------------------------------------------------------
    // HP比率に応じた段階（色と密度の変化）
    // --------------------------------------------------------
    double hpRatio = (double)enemy.hp / enemy.maxHp;
    int stage;
    if (hpRatio > 0.8) stage = 0; // 青
    else if (hpRatio > 0.6) stage = 1; // 水色
    else if (hpRatio > 0.4) stage = 2; // 黄
    else if (hpRatio > 0.2) stage = 3; // 紫
    else                    stage = 4; // 赤

    // 攻撃間隔：段階が上がると短くなる
    int interval = 90 - stage * 10;
    if (interval < 30) interval = 30;

    // --------------------------------------------------------
    // 円（リング）の生成
    // --------------------------------------------------------
    if (count % interval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlyingSaucer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // --- 弾数：段階が上がるほど密度アップ ---
        int bulletCount = 12 + stage * 4; // 12, 16, 20, 24, 28
        pEnemyShotSet->param_i[0] = bulletCount;
        pEnemyShotSet->param_i[1] = stage; // 色段階

        // --- 円の種類：固定→自機狙い→自機狙い→固定 の繰り返し ---
        int mode;
        switch (ringIndex % 4) {
        case 0: mode = 0; break; // 固定
        case 1: mode = 1; break; // 自機狙い
        case 2: mode = 1; break; // 自機狙い
        case 3: mode = 0; break; // 固定
        default: mode = 0; break;
        }
        pEnemyShotSet->param_i[2] = mode;

        // --- 円の運動パラメータ ---
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 中心X
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 中心Y
        pEnemyShotSet->param_d[2] = 40.0 + stage * 5.0; // 基準半径
        pEnemyShotSet->param_d[3] = pEnemyShotSet->param_d[2]; // 現在半径（初期値）
        pEnemyShotSet->param_d[4] = 15.0 + stage * 2.0; // 膨張・収縮の振幅
        pEnemyShotSet->param_d[5] = 0.0; // 位相
        pEnemyShotSet->param_d[6] = 0.03 + stage * 0.005; // 位相速度
        pEnemyShotSet->param_d[7] = 0.8 + stage * 0.1; // 降下速度
        pEnemyShotSet->param_d[8] = 0.0; // 円の回転速度（0で回転なし）
        // 自機狙いの基準角度：生成瞬間の自機方向
        pEnemyShotSet->param_d[9] = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 双方向リストへ追加
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        ringIndex++;
    }

    // 補足：
    // ・このスペルカード中、原作ではボス本体に当たり判定がありません。
    //   必要に応じてメインルーチン側で enemy の当たり判定を無効にしてください。
    // ・弾同士を結ぶ「線」は、描画側で sEnemyShotSet の弾リストを走査し、
    //   角度順（param_d[0] の昇順）に隣接する弾を線で結ぶことで再現できます。
    //   加算合成で描画すると原作に近い発光表現になります。
    // ・ボムの弾消し処理では、sEnemyShot::param_i[3] が 1 の弾のみを対象にすると、
    //   「輪が広がりきるまで消えない」という原作の挙動に近づけられます。
}