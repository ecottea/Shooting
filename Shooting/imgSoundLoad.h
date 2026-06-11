#pragma once

void imgSoundLoad();

// ‰æ‘œ“Ç‚İ‚İŠÖ”iID‚ğ•Ô‚·j
int LoadImage(const char* filename, double mag = 1.0, double radius = 0.0);
int LoadDivImages(const char* filename, int allNum, int xNum, int yNum,
    int xSize, int ySize, int* idBuf,
    double mag = 1.0, double radius = 0.0);
bool LoadColoredShotsEx(const char* monoFileName, int width, int height,
    int* idBuf, double mag = 1.0, double radius = 0.0);