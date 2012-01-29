#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cmath>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
const int SCREEN_FLAGS = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ASYNCBLIT;// | SDL_FULLSCREEN;
const int FPSFPS = 10; // rate at which FPS display is updated
const float GRAVITY = 3000.0; //pixels per second per second
const int FPS_CAP = 60; // if FLIP isn't vsynced, limit to this so we don't waste cycles

struct sprite_frame {
    SDL_Rect rect;
    float duration; //s
};

SDL_Surface *load_image( std::string filename ) {
	SDL_Surface *loadedImage = NULL;
	SDL_Surface *optimisedImage = NULL;
	loadedImage = IMG_Load( filename.c_str() );
	if( loadedImage != NULL ) {
		optimisedImage = SDL_DisplayFormatAlpha( loadedImage );
		SDL_FreeSurface( loadedImage );
	}
	return optimisedImage;
}
void apply_sprite( int x, int y, SDL_Surface *source, SDL_Rect *frame, SDL_Surface *destination ) {
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface( source, frame, destination, &offset );
}
void apply_surface( int x, int y, SDL_Surface *source, SDL_Surface *destination ) {
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface( source, NULL, destination, &offset );
}
void clear_surface( SDL_Surface *surf, Uint32 col ) {
	SDL_FillRect( surf, NULL, col );
}
void apply_tiling_surface( int x, int y, int width, int x_offset, SDL_Surface *source, SDL_Surface *destination ) {
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_Rect srcoff;
	srcoff.x = x_offset;
	srcoff.y = 0;
	srcoff.h = source->h;
	//printf( "%i %i\n", x_offset, width );
	if( x_offset + width > source->w ) {
		//printf( "dual\n" );
		srcoff.w = source->w - x_offset;
		SDL_BlitSurface( source, &srcoff, destination, &offset );
		offset.x += srcoff.w;
		srcoff.x = 0;
		srcoff.w = width - srcoff.w;
		SDL_BlitSurface( source, &srcoff, destination, &offset );
	} else {
		srcoff.w = width;
		SDL_BlitSurface( source, &srcoff, destination, &offset );
	}
}

class Sprite {
    public:
	    float x;
	    float y;
        Sprite();
        virtual ~Sprite();
};
Sprite::Sprite() {
    x = 0.0;
    y = 0.0;
}
Sprite::~Sprite() {
    //
}

class NinjaPlayer: public Sprite {
	public:
        SDL_Surface *sprite_sheet;
        struct sprite_frame frames[30];

        static const int run_f_c = 30;
        int run_left[30];
        int run_right[30];

        int *current_animation;

        int frame_count;
        float last_frame_timer;
        float frame_timer;


        float dx;
        float dy;
        float jump_dy; // -900.0// initial jump velocity
        float run_ddx; //3000.0; // p/s^2

        float walkspeed; // p/s^2
        float runspeed; // p/s^2
        float top_speed;

        bool jump_powering;
        int jump_start;
        int jump_power_time; // ms

        NinjaPlayer();
        virtual ~NinjaPlayer();

        void animate( float tdelta );

        void walk();
        void run();
        void jump( int time, int floor );
        void left( float tdelta );
        void right( float tdelta );

        void updateKinematics( float tdelta );

};
NinjaPlayer::NinjaPlayer() {
    frame_count = 0;
    last_frame_timer = 0.0;
    frame_timer = 0.0;

    dx = 0.0;
    dy = 0.0;
    jump_dy = -600.0; // -900.0// initial jump velocity
    run_ddx = 2000.0; //3000.0; // p/s^2

    walkspeed = 600.0; // p/s^2
    runspeed = 1200.0; // p/s^2
    top_speed;

    jump_powering = false;
    jump_start = 0;
    jump_power_time = 150; // ms
	sprite_sheet = load_image( "player_2.png" );
    //frames[0] = { { 0, 0, 80, 80 },  0.05 };
    //frames[1] = { { 80, 0, 80, 80 }, 0.05 };
    //frames[2] = { { 160, 0, 80, 80 }, 0.05 };
    //frames[3] = { { 0, 80, 80, 80 },  0.05 };
    //frames[4] = { { 80, 80, 80, 80 }, 0.05 };
    //frames[5] = { { 160, 80, 80, 80 }, 0.05 };
    int fr_w = 42;
    int fr_h = 50;
    for( int f_c = 0; f_c < run_f_c; f_c ++ ) {
        frames[ f_c ] = { { (unsigned short int)(f_c * fr_w), 0, (unsigned short int)fr_w, (unsigned short int)fr_h }, 0.01 };
        run_left[f_c] = f_c;
        run_right[f_c] = f_c;
    }
    current_animation = run_left;
    top_speed = walkspeed;
}
void NinjaPlayer::animate( float tdelta ) {
    if( dx > 0.0 ) {
        current_animation = run_right;
    } else if( dx < 0.0 ) {
        current_animation = run_left;
    }
    frame_timer += ( ( pow( abs(dx) / runspeed, 0.9 ) ) * tdelta );
    if( frame_timer >= last_frame_timer + frames[ current_animation[ frame_count ] ].duration ) {
        frame_timer = 0.0;
        last_frame_timer = 0.0;
        frame_count += 1;
        if( frame_count > run_f_c - 1 ) {
            frame_count = 0;
        }
    }
}
void NinjaPlayer::updateKinematics( float tdelta ) {
    if( dx < -1.0 * top_speed ) {
        dx = -1.0 * top_speed;
    }
    if( dx > top_speed ) {
        dx = top_speed;
    }
    x = x + tdelta * dx;
    y = y + tdelta * dy;
}
void NinjaPlayer::run() {
    top_speed = runspeed;
}
void NinjaPlayer::walk() {
    top_speed = walkspeed;
}
void NinjaPlayer::jump( int time, int floor ) {
    if( jump_powering == true ) {
        if( time >= jump_start + jump_power_time ) {
            // finish jump power phase
            jump_powering = false;
        } else {
            // keep powering
            dy = jump_dy * ( 1.0 - (time - jump_start ) / jump_power_time );
        }
    } else if( y == floor ) {
        // start jump power phase
        dy = jump_dy;
        jump_powering = true;
        jump_start = time;
    }
}
void NinjaPlayer::left( float tdelta ) {
    dx -= tdelta * run_ddx;
}
void NinjaPlayer::right( float tdelta ) {
    dx += tdelta * run_ddx;
}
NinjaPlayer::~NinjaPlayer() {
	SDL_FreeSurface( sprite_sheet );
}


int main( int argc, char **argv ) {
	//The images
	SDL_Surface *message = NULL;
	SDL_Surface *background = NULL;
	SDL_Surface *screen = NULL;
	SDL_Surface *msg = NULL;
	SDL_Event event;
	bool quit = false;
	int last_time;
	int time = 0;
	std::ostringstream formatter;


	// for FPS
	formatter.precision( 4 );

	TTF_Font *font = NULL;
	SDL_Color textColor = { 255, 255, 255 };

	//Start SDL
	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 ) {
		return 1;
	}
	if( TTF_Init() == -1 ) {
		return 3;
	}
	screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SCREEN_FLAGS );
	if( screen == NULL ) {
		return 2;
	}
	SDL_WM_SetCaption( "Hello World", NULL );

	font = TTF_OpenFont( "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf", 16 );


	background = load_image( "slate_floor.png" );
	int floor = 400;

	float dynamic_friction = 12.00; // 1/s
	float static_friction = 12.0; // p/s^2
	// distances are pixels
	int lc = 0;
	float tdelta = 0;

    NinjaPlayer player = NinjaPlayer();
    player.x = 100.0;
    player.y = (float)floor;

    // init last_time or it goes mental
    last_time = SDL_GetTicks();
	while( !quit ) {

		time = SDL_GetTicks();
		tdelta = (float)((time - last_time)/1000.0);
		last_time = time;
		
		if( SDL_PollEvent( &event ) ) {
			//If a key was pressed
			if( event.type == SDL_KEYDOWN ) {
				//Set the proper message surface
				switch( event.key.keysym.sym ) {
					case SDLK_ESCAPE:
						quit = true;
						break;
				}
			}
			
			if( event.type == SDL_QUIT ) {
				quit = true;
			}
		}
		Uint8 *keystates = SDL_GetKeyState( NULL );
		if( keystates[ SDLK_LCTRL ] ) {
			player.run();
		} else {
			player.walk();
		}
		if( keystates[ SDLK_UP ] ) {
			player.jump( time, floor );
		}
		if( keystates[ SDLK_UP ] ) {
			//y -= 1;
		}
		if( keystates[ SDLK_DOWN ] ) {
			//y += 1;
		}
		if( keystates[ SDLK_LEFT ] ) {
            player.left( tdelta );
		} else if( keystates[ SDLK_RIGHT ] ) {
            player.right( tdelta );
		} else {
			// friction
			if( player.dx < 0.0 || player.dx > 0.0 ) {
				float friction_dir = ( player.dx > 0.0 ? -1.0 : 1.0 );
				int newdx = player.dx + friction_dir * tdelta * ( static_friction + abs(player.dx) * dynamic_friction );
				if( newdx * player.dx < 0.0 ) {
					// sign change - we've gone through 0
					newdx = 0.0;
				}
				player.dx = newdx;
			}
		}

		if( player.y < floor ) {
			player.dy += tdelta * GRAVITY;
		}

        player.updateKinematics( tdelta );
        if( player.y > floor ) {
            player.y = floor;
            player.dy = 0;
        }
        player.animate( tdelta );

		int bg_offset = (int)player.x % background->w;
		clear_surface( screen, 0xffffffff );
		apply_tiling_surface( 0, (int)floor, screen->w, bg_offset, background, screen );
		//apply_surface( 0, (int)floor, background, screen );

        SDL_Rect player_rect = player.frames[ player.current_animation[ player.frame_count ] ].rect;
        //printf( "%i\n", player_current_animation[ player_frame_count ] );

		apply_sprite( 300, (int)(player.y - player_rect.h), player.sprite_sheet, &player_rect, screen );

        // use up remaining ticks before frame is done
        if( tdelta < (1.0/(float)FPS_CAP) ) {
		    SDL_Delay( (int)( ( 1/(float)FPS_CAP - tdelta ) * 1000 ) );
        }
        //SDL_Delay( 1 ); // recommend to smooth things out
		msg = TTF_RenderText_Blended( font, formatter.str().c_str(), textColor );
		apply_surface( 400, 400, msg, screen );

		SDL_Flip( screen );
		if( lc++ % FPSFPS == 0 ) {
			formatter.str( "FPS: " );
			float fps = 1.0 / tdelta;
			//printf( "FPS: %.4f\n", fps );
			formatter << fps;
		}
	}
	//SDL_Delay( 500 );
	SDL_Quit();
	return 0;
}
