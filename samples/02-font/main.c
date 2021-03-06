/*
	The Game Engine Library is a multiplatform library made to make games
	Copyright (C) 2012  Aubry Adrien (dridri85)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libge/libge.h>

int main(int argc, char** argv){
	geInit();
	geCreateMainWindow("LibGE Win32 low profile", 256, 256, 0);

	ge_Image* image = geLoadImage("data/image.png");

	ge_Font* font = geLoadFont("data/arial.ttf");
	geFontSize(font, 30);

	while(1){
		geClearScreen();
	
		geDrawImage(0, 0, image);
		geFontPrintScreen(2, 2, font, "Hello LibGE !", RGBA(255, 255, 255, 255));

		geSwapBuffers();
	}

	return 0;
}
