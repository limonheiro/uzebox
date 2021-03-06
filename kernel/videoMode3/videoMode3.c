/*
 *  Uzebox Kernel - Mode 3
 *  Copyright (C) 2008  Alec Bourque
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Uzebox is a reserved trade mark
*/

	#include <stdbool.h>
	#include <avr/io.h>
	#include <stdlib.h>
	#include <avr/pgmspace.h>
	#include <avr/interrupt.h>
	#include "uzebox.h"
	#include "intro.h"
	

	#if INTRO_LOGO !=0
		#include "videoMode3/uzeboxlogo_8x8.pic.inc"
		#include "videoMode3/uzeboxlogo_8x8.map.inc"
	#endif

	extern unsigned char overlay_vram[];
	extern unsigned char ram_tiles[];
	extern struct SpriteStruct sprites[];
	extern unsigned char *sprites_tiletable_lo;
	extern unsigned int sprites_tile_banks[];
	extern unsigned char *tile_table_lo;
	extern struct BgRestoreStruct ram_tiles_restore[];

	extern void BlitSprite(unsigned char spriteNo,unsigned char ramTileNo,unsigned int xy,unsigned int dxdy);

	unsigned char free_tile_index, userRamTilesCount=0,userRamTilesCount_tmp=0;
	bool spritesOn=true;

	void RestoreBackground(){
		unsigned char i;
		for(i=userRamTilesCount;i<free_tile_index;i++){
			//vram[ram_tiles_restore[i].addr]=ram_tiles_restore[i].tileIndex;
			*ram_tiles_restore[i].addr=ram_tiles_restore[i].tileIndex;
		}	
	}

	void SetUserRamTilesCount(u8 count){
		userRamTilesCount_tmp=count;		
	}

	u8* GetUserRamTile(u8 index){
		return ram_tiles+(index*TILE_HEIGHT*TILE_WIDTH);
	}

	void SetSpriteVisibility(bool visible){
		spritesOn=visible;
	}

	/*
	//
	// This C function is the direct equivalent of the assembly
	// function of the same same.
	//
	void BlitSprite(u8 sprNo,u8 ramTileIndex,u16 yx,u16 dydx){
		u8 dy=dydx>>8;
		u8 dx=dydx &0xff;
		u8 flags=sprites[sprNo].flags;
		u8 destXdiff,ydiff,px,x2,y2;
		s8 step=1,srcXdiff;

		u16 src=(sprites[sprNo].tileIndex*TILE_HEIGHT*TILE_WIDTH)
				+sprites_tile_banks[flags>>6];	//add bank adress		

		u8* dest=&ram_tiles[ramTileIndex*TILE_HEIGHT*TILE_WIDTH];
	
		if((yx&1)==0){
			dest+=dx;	
			destXdiff=dx;
			srcXdiff=dx;
					
			if(flags&SPRITE_FLIP_X){
				src+=(TILE_WIDTH-1);
				srcXdiff=((TILE_WIDTH*2)-dx);
			}
		}else{
			destXdiff=(TILE_WIDTH-dx);

			if(flags&SPRITE_FLIP_X){
				srcXdiff=TILE_WIDTH+dx;
				src+=dx;
				src--;
			}else{
				srcXdiff=destXdiff;
				src+=destXdiff;
			}
		}
	

		if((yx&0x0100)==0){
			dest+=(dy*TILE_WIDTH);
			ydiff=dy;
			if(flags&SPRITE_FLIP_Y){
				src+=(TILE_WIDTH*(TILE_HEIGHT-1));
			}
		}else{			
			ydiff=(TILE_HEIGHT-dy);
			if(flags&SPRITE_FLIP_Y){
				src+=((dy-1)*TILE_WIDTH); 
			}else{
				src+=(ydiff*TILE_WIDTH);
			}
		}

		if(flags&SPRITE_FLIP_X){
			step=-1;
		}
		
		if(flags&SPRITE_FLIP_Y){
			srcXdiff-=(TILE_WIDTH*2);
		}

		for(y2=0;y2<(TILE_HEIGHT-ydiff);y2++){
			for(x2=0;x2<(TILE_WIDTH-destXdiff);x2++){
							
				px=pgm_read_byte(src);
				if(px!=TRANSLUCENT_COLOR){
					*dest=px;
				}
				dest++;
				src+=step;
			}		
			src+=srcXdiff;
			dest+=destXdiff;
		}

	}
	*/

	void ProcessSprites(){
	
		unsigned char i,bx,by,dx,dy,bt,x,y,tx=1,ty=1,wx,wy;
		unsigned int ramPtr,ssx,ssy;

		if(!spritesOn) return;

		userRamTilesCount=userRamTilesCount_tmp;
		free_tile_index=userRamTilesCount;
	
		for(i=0;i<MAX_SPRITES;i++){
			bx=sprites[i].x;

			if(bx!=(SCREEN_TILES_H*TILE_WIDTH)){
				//get tile's screen section offsets
				
				#if SCROLLING == 1
					ssx=sprites[i].x+Screen.scrollX;
					ssy=sprites[i].y+Screen.scrollY;
   				#else
					ssx=sprites[i].x;
					ssy=sprites[i].y;
				#endif

				tx=1;
				ty=1;

				//get the BG tiles that are overlapped by the sprite
				bx=ssx>>3;
				dx=ssx&0x7;
				if(dx>0) tx++;

				//by=ssy>>3;
				//dy=ssy&0x7;
				by=ssy/TILE_HEIGHT;
				dy=ssy%TILE_HEIGHT;
				if(dy>0) ty++;			

				for(y=0;y<ty;y++){

					for(x=0;x<tx;x++){
						wy=by+y;
						wx=bx+x;

						//if( (wx-(Screen.scrollX/8))>0 ) {

							//process X-Y wrapping
                            #if SCROLLING == 0
							    if(wy>=(VRAM_TILES_V*2)){
								    wy-=(VRAM_TILES_V*2);
							    }else if(wy>=VRAM_TILES_V){
							    	wy-=VRAM_TILES_V;
							    }
                            #else
                                if(wy>=(Screen.scrollHeight*2)){
								    wy-=(Screen.scrollHeight*2);
							    }else if(wy>=Screen.scrollHeight){
							    	wy-=Screen.scrollHeight;
							    }
                            #endif
							if(wx>=VRAM_TILES_H)wx-=VRAM_TILES_H; //should always be 32

							#if SCROLLING == 0
								ramPtr=(wy*VRAM_TILES_H)+wx;
							#else

								ramPtr=((wy>>3)*256)+(wx*8)+(wy&7);	

							#endif

							bt=vram[ramPtr];						

							if( ((bt>=RAM_TILES_COUNT) | (bt<userRamTilesCount)) && (free_tile_index < RAM_TILES_COUNT) ){ //if no ram free ignore tile
								if( bt>=RAM_TILES_COUNT ){
									//tile is mapped to flash. Copy it to next free RAM tile.
									CopyFlashTile(bt-RAM_TILES_COUNT,free_tile_index);
								}else if(bt<userRamTilesCount){
									//tile is a user ram tile. Copy it to next free RAM tile.
									CopyRamTile(bt,free_tile_index);
								}
								ram_tiles_restore[free_tile_index].addr=(&vram[ramPtr]);
								ram_tiles_restore[free_tile_index].tileIndex=bt;
								vram[ramPtr]=free_tile_index;
								bt=free_tile_index;
								free_tile_index++;
							}
				
							if(bt<RAM_TILES_COUNT){				
								BlitSprite(i,bt,(y<<8)+x,(dy<<8)+dx);						
							}

					//	}

					}//end for X
				}//end for Y
	
			}//	if(bx<(SCREEN_TILES_H*TILE_WIDTH))		
		}


		//restore BG tiles
		RestoreBackground();

	}

	#if SCROLLING == 1
		//Scroll the screen by the relative amount specified (+/-)
		//This function handles screen wrapping on the Y axis if VRAM_TILES_V is less than 32
		void Scroll(char dx,char dy){
		Screen.scrollY+=dy;
		Screen.scrollX+=dx;

		if(Screen.scrollHeight<32){

			if(Screen.scrollY>=(Screen.scrollHeight*TILE_HEIGHT)){
				if(dy>=0){	
					Screen.scrollY=(Screen.scrollY-(Screen.scrollHeight*TILE_HEIGHT));
				}else{
					Screen.scrollY=((Screen.scrollHeight*TILE_HEIGHT)-1)-(0xff-Screen.scrollY);
				}			
			}
	
		}

		}

		//position the scrolling is absolute value
		void SetScrolling(char sx,char sy){

			Screen.scrollX=sx;

			if(Screen.scrollHeight<32){
				if(sy<(Screen.scrollHeight*TILE_HEIGHT)){
					Screen.scrollY=sy;
				}
			}else{
				Screen.scrollY=sy;
			}
		}
	#endif

	
	void MapSprite(unsigned char startSprite,const char *map){
		unsigned char tile;
		unsigned char mapWidth=pgm_read_byte(&(map[0]));
		unsigned char mapHeight=pgm_read_byte(&(map[1]));

		for(unsigned char dy=0;dy<mapHeight;dy++){
			for(unsigned char dx=0;dx<mapWidth;dx++){
		
			 	tile=pgm_read_byte(&(map[(dy*mapWidth)+dx+2]));		
				sprites[startSprite++].tileIndex=tile ;
			}
		}

	}


	void MapSprite2(unsigned char startSprite,const char *map,u8 spriteFlags){      
    
		unsigned char mapWidth=pgm_read_byte(&(map[0]));
		unsigned char mapHeight=pgm_read_byte(&(map[1]));
		s8 x,y,dx,dy,t; 

		if(spriteFlags & SPRITE_FLIP_X){
			x=(mapWidth-1);
			dx=-1;
		}else{
			x=0;
			dx=1;
		}

		if(spriteFlags & SPRITE_FLIP_Y){
			y=(mapHeight-1);
			dy=-1;
		}else{
			y=0;
			dy=1;
		}

		for(u8 cy=0;cy<mapHeight;cy++){
			for(u8 cx=0;cx<mapWidth;cx++){
				t=pgm_read_byte(&(map[(y*mapWidth)+x+2])); 
				sprites[startSprite].tileIndex=t;
				sprites[startSprite++].flags=spriteFlags;
				x+=dx;
			}
			y+=dy;
			x=(spriteFlags & SPRITE_FLIP_X)?(mapWidth-1):0;
	    }
	}


	void MoveSprite(unsigned char startSprite,unsigned char x,unsigned char y,unsigned char width,unsigned char height){

		for(unsigned char dy=0;dy<height;dy++){
			for(unsigned char dx=0;dx<width;dx++){
			
				sprites[startSprite].x=x+(TILE_WIDTH*dx);
			
				#if SCROLLING == 1
					if((Screen.scrollHeight<32) && (y+(TILE_HEIGHT*dy))>(Screen.scrollHeight*TILE_HEIGHT)){
						unsigned char tmp=(y+(TILE_HEIGHT*dy))-(Screen.scrollHeight*TILE_HEIGHT);
						sprites[startSprite].y=tmp;
					}else{
						sprites[startSprite].y=y+(TILE_HEIGHT*dy);
					}
				#else
					if((VRAM_TILES_V<32) && (y+(TILE_HEIGHT*dy))>(VRAM_TILES_V*TILE_HEIGHT)){
						unsigned char tmp=(y+(TILE_HEIGHT*dy))-(VRAM_TILES_V*TILE_HEIGHT);
						sprites[startSprite].y=tmp;
					}else{
						sprites[startSprite].y=y+(TILE_HEIGHT*dy);
					}
				#endif

				startSprite++;
			}
		}	

	}

	//Callback invoked by UzeboxCore.Initialize()
	void DisplayLogo(){
	
		#if INTRO_LOGO !=0
			#define LOGO_X_POS 12
			
			InitMusicPlayer(logoInitPatches);
			SetTileTable(logo_tileset);
			
			//draw logo
			ClearVram();
			WaitVsync(15);		


			#if INTRO_LOGO == 1 
				TriggerFx(0,0xff,true);
			#endif

			DrawMap2(LOGO_X_POS,12,map_uzeboxlogo);
			WaitVsync(3);
			DrawMap2(LOGO_X_POS,12,map_uzeboxlogo2);
			WaitVsync(2);
			DrawMap2(LOGO_X_POS,12,map_uzeboxlogo);

			#if INTRO_LOGO == 2
				SetMasterVolume(0xc0);
				TriggerNote(3,0,16,0xff);
			#endif 
		
			WaitVsync(65);
			ClearVram();
			WaitVsync(20);
		#endif	
	}


	//Callback invoked by UzeboxCore.Initialize()
	void InitializeVideoMode(){

		//disable sprites
		for(int i=0;i<MAX_SPRITES;i++){
			sprites[i].x=(SCREEN_TILES_H*TILE_WIDTH);		
		}
		
		#if SCROLLING == 1
		//	for(int i=0;i<(OVERLAY_LINES*VRAM_TILES_H);i++){
		//		overlay_vram[i]=RAM_TILES_COUNT;
		//	}
			Screen.scrollHeight=VRAM_TILES_V;
			Screen.overlayHeight=0;
		#endif

		//set defaults for main screen section
		/*
		for(i=0;i<SCREEN_SECTIONS_COUNT;i++){
			screenSections[i].scrollX=0;
			screenSections[i].scrollY=0;
		
			if(i==0){
				screenSections[i].height=SCREEN_TILES_V*TILE_HEIGHT;
			}else{
				screenSections[i].height=0;
			}
			screenSections[i].vramBaseAdress=vram;
			screenSections[i].wrapLine=0;
			screenSections[i].flags=SCT_PRIORITY_SPR;
		}
		*/

	}

	//Callback invoked during hsync
	void VideoModeVsync(){
		
		ProcessFading();
		ProcessSprites();

	}
