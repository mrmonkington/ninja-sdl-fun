#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>
#include <stdarg.h>

#include <Box2D.h>

#include "TmxParser/Tmx.h"

#ifdef DEBUG
inline void printf_debug( const char* f, ... ) {
    va_list argp;
    va_start( argp, f );
    vfprintf( stderr, f, argp );
}
#else
inline void printf_debug( const char* f, ... ) {
}
#endif


const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
const int TW = 32;
const int TH = 32;
const int SCREEN_FLAGS = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ASYNCBLIT;// | SDL_FULLSCREEN;
const int FPSFPS = 10; // rate at which FPS display is updated
const float GRAVITY = 3000.0; //pixels per second per second
const int FPS_CAP = 5; // if FLIP isn't vsynced, limit to this so we don't waste cycles
const float SLOW_DOWN = 1.0;
// pixels per metre
const float SCALE = 25; // pixels per metre

Tmx::Map *map;
std::map<std::string, SDL_Surface*> tilesets;

// SDL Stuff

SDL_Surface *screen = NULL;

// physics stuff

inline int to_screen( float c ) {
    return (int) ( c * SCALE );
}

b2World* world;

// ********** global funcs ************

struct sprite_frame {
    SDL_Rect rect;
    float duration; //s
};
struct animation {
    int index;
    struct sprite_frame *frames;
    int count;
};

SDL_Rect calculate_viewport( int x, int y, int w, int h ) {
    SDL_Rect r;
    if( w < SCREEN_WIDTH || h < SCREEN_HEIGHT ) {
        // centre it
        if( w < SCREEN_WIDTH ) {
            r.x = ( w - SCREEN_WIDTH ) / 2;
        }
        if( h < SCREEN_HEIGHT ) {
            r.y = ( h - SCREEN_HEIGHT ) / 2;
        }
        r.w = w;
        r.h = h;
        return r;
    } else {
        r.w = SCREEN_WIDTH;
        r.h = SCREEN_HEIGHT;
        r.x = x - SCREEN_WIDTH / 2;
        r.y = y - SCREEN_HEIGHT / 2;
        if( r.x < 0 ) {
            r.x = 0;
        }
        if( r.x + r.w > w ) {
            r.x = w - r.w;
        }
        if( r.y < 0 ) {
            r.y = 0;
        }
        if( r.y + r.h > h ) {
            r.y = h - r.h;
        }
        return r;
    }
}

SDL_Surface *load_image( std::string filename ) {
	SDL_Surface *loadedImage = NULL;
	SDL_Surface *optimisedImage = NULL;
	loadedImage = IMG_Load( filename.c_str() );
	if( loadedImage != NULL ) {
        return loadedImage;
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
void apply_tile( int sx, int sy, SDL_Surface *source, int dx, int dy, SDL_Surface *destination ) {
	SDL_Rect soffset;
	SDL_Rect doffset;
	soffset.x = sx;
	soffset.y = sy;
	soffset.w = TW;
	soffset.h = TH;
	doffset.x = dx;
	doffset.y = dy;
	doffset.w = TW;
	doffset.h = TH;
	SDL_BlitSurface( source, &soffset, destination, &doffset );
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
	if( x_offset + width > source->w ) {
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

void load_map() {
    map = new Tmx::Map();
	map->ParseFile("map/platformtest.tmx");

	if (map->HasError()) {
        //printf_debug("error code: %d\n", map->GetErrorCode());
	    //printf_debug("error text: %s\n", map->GetErrorText().c_str());
		system("PAUSE");
        exit(1);
	}

	for (int i = 0; i < map->GetNumTilesets(); ++i) {
		// Get a tileset.
		const Tmx::Tileset *tileset = map->GetTileset(i);
		tilesets[ tileset->GetImage()->GetSource() ] = load_image( ( "map/" + tileset->GetImage()->GetSource() ).c_str() );
	}

}


int tile_is_solid( const Tmx::Tile *tile ) {
    return ( tile->GetProperties().GetLiteralProperty( "solid" ) == "1" );
}

int level_is_solid_here( const Tmx::Layer *layer, int col, int row ) {
    int tile_id = layer->GetTileId( col, row );
    if( tile_id ) {
        const Tmx::Tileset *tileset = map->FindTileset(tile_id);
        const Tmx::Tile *tile = tileset->GetTile(tile_id);
        if( tile ) {
            // not sure why a tileid wouldn't resolve to a tile...
            if( tile_is_solid( tile ) ) {
                return 1;
            }
        }
    }
    return 0;
}

int map_is_solid_here( int col, int row ) {
    for (int i = map->GetNumLayers() - 1; i >= 0; i--) {
        const Tmx::Layer *layer = map->GetLayer(i);
        if( level_is_solid_here( layer, col, row ) ) {
            return 1;
        }
    }
    return 0;
}

const Tmx::Tile *get_tile_by_coords( int x, int y ) {
    int col = x / map->GetTileWidth();
    int row = y / map->GetTileHeight();
    for (int i = map->GetNumLayers() - 1; i >= 0; i--) {
        const Tmx::Layer *layer = map->GetLayer(i);
        if( col < 0 || col >= layer->GetWidth() || row < 0 || row > layer->GetHeight() ) {
            continue;
        }
        int tile_id = layer->GetTileId( col, row );
        if( tile_id > 0 ) {
            const Tmx::Tileset *tileset = map->FindTileset(tile_id);
            const Tmx::Tile *tile = tileset->GetTile(tile_id);
            if( tile ) {
                return tile;
            }
        }
    }
    return NULL;
}


SDL_Surface *init_background() {
    // create surface for background using default bit masks
    return SDL_CreateRGBSurface( SDL_HWSURFACE, map->GetWidth() * TW, map->GetHeight() * TH, 32, 0, 0, 0, 0 );
}

int render_map( int v_x, int v_y, SDL_Surface *destination ) {
                //Tmx::Tile *tile = *(tileset->GetTiles().begin());
                //tile_is_solid( tile )

	// Iterate through the layers.
    // test std::min( 0, 4 );

    for (int y = 0; y < map->GetHeight(); ++y) {
        for (int x = 0; x < map->GetWidth(); ++x) {
            // iterate in reverse so we get top layer first
            for (int i = map->GetNumLayers() - 1; i >= 0; i--) {
                const Tmx::Layer *layer = map->GetLayer(i);
                int tile_id = layer->GetTileId(x, y);
                if( tile_id ) {
                    const Tmx::Tileset *tileset = map->FindTileset(tile_id);
                    const Tmx::Tile *tile = tileset->GetTile(tile_id);
                    if( tile ) {
                        // not sure why a tileid wouldn't resolve to a tile...
                        if( tile_is_solid( tile ) ) {
                            // TODO
                        } else {
                            // TODO
                        }
                    } else {
                        // we have to construct a tile ourselves!
                        // TODO
                    }
                    // This is all very shit - TODO modify TmxParser to calculate this crap for you
                    // TODO margin and spacing compensation
                    int tileset_cols = tileset->GetImage()->GetWidth() / tileset->GetTileWidth();
                    int tileset_rows = tileset->GetImage()->GetHeight() / tileset->GetTileHeight();
                    int tile_index = tile_id;
                    int col = ( tile_index % tileset_cols );
                    int row = ( tile_index / tileset_cols );
                    apply_tile(
                        col * ( tileset->GetSpacing() + tileset->GetTileWidth() ) + tileset->GetMargin(),
                        row * ( tileset->GetSpacing() + tileset->GetTileHeight() ) + tileset->GetMargin(),
                        tilesets[ tileset->GetImage()->GetSource() ], x * TW, y * TH, destination
                    );
                    //we only really care about the top tile for now
                    break;
                } else {
                    //Nuffin 'ere at all
                }
            }
        }
	}

    return 1;
}

std::vector<b2Body*> solids;
int build_map() {
    for (int y = 0; y < map->GetHeight(); ++y) {
        for (int x = 0; x < map->GetWidth(); ++x) {
            // iterate in reverse so we get top layer first
            for (int i = map->GetNumLayers() - 1; i >= 0; i--) {
                const Tmx::Layer *layer = map->GetLayer(i);
                int tile_id = layer->GetTileId(x, y);
                if( tile_id ) {
                    const Tmx::Tileset *tileset = map->FindTileset(tile_id);
                    const Tmx::Tile *tile = tileset->GetTile(tile_id);
                    if( tile ) {
                        // not sure why a tileid wouldn't resolve to a tile...
                        if( tile_is_solid( tile ) ) {
                            b2BodyDef groundBodyDef;
                            groundBodyDef.position.Set(
                                ((float)x + 0.5) * tileset->GetTileWidth() / SCALE,
                                ((float)y + 0.5) * tileset->GetTileHeight() / SCALE
                            );
                            groundBodyDef.userData = (void*)tile;
                            b2Body* groundBody = world->CreateBody(&groundBodyDef);
                            b2PolygonShape groundBox;
                            groundBox.SetAsBox(
                                (float)(tileset->GetTileWidth()) / SCALE / 2,
                                (float)(tileset->GetTileHeight()) / SCALE / 2
                            );

                            groundBody->CreateFixture(&groundBox, 0.0f);
                            solids.push_back( groundBody );
                            

                        } else {
                            // TODO
                        }
                    } else {
                        // we have to construct a tile ourselves!
                        // TODO
                    }
                    break;
                } else {
                    //Nuffin 'ere at all
                }
            }
        }
	}
    for (int i = 0; i < map->GetNumObjectGroups(); i ++) {
        const Tmx::ObjectGroup *group = map->GetObjectGroup(i);
        for (int j = 0; j < group->GetNumObjects(); j ++) {
            const Tmx::Object *ob = group->GetObject(j);
            if( ob->GetType().compare("polyline") == 0 ) {
                const Tmx::Polyline *pl = ob->GetPolyline();
                int x = ob->GetX();
                int y = ob->GetY();
                b2Vec2 * vertices;
                printf_debug( "polyline %i\n", ob->GetPolyline()->GetNumPoints() );
                vertices = new b2Vec2 [ob->GetPolyline()->GetNumPoints()];
                for( int k = 0; k < ob->GetPolyline()->GetNumPoints(); k ++ ) {
                    Tmx::Point p = ob->GetPolyline()->GetPoint( k );
                    vertices[ k ].Set( (float)(p.x)/SCALE, (float)(p.y)/SCALE );
                    printf_debug( "node %f %f\b", (float)(p.x), (float)(p.y) );
                }
                b2ChainShape chain;// = new b2ChainShape();
                chain.CreateChain( vertices, ob->GetPolyline()->GetNumPoints() );
                            b2BodyDef groundBodyDef;
                            groundBodyDef.position.Set( (float)x/SCALE, (float)y/SCALE );
                            b2Body* groundBody = world->CreateBody(&groundBodyDef);
                            groundBody->CreateFixture(&chain, 10.0f);
                            solids.push_back( groundBody );
            }

        }

    }

    return 1;
}

// ************* Sprite classes ******************8

class Sprite {
    public:
	    float x;
	    float y;
        Sprite();
        virtual ~Sprite();
        SDL_Surface *sprite_sheet;
        //struct sprite_frame *frames;
        int current_animation;
        struct animation *animations;
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
        const static int RUN_LEFT = 0;
        const static int RUN_RIGHT = 1;
        const static int STAND_LEFT = 2;
        const static int STAND_RIGHT = 3;

        int frame_count;
        float last_frame_timer;
        float frame_timer;

        //float dx;
        //float dy;
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
        void jump( int time, int floor_left, int floor_right );
        //void jump( int time, struct contact touching );
        void left( float tdelta );
        void right( float tdelta );
        void halt( float tdelta );

        void updateKinematics( float tdelta );
        SDL_Rect getCurrentFrame();
        // find if we're overlapping a tile while travelling

        int xleft() { return (int)x; }
        int xright() { return (int)x+fr_w; }
        int ytop() { return (int)y; }
        int ybottom() { return (int)y+fr_h; }
        int set_xleft( int nx ) { x = (float)nx; }
        int set_xright( int nx ) { x = (float)(nx - fr_w); }
        int set_ytop( int ny ) { y = (float)ny; }
        int set_ybottom( int ny ) { y = (float)( ny - fr_h ); }

        short fr_w;
        short fr_h;

        b2Body* body;
        void setPosition( float x, float y );
        int getScreenX();
        int getScreenY();

        float dx();
        float dy();

};

int NinjaPlayer::getScreenX() {
    return (int)( body->GetPosition().x * SCALE ) - (fr_w/2);
}
int NinjaPlayer::getScreenY() {
    return (int)( body->GetPosition().y * SCALE ) - (fr_h/2);
}


NinjaPlayer::NinjaPlayer() {
    frame_count = 0;
    last_frame_timer = 0.0;
    frame_timer = 0.0;

    fr_w = 42;
    fr_h = 50;

    //dx = 0.0;
    //dy = 0.0;
    jump_dy = -500.0; // -900.0// initial jump velocity
    run_ddx = 4000.0; //3000.0; // p/s^2

    walkspeed = 100.0; // p/s^2
    runspeed = 200.0; // p/s^2

    jump_powering = false;
    jump_start = 0;
    jump_power_time = 150; // ms
	sprite_sheet = load_image( "player_2.png" );

    animations = new animation[4];
    short fr_w = 42;
    short fr_h = 50;
    const struct animation animation_template = {};
    animations[RUN_RIGHT].frames = new struct sprite_frame[30];
    animations[RUN_RIGHT].count = 30;
    for( short f_c = 0; f_c < 30; f_c ++ ) {
        animations[RUN_RIGHT].frames[ f_c ] = { { (short) (f_c * fr_w), 0, fr_w, fr_h }, 0.01 };
    }
    animations[RUN_LEFT].frames = new struct sprite_frame[30];
    animations[RUN_LEFT].count = 30;
    for( short f_c = 0; f_c < 30; f_c ++ ) {
        animations[RUN_LEFT].frames[ f_c ] = { { (short) (f_c * fr_w), fr_h, fr_w, fr_h }, 0.01 };
    }
    animations[STAND_RIGHT].frames = new struct sprite_frame[30];
    animations[STAND_RIGHT].count = 1;
    for( short f_c = 0; f_c < 1; f_c ++ ) {
        animations[STAND_RIGHT].frames[ f_c ] = { { (short) (f_c * fr_w), (short)(fr_h * 2), fr_w, fr_h }, 0.01 };
    }
    animations[STAND_LEFT].frames = new struct sprite_frame[30];
    animations[STAND_LEFT].count = 1;
    for( short f_c = 0; f_c < 1; f_c ++ ) {
        animations[STAND_LEFT].frames[ f_c ] = { { (short) (f_c * fr_w), (short)(fr_h * 3), fr_w, fr_h }, 0.01 };
    }
    current_animation = RUN_LEFT;
    top_speed = runspeed;

    //
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(10.0f, 10.0f);
    bodyDef.fixedRotation = true;
    //bodyDef.linearDamping = 0.9f;
    body = world->CreateBody(&bodyDef);

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox((float)fr_w/SCALE/2.0f, (float)fr_h/SCALE/2.0f);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 100.0f; //300kg/m3
    fixtureDef.friction = 0.5f;
    fixtureDef.restitution = 0.05f;

    body->CreateFixture(&fixtureDef);
}
void NinjaPlayer::setPosition( float x, float y ) {
    body->SetTransform(b2Vec2(x/SCALE, y/SCALE),0.0);
}

NinjaPlayer::~NinjaPlayer() {
    delete animations[RUN_LEFT].frames;
    delete animations[RUN_RIGHT].frames;
    delete animations;
	SDL_FreeSurface( sprite_sheet );
}
// get pixel vel updown
float NinjaPlayer::dy() {
    return body->GetLinearVelocity().y * SCALE;
}
// get pixel vel sideways
float NinjaPlayer::dx() {
    return body->GetLinearVelocity().x * SCALE;
}
void NinjaPlayer::animate( float tdelta ) {
    if( dx() > 0.1 ) {
        current_animation = RUN_RIGHT;
    } else if( dx() < -0.1 ) {
        current_animation = RUN_LEFT;
    } else {
        if( current_animation == RUN_LEFT ) {
            current_animation = STAND_LEFT;
        } else if( current_animation == RUN_RIGHT ) {
            current_animation = STAND_RIGHT;
        }
        // else don't change it
    }
    frame_timer += ( ( pow( abs(dx()) / runspeed / 2, 0.9 ) ) * tdelta );
    if( frame_timer >= last_frame_timer + animations[ current_animation ].frames[ frame_count ].duration ) {
        frame_timer = 0.0;
        last_frame_timer = 0.0;
        frame_count += 1;
        if( frame_count > animations[ current_animation ].count - 1 ) {
            frame_count = 0;
        }
    }
}
void NinjaPlayer::updateKinematics( float tdelta ) {
    if( dx() < -1.0 * top_speed ) {
        //dx() = -1.0 * top_speed;
    }
    if( dx() > top_speed ) {
        //dx() = top_speed;
    }
    x = x + tdelta * dx();
    y = y + tdelta * dy();
}
void NinjaPlayer::run() {
    top_speed = runspeed;
}
void NinjaPlayer::walk() {
    top_speed = walkspeed;
}
SDL_Rect NinjaPlayer::getCurrentFrame() {
    return animations[ current_animation ].frames[ frame_count ].rect;    
}
// floor = last place you were standing
//void NinjaPlayer::jump( int time, int floor_left, int floor_right ) {
/*void NinjaPlayer::jump( int time, struct contact touching ) {
    if( jump_powering == true ) {
        if( time >= jump_start + jump_power_time ) {
            // finish jump power phase
            jump_powering = false;
        } else {
            // keep powering
            dy = jump_dy * ( 1.0 - (time - jump_start ) / jump_power_time );
        }
    //} else if( ybottom() == floor_left || ybottom() == floor_right ) {
    } else if( touching.t2i < 0.1 && touching.t2i > -0.1 && touching.ry == -1.0 ) {
        printf_debug( "jump\n" );
        // start jump power phase
        dy = jump_dy;
        jump_powering = true;
        jump_start = time;
    }
}*/
void NinjaPlayer::left( float tdelta ) {
    b2Vec2 vel = body->GetLinearVelocity();
    float vel_change = -1.0f * top_speed/SCALE - vel.x;
    float impulse = body->GetMass() * vel_change / 10;
    body->ApplyLinearImpulse( b2Vec2( impulse, 0), body->GetWorldCenter() );
}
void NinjaPlayer::right( float tdelta ) {
    b2Vec2 vel = body->GetLinearVelocity();
    float vel_change = top_speed/SCALE - vel.x;
    float impulse = body->GetMass() * vel_change / 10;
    body->ApplyLinearImpulse( b2Vec2( impulse, 0), body->GetWorldCenter() );
}
void NinjaPlayer::halt( float tdelta ) {
    b2Vec2 vel = body->GetLinearVelocity();
    float vel_change = 0.0f - vel.x;
    float impulse = body->GetMass() * vel_change / 10;
    body->ApplyLinearImpulse( b2Vec2( impulse, 0), body->GetWorldCenter() );
}


// ***************** entry point *******************

int main( int argc, char **argv ) {

    b2Vec2 gravity(0.0f, 9.81f);
    bool doSleep = true;
    world = new b2World(gravity, doSleep);

	//The layers
	SDL_Surface *message = NULL;
	SDL_Surface *background = NULL;

	SDL_Surface *msg = NULL;
	SDL_Event event;

	bool quit = false;
	int last_time;
	int time = 0;
	std::ostringstream formatter;

    load_map();

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


    background = init_background();
    render_map( 0, 0, background );
    build_map();

	float dynamic_friction = 12.00; // 1/s
	float static_friction = 12.0; // p/s^2

	// distances are pixels
	int lc = 0;
	float tdelta = 0;

    NinjaPlayer player = NinjaPlayer();
    player.setPosition( 300.0, 200.0 );

    // init last_time or it goes mental
    last_time = SDL_GetTicks();

    int32 velocityIterations = 6;
    int32 positionIterations = 2;

	while( !quit ) {


		time = SDL_GetTicks();
		tdelta = (float)((time - last_time)/1000.0) / SLOW_DOWN;
		last_time = time;

        world->Step(tdelta, velocityIterations, positionIterations);
        printf_debug( "step" );
        b2Vec2 position = player.body->GetPosition();
        float32 angle = player.body->GetAngle();
        printf_debug("%4.2f %4.2f %4.2f\n", position.x, position.y, angle);
        printf_debug("%i %i\n", to_screen(position.x), to_screen(position.y));
		
		if( SDL_PollEvent( &event ) ) {
			if( event.type == SDL_KEYDOWN ) {
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
		//if( keystates[ SDLK_LCTRL ] ) {
		//	player.run();
		//} else {
		//	player.walk();
		//}

        //player.updateKinematics( tdelta );

		if( keystates[ SDLK_UP ] ) {
			//player.jump( time, touching );
		}

        //player.dy += tdelta * GRAVITY;

        
		if( keystates[ SDLK_LEFT ] ) {
            player.left( tdelta );
		} else if( keystates[ SDLK_RIGHT ] ) {
            player.right( tdelta );
		} else {
            // supply a halting impule
            player.halt( tdelta );
        }
        if( keystates[ SDLK_UP ] ) {
            //player.right( tdelta );
            b2Vec2 push( 0.0f, -10000.0f );
            player.body->ApplyForce( push, position );
		}

        player.animate( tdelta );

        SDL_Rect vp = calculate_viewport( player.getScreenX(), player.getScreenY(), map->GetWidth() * map->GetTileWidth(), map->GetHeight() * map->GetTileHeight() );

		clear_surface( screen, 0xffffffff );
		apply_surface( 0-vp.x, 0-vp.y, background, screen );

        SDL_Rect player_rect = player.getCurrentFrame();

		apply_sprite( player.getScreenX() - vp.x, player.getScreenY() -  vp.y, player.sprite_sheet, &player_rect, screen );

        // use up remaining ticks before frame is done
        //if( tdelta < (1.0/(float)FPS_CAP) ) {
		//    SDL_Delay( (int)( ( 1/(float)FPS_CAP - tdelta ) * 1000 ) );
        //}
        SDL_Delay( (int) ( 3 * pow( SLOW_DOWN, 2 ) ) ); // recommend to smooth things out
		//msg = TTF_RenderText_Blended( font, formatter.str().c_str(), textColor );
		//apply_surface( 400, 400, msg, screen );

		SDL_Flip( screen );
		if( lc++ % FPSFPS == 0 ) {
			formatter.str( "FPS: " );
			float fps = 1.0 / tdelta;
			//printf_debug( "FPS: %.4f\n", fps );
			formatter << fps;
		}
	}
	SDL_Quit();
	delete map;
	return 0;
}

