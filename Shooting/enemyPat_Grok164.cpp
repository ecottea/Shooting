#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 北斗七星モチーフ弾幕「北斗の陣・天の川流し」
// 使用素材:
//   - 星本体     : 中玉 (img_enemyShotMediumBall) シアン
//   - 輝き放射   : 小玉 (img_enemyShotSmallBall) 白
//   - 回転中放射 : 中玉 (img_enemyShotMediumBall) 青
//   - 誘導弾     : 菱形弾 (img_enemyShotDiamond) マゼンタ
//   - 流星       : 銃弾 (img_enemyShotBullet) 黄・橙
//   - フィニッシュばら撒き : 中玉 (img_enemyShotMediumBall) 白
//   - 星座の線   : 短レーザー (img_enemyShotLaser) シアン（一瞬のみ）

static void ShotBigDipper(sEnemyShotSet* pEnemyShotSet)
{
    // 北斗七星の相対座標（中心基準、画面上で柄が左・斗が右）
    // 距離を短レーザー(64)以内に収めるためスケール調整済み
    static const double star_ox[7] = {
        -72.0,  // 0: 揺光 (Alkaid) 柄の先端
        -44.0,  // 1: 開陽 (Mizar)
        -16.0,  // 2: 玉衡 (Alioth)
          8.0,  // 3: 天権 (Megrez) 接続点
         24.0,  // 4: 天璣 (Phecda)
         48.0,  // 5: 天璇 (Merak)
         36.0   // 6: 天枢 (Dubhe)
    };
    static const double star_oy[7] = {
         12.0,
          6.0,
          0.0,
         -6.0,
         32.0,
         40.0,
        -22.0
    };

    // 星座の接続関係（レーザー用）
    static const int connect[7][2] = {
        {0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,6}, {6,3}
    };

    sEnemyShot* pEnemyShot;

    //--------------------------------------------------
    // 初期化（count == 0）
    //--------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 中心座標をセット（画面上部中央付近）
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 110.0;

        // 回転角度初期化
        pEnemyShotSet->param_d[0] = 0.0;          // 現在の回転角
        pEnemyShotSet->param_d[1] = 0.015;        // 角速度

        // 7つの星を生成（最初はすべて中心に重ねておく）
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン中玉
            pEnemyShot->param_i[0] = i;                    // 星ID (0〜6)
            pEnemyShot->param_i[1] = 0;                    // 予備
            pEnemyShot->margin = 40.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------
    // フェーズ定義（フレーム単位）
    //--------------------------------------------------
    const int FORMATION_END = 90;   // 形成完了
    const int SHINE_END = 150;  // 輝き終了
    const int ROTATE_END = 300;  // 回転終了
    const int METEOR_END = 480;  // 流星終了

    int cnt = pEnemyShotSet->count;
    double center_x = pEnemyShotSet->x;
    double center_y = pEnemyShotSet->y;
    double angle = pEnemyShotSet->param_d[0];

    //--------------------------------------------------
    // 弾の更新ループ
    //--------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int id = pShot->param_i[0];

        if (id >= 0 && id <= 6) {
            // ===== 星本体の位置制御 =====
            double ox = star_ox[id];
            double oy = star_oy[id];

            // 回転適用
            double cos_a = cos(angle);
            double sin_a = sin(angle);
            double rx = ox * cos_a - oy * sin_a;
            double ry = ox * sin_a + oy * cos_a;

            if (cnt <= FORMATION_END) {
                // 形成フェーズ：中心から目標位置へ補間
                double t = (double)cnt / FORMATION_END;
                // ease-out
                t = 1.0 - (1.0 - t) * (1.0 - t);
                pShot->x = center_x + rx * t;
                pShot->y = center_y + ry * t;
            }
            else {
                // 形成後は完全に追従
                pShot->x = center_x + rx;
                pShot->y = center_y + ry;
            }

            // 星自体は動かさない
            pShot->speed = 0.0;
        }
        else if (id == -2) {
            // ===== 一時レーザー（星座の線） =====
            // 一定時間後に画面外へ飛ばして消去させる
            if (pShot->count > 18) {
                pShot->y = -999.0;
            }
        }
        else {
            // ===== 通常弾・誘導弾の移動 =====
            // 誘導処理（param_i[1] == 1 のとき）
            if (pShot->param_i[1] == 1) {
                double target = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = target - pShot->muki;
                // -PI〜PI に正規化
                while (diff > DX_PI) diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                // 旋回制限
                if (diff > 0.04) diff = 0.04;
                if (diff < -0.04) diff = -0.04;
                pShot->muki += diff;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    //--------------------------------------------------
    // フェーズ別の弾生成
    //--------------------------------------------------

    // 形成完了時に星座の線（短レーザー）を一瞬表示
    if (cnt == FORMATION_END) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int c = 0; c < 7; c++) {
            int a = connect[c][0];
            int b = connect[c][1];

            // 現在の星位置を取得するため、もう一度リストを走査
            double ax = 0, ay = 0, bx = 0, by = 0;
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] == a) { ax = p->x; ay = p->y; }
                if (p->param_i[0] == b) { bx = p->x; by = p->y; }
                p = p->next;
            }

            double mx = (ax + bx) * 0.5;
            double my = (ay + by) * 0.5;
            double mk = atan2(by - ay, bx - ax);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = mx;
            pEnemyShot->y = my;
            pEnemyShot->muki = mk;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[3]; // シアン短レーザー
            pEnemyShot->param_i[0] = -2;              // 一時レーザー識別
            pEnemyShot->param_i[1] = 0;
            pEnemyShot->margin = 20.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 輝きフェーズ：各星から小玉を8方向に放射
    if (cnt > FORMATION_END && cnt <= SHINE_END) {
        if (cnt % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] >= 0 && p->param_i[0] <= 6) {
                    for (int i = 0; i < 8; i++) {
                        pEnemyShot = new sEnemyShot;
                        pEnemyShot->x = p->x;
                        pEnemyShot->y = p->y;
                        pEnemyShot->muki = i * (DX_PI * 2.0 / 8.0) + angle;
                        pEnemyShot->speed = 1.4 + GetRand(40) / 100.0;
                        pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白小玉
                        pEnemyShot->param_i[0] = -1;
                        pEnemyShot->param_i[1] = 0;
                        pEnemyShot->margin = 20.0;

                        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    }
                }
                p = p->next;
            }
        }
    }

    // 回転フェーズ：全体回転 + 外側放射 + 柄先端から誘導弾
    if (cnt > SHINE_END && cnt <= ROTATE_END) {
        // 回転を進める
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1];

        // 外側放射
        if (cnt % 18 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] >= 0 && p->param_i[0] <= 6) {
                    // 中心からの放射方向
                    double out_muki = atan2(p->y - center_y, p->x - center_x);
                    for (int k = 0; k < 3; k++) {
                        pEnemyShot = new sEnemyShot;
                        pEnemyShot->x = p->x;
                        pEnemyShot->y = p->y;
                        pEnemyShot->muki = out_muki + (k - 1) * 0.18;
                        pEnemyShot->speed = 2.0 + GetRand(80) / 100.0;
                        pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青中玉
                        pEnemyShot->param_i[0] = -1;
                        pEnemyShot->param_i[1] = 0;
                        pEnemyShot->margin = 20.0;

                        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    }
                }
                p = p->next;
            }
        }

        // 柄の先端（星0）から誘導弾
        if (cnt % 25 == 5) {
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] == 0) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = p->x;
                    pEnemyShot->y = p->y;
                    pEnemyShot->muki = atan2(player.y - p->y, player.x - p->x);
                    pEnemyShot->speed = 2.6;
                    pEnemyShot->kind = img_enemyShotDiamond[5]; // マゼンタ菱形
                    pEnemyShot->param_i[0] = -1;
                    pEnemyShot->param_i[1] = 1;                 // 誘導フラグ
                    pEnemyShot->margin = 10.0;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    break;
                }
                p = p->next;
            }
        }
    }

    // 流星フェーズ：柄から大量の弾を下へ流し、斗はばら撒き
    if (cnt > ROTATE_END && cnt <= METEOR_END) {
        // 回転を少し加速
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1] * 1.6;

        // 柄（星0,1,2）から流星
        if (cnt % 3 == 0) {
            if (cnt % 9 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                int sid = p->param_i[0];
                if (sid >= 0 && sid <= 2) {
                    int num = 2 + GetRand(2);
                    for (int n = 0; n < num; n++) {
                        pEnemyShot = new sEnemyShot;
                        pEnemyShot->x = p->x + GetRand(12) - 6;
                        pEnemyShot->y = p->y + GetRand(8) - 4;
                        // 下方向を基調に少しばらつかせる
                        pEnemyShot->muki = DX_PI * 0.5 + (GetRand(50) - 25) / 180.0 * DX_PI;
                        pEnemyShot->speed = 3.2 + GetRand(180) / 100.0;
                        // 黄と橙を混ぜる
                        int col = (GetRand(1) == 0) ? 1 : 8;
                        pEnemyShot->kind = img_enemyShotBullet[col];
                        pEnemyShot->param_i[0] = -1;
                        pEnemyShot->param_i[1] = 0;
                        pEnemyShot->margin = 20.0;

                        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    }
                }
                p = p->next;
            }
        }

        // 斗（星3〜6）のフィニッシュばら撒き（終盤のみ）
        if (cnt > ROTATE_END + 60 && cnt % 20 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                int sid = p->param_i[0];
                if (sid >= 3 && sid <= 6) {
                    for (int i = 0; i < 6-2; i++) {
                        pEnemyShot = new sEnemyShot;
                        pEnemyShot->x = p->x;
                        pEnemyShot->y = p->y;
                        pEnemyShot->muki = atan2(player.y - p->y, player.x - p->x)
                            + (GetRand(80) - 40) / 180.0 * DX_PI;
                        pEnemyShot->speed = 2.4 + GetRand(120) / 100.0;
                        pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白中玉
                        pEnemyShot->param_i[0] = -1;
                        pEnemyShot->param_i[1] = 0;
                        pEnemyShot->margin = 20.0;

                        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    }
                }
                p = p->next;
            }
        }
    }

    // 流星終了後はゆっくり回転を続けつつ、低密度の放射を継続（パターン維持）
    if (cnt > METEOR_END && cnt < 600) {
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1] * 0.6;

        if (cnt % 30 == 0) {
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] >= 0 && p->param_i[0] <= 6) {
                    double out_muki = atan2(p->y - center_y, p->x - center_x);
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = p->x;
                    pEnemyShot->y = p->y;
                    pEnemyShot->muki = out_muki;
                    pEnemyShot->speed = 1.8;
                    pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン小玉
                    pEnemyShot->param_i[0] = -1;
                    pEnemyShot->param_i[1] = 0;
                    pEnemyShot->margin = 20.0;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
                p = p->next;
            }
        }
    }
}

// 敵本体のパターン
void EnemyPat_BigDipper_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 北斗七星パターンを1回だけ生成
    if (count % 600 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBigDipper;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 110.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
