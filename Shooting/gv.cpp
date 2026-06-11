#include "gv.h"

int count = 0;
int key[256] = {0};

int colorWhite;
int colorGray;
int colorGreenBlue;

// ---------- 画像ID管理 ----------
std::vector<ImageData> imageData;

// ---------- グローバル変数（IDを保持） ----------
int img_player;
int img_playerShot;
int img_enemy[2];
int img_enemyShotSmallBall[8];
int img_enemyShotMediumBall[8];
int img_enemyShotLargeBall[8];
int img_enemyShotBullet[8];
int img_enemyShotScale[8];
int img_enemyShotDiamond[8];

int sound_menuCursor;
int sound_enemyShot_light;
int sound_enemyShot_medium;
int sound_enemyShot_heavy;
int sound_enemyDestroyed;
int sound_playerShotHit_default;
int sound_playerShotHit_bossLowHP;
int sound_playerDestroyed;

int bgm_menu;
int currentBGMHandle;

Joutai joutaiFlag = Joutai::Menu;

int stageNum = 1;

sCursor cursor = {0, 0};
sPlayer player = {240.0, 400.0};
sPlayerShot playerShotHead{};
sEnemy enemy = {240.0, 100.0, 100, 100};
sEnemyShotSet enemyShotSetHead{};
