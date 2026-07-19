#pragma once

#include <string>
#include <vector>

void menuDraw();
void moveCursor();
std::vector<std::string> WrapText(const char* text, int maxWidth);
