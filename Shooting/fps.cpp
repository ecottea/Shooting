#include "DxLib.h"
#include "gv.h"
#include "fps.h"
#include "imgSoundLoad.h"

void fpsTimeFunction()
{
        static int fpsTime[2]={0,}, fpsTime_i=0;
        static double fps=0.0;

        if(fpsTime_i== 0)
                fpsTime[0]=GetNowCount();
        if(fpsTime_i==49){
                fpsTime[1]=GetNowCount();
                fps = 1000.0f / ((fpsTime[1] - fpsTime[0]) / 50.0f);
                fpsTime_i=0;
        }
        else
                fpsTime_i++;
        if(fps != 0)
                DrawFormatString(565, 460, colorWhite, "FPS %.1f", fps);
        return;
}
