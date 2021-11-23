#pragma once

#include <string>



namespace elona
{

struct Character;



void map_initialize();
void map_init_static_map(const std::string&);
void map_tileset(int = 0);
void map_nextdir1(int = 0, int = 0);
void map_nextdir2(int = 0, int = 0);


enum class ArenaCharaType
{
    allies,
    monsters,
};

void map_place_chara_on_pet_arena(Character& chara, ArenaCharaType chara_type);

void map_place_player_and_allies();

/// Replace random tiles by @a tile_id.
/// @param tile_id Tile ID. Some of tiles in the current map will be replaced by
/// the tile.
/// @param density Density in percentage
void map_replace_random_tiles(int tile_id, int density);

void map_set_fog();

void map_place_upstairs(int x, int y);
void map_place_downstairs(int x, int y);

void map_generate_debug_map();
void generate_random_nefia();
int initialize_quest_map_crop();
int initialize_random_nefia_rdtype1();
int initialize_random_nefia_rdtype4();
int initialize_random_nefia_rdtype5();
int initialize_random_nefia_rdtype2();
int initialize_random_nefia_rdtype3();
int initialize_quest_map_party();
void initialize_home_mdata();

// Functions called from `init.cpp`.
void map_init_cell_object_data();



enum class FieldMapType
{
    plain_field,
    forest,
    sea,
    grassland,
    desert,
    snow_field,
};

FieldMapType map_get_field_type();



int map_barrel(int = 0, int = 0);
int map_connectroom();
int map_createroom(int = 0);
int map_digcheck(int = 0, int = 0);
int map_placedownstairs(int = 0, int = 0);
int map_placeupstairs(int = 0, int = 0);
int map_trap(int = 0, int = 0, int = 0, int = 0);
int map_web(int = 0, int = 0, int = 0);
void initialize_cell_object_data();
void initialize_random_nefia_rdtype6();
void initialize_quest_map_town();
void initialize_random_nefia_rdtype8();
void initialize_random_nefia_rdtype9();
void mapgen_dig_maze();
void initialize_random_nefia_rdtype10();

} // namespace elona
