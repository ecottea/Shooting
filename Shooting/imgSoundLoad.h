#pragma once

extern int colorWhite;
extern int colorGray;
extern int colorGreenBlue;

// ---------- 画像管理用の構造体とグローバル変数 ----------
struct ImageData {
    int    handle = -1;
    double mag = 1.0;
    bool   no_rotate = false;
    double radius = 0.0;
}; 
extern std::vector<ImageData> imageData;

// 画像ID
extern int img_player;
extern int img_playerShot;
extern int img_enemy[2];
extern int img_enemyShotSmallBall[8];
extern int img_enemyShotMediumBall[8];
extern int img_enemyShotLargeBall[8];
extern int img_enemyShotBullet[8];
extern int img_enemyShotScale[8];
extern int img_enemyShotDiamond[8];

extern int sound_menuCursor;
extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;
extern int sound_enemyShot_heavy;
extern int sound_enemyShot_noize;
extern int sound_enemyDestroyed;
extern int sound_playerShotHit_default;
extern int sound_playerShotHit_bossLowHP;
extern int sound_playerDestroyed;

extern int bgm_menu;
extern int currentBGMHandle; 


void imgSoundLoad();
