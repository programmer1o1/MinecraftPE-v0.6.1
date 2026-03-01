MODULE := minecraftpe

SRCDIR := ../../src
SHELL  := /bin/bash
CC     := clang++

# ── Dependencies (Homebrew) ─────────────────────────────────────────────────
# Install with:
#   brew install sdl2 libpng zlib
SDL2_CFLAGS  := $(shell sdl2-config --cflags)
SDL2_LIBS    := $(shell sdl2-config --libs)
PNG_CFLAGS   := $(shell pkg-config --cflags libpng 2>/dev/null || echo "-I/opt/homebrew/include")
PNG_LIBS     := $(shell pkg-config --libs   libpng 2>/dev/null || echo "-lpng")

# ── Compiler flags ────────────────────────────────────────────────────────────
CFLAGS := \
	-DMACOS \
	-DPOSIX \
	-DGFX_SMALLER_CHUNKS \
	$(SDL2_CFLAGS) \
	$(PNG_CFLAGS) \
	-I$(SRCDIR) \
	-I$(SRCDIR)/raknet \
	-std=c++11 \
	-O0 -g \
	-Wno-deprecated-declarations \
	-Wno-c++11-extensions \
	-Wno-c++11-narrowing \
	-Wno-reserved-user-defined-literal \
	-Wno-unused-variable \
	-Wno-reorder \
	-Wno-overloaded-virtual \
	-Wno-writable-strings \
	-Wno-invalid-source-encoding \
	-Wno-unused-private-field \
	-Wno-tautological-compare

# ── Linker flags ──────────────────────────────────────────────────────────────
LDFLAGS := \
	$(SDL2_LIBS) \
	$(PNG_LIBS) \
	-framework OpenGL \
	-framework OpenAL \
	-framework Foundation \
	-lpthread \
	-lz

# ── Game source files (mirrors the Raspberry Pi Makefile) ────────────────────
GAME_SOURCES := \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/platform/input/Controller.cpp \
	$(SRCDIR)/platform/input/Keyboard.cpp \
	$(SRCDIR)/platform/input/Mouse.cpp \
	$(SRCDIR)/platform/input/Multitouch.cpp \
	$(SRCDIR)/platform/time.cpp \
	$(SRCDIR)/platform/CThread.cpp \
	$(SRCDIR)/NinecraftApp.cpp \
	$(SRCDIR)/Performance.cpp \
	$(SRCDIR)/SharedConstants.cpp \
	$(SRCDIR)/client/IConfigListener.cpp \
	$(SRCDIR)/client/Minecraft.cpp \
	$(SRCDIR)/client/Options.cpp \
	$(SRCDIR)/client/OptionStrings.cpp \
	$(SRCDIR)/client/OptionsFile.cpp \
	$(SRCDIR)/client/MouseHandler.cpp \
	$(SRCDIR)/client/gamemode/CreatorMode.cpp \
	$(SRCDIR)/client/gamemode/GameMode.cpp \
	$(SRCDIR)/client/gamemode/CreativeMode.cpp \
	$(SRCDIR)/client/gamemode/SurvivalMode.cpp \
	$(SRCDIR)/client/gui/components/Button.cpp \
	$(SRCDIR)/client/gui/components/ImageButton.cpp \
	$(SRCDIR)/client/gui/components/GuiElement.cpp \
	$(SRCDIR)/client/gui/components/GuiElementContainer.cpp \
	$(SRCDIR)/client/gui/components/ItemPane.cpp \
	$(SRCDIR)/client/gui/components/InventoryPane.cpp \
	$(SRCDIR)/client/gui/components/LargeImageButton.cpp \
	$(SRCDIR)/client/gui/components/NinePatch.cpp \
	$(SRCDIR)/client/gui/components/RolledSelectionListH.cpp \
	$(SRCDIR)/client/gui/components/RolledSelectionListV.cpp \
	$(SRCDIR)/client/gui/components/ScrolledSelectionList.cpp \
	$(SRCDIR)/client/gui/components/ScrollingPane.cpp \
	$(SRCDIR)/client/gui/components/SmallButton.cpp \
	$(SRCDIR)/client/gui/components/Slider.cpp \
	$(SRCDIR)/client/gui/components/OptionsItem.cpp \
	$(SRCDIR)/client/gui/components/OptionsGroup.cpp \
	$(SRCDIR)/client/gui/components/OptionsPane.cpp \
	$(SRCDIR)/client/gui/Font.cpp \
	$(SRCDIR)/client/gui/Gui.cpp \
	$(SRCDIR)/client/gui/GuiComponent.cpp \
	$(SRCDIR)/client/gui/Screen.cpp \
	$(SRCDIR)/client/gui/screens/ScreenChooser.cpp \
	$(SRCDIR)/client/gui/screens/ArmorScreen.cpp \
	$(SRCDIR)/client/gui/screens/ChatScreen.cpp \
	$(SRCDIR)/client/gui/screens/ConfirmScreen.cpp \
	$(SRCDIR)/client/gui/screens/ChestScreen.cpp \
	$(SRCDIR)/client/gui/screens/crafting/PaneCraftingScreen.cpp \
	$(SRCDIR)/client/gui/screens/crafting/StonecutterScreen.cpp \
	$(SRCDIR)/client/gui/screens/crafting/WorkbenchScreen.cpp \
	$(SRCDIR)/client/gui/screens/crafting/CraftingFilters.cpp \
	$(SRCDIR)/client/gui/screens/DeathScreen.cpp \
	$(SRCDIR)/client/gui/screens/ChooseLevelScreen.cpp \
	$(SRCDIR)/client/gui/screens/SimpleChooseLevelScreen.cpp \
	$(SRCDIR)/client/gui/screens/FurnaceScreen.cpp \
	$(SRCDIR)/client/gui/screens/InBedScreen.cpp \
	$(SRCDIR)/client/gui/screens/IngameBlockSelectionScreen.cpp \
	$(SRCDIR)/client/gui/screens/JoinGameScreen.cpp \
	$(SRCDIR)/client/gui/screens/OptionsScreen.cpp \
	$(SRCDIR)/client/gui/screens/PauseScreen.cpp \
	$(SRCDIR)/client/gui/screens/ProgressScreen.cpp \
	$(SRCDIR)/client/gui/screens/RenameMPLevelScreen.cpp \
	$(SRCDIR)/client/gui/screens/SelectWorldScreen.cpp \
	$(SRCDIR)/client/gui/screens/StartMenuScreen.cpp \
	$(SRCDIR)/client/gui/screens/TextEditScreen.cpp \
	$(SRCDIR)/client/gui/screens/touch/TouchIngameBlockSelectionScreen.cpp \
	$(SRCDIR)/client/gui/screens/touch/TouchJoinGameScreen.cpp \
	$(SRCDIR)/client/gui/screens/touch/TouchSelectWorldScreen.cpp \
	$(SRCDIR)/client/gui/screens/touch/TouchStartMenuScreen.cpp \
	$(SRCDIR)/client/gui/screens/UploadPhotoScreen.cpp \
	$(SRCDIR)/client/model/ChickenModel.cpp \
	$(SRCDIR)/client/model/CowModel.cpp \
	$(SRCDIR)/client/model/HumanoidModel.cpp \
	$(SRCDIR)/client/model/PigModel.cpp \
	$(SRCDIR)/client/model/QuadrupedModel.cpp \
	$(SRCDIR)/client/model/SheepModel.cpp \
	$(SRCDIR)/client/model/SheepFurModel.cpp \
	$(SRCDIR)/client/model/geom/Cube.cpp \
	$(SRCDIR)/client/model/geom/ModelPart.cpp \
	$(SRCDIR)/client/model/geom/Polygon.cpp \
	$(SRCDIR)/client/particle/Particle.cpp \
	$(SRCDIR)/client/particle/ParticleEngine.cpp \
	$(SRCDIR)/client/player/LocalPlayer.cpp \
	$(SRCDIR)/client/player/RemotePlayer.cpp \
	$(SRCDIR)/client/player/input/KeyboardInput.cpp \
	$(SRCDIR)/client/player/input/touchscreen/TouchscreenInput.cpp \
	$(SRCDIR)/client/renderer/Chunk.cpp \
	$(SRCDIR)/client/renderer/EntityTileRenderer.cpp \
	$(SRCDIR)/client/renderer/GameRenderer.cpp \
	$(SRCDIR)/client/renderer/ItemInHandRenderer.cpp \
	$(SRCDIR)/client/renderer/LevelRenderer.cpp \
	$(SRCDIR)/client/renderer/RenderChunk.cpp \
	$(SRCDIR)/client/renderer/RenderList.cpp \
	$(SRCDIR)/client/renderer/Tesselator.cpp \
	$(SRCDIR)/client/renderer/Textures.cpp \
	$(SRCDIR)/client/renderer/TileRenderer.cpp \
	$(SRCDIR)/client/renderer/gles.cpp \
	$(SRCDIR)/client/renderer/culling/Frustum.cpp \
	$(SRCDIR)/client/renderer/entity/ArrowRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/ChickenRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/EntityRenderDispatcher.cpp \
	$(SRCDIR)/client/renderer/entity/EntityRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/FallingTileRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/HumanoidMobRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/ItemRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/ItemSpriteRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/MobRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/PaintingRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/PlayerRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/SheepRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/TntRenderer.cpp \
	$(SRCDIR)/client/renderer/entity/TripodCameraRenderer.cpp \
	$(SRCDIR)/client/renderer/ptexture/DynamicTexture.cpp \
	$(SRCDIR)/client/renderer/tileentity/ChestRenderer.cpp \
	$(SRCDIR)/client/renderer/tileentity/SignRenderer.cpp \
	$(SRCDIR)/client/renderer/tileentity/TileEntityRenderDispatcher.cpp \
	$(SRCDIR)/client/renderer/tileentity/TileEntityRenderer.cpp \
	$(SRCDIR)/client/sound/Sound.cpp \
	$(SRCDIR)/client/sound/SoundEngine.cpp \
	$(SRCDIR)/platform/audio/SoundSystemAL.cpp \
	$(SRCDIR)/locale/I18n.cpp \
	$(SRCDIR)/nbt/Tag.cpp \
	$(SRCDIR)/network/command/CommandServer.cpp \
	$(SRCDIR)/network/ClientSideNetworkHandler.cpp \
	$(SRCDIR)/network/NetEventCallback.cpp \
	$(SRCDIR)/network/Packet.cpp \
	$(SRCDIR)/network/RakNetInstance.cpp \
	$(SRCDIR)/network/ServerSideNetworkHandler.cpp \
	$(SRCDIR)/server/CreatorLevel.cpp \
	$(SRCDIR)/server/ServerLevel.cpp \
	$(SRCDIR)/server/ServerPlayer.cpp \
	$(SRCDIR)/util/DataIO.cpp \
	$(SRCDIR)/util/Mth.cpp \
	$(SRCDIR)/util/StringUtils.cpp \
	$(SRCDIR)/util/PerfTimer.cpp \
	$(SRCDIR)/util/PerfRenderer.cpp \
	$(SRCDIR)/world/Direction.cpp \
	$(SRCDIR)/world/entity/AgableMob.cpp \
	$(SRCDIR)/world/entity/Entity.cpp \
	$(SRCDIR)/world/entity/EntityFactory.cpp \
	$(SRCDIR)/world/entity/FlyingMob.cpp \
	$(SRCDIR)/world/entity/HangingEntity.cpp \
	$(SRCDIR)/world/entity/Mob.cpp \
	$(SRCDIR)/world/entity/MobCategory.cpp \
	$(SRCDIR)/world/entity/Motive.cpp \
	$(SRCDIR)/world/entity/Painting.cpp \
	$(SRCDIR)/world/entity/PathfinderMob.cpp \
	$(SRCDIR)/world/entity/SynchedEntityData.cpp \
	$(SRCDIR)/world/entity/ai/control/MoveControl.cpp \
	$(SRCDIR)/world/entity/animal/Animal.cpp \
	$(SRCDIR)/world/entity/animal/Chicken.cpp \
	$(SRCDIR)/world/entity/animal/Cow.cpp \
	$(SRCDIR)/world/entity/animal/Pig.cpp \
	$(SRCDIR)/world/entity/animal/Sheep.cpp \
	$(SRCDIR)/world/entity/animal/WaterAnimal.cpp \
	$(SRCDIR)/world/entity/item/FallingTile.cpp \
	$(SRCDIR)/world/entity/item/ItemEntity.cpp \
	$(SRCDIR)/world/entity/item/PrimedTnt.cpp \
	$(SRCDIR)/world/entity/item/TripodCamera.cpp \
	$(SRCDIR)/world/entity/monster/Creeper.cpp \
	$(SRCDIR)/world/entity/monster/Monster.cpp \
	$(SRCDIR)/world/entity/monster/PigZombie.cpp \
	$(SRCDIR)/world/entity/monster/Skeleton.cpp \
	$(SRCDIR)/world/entity/monster/Spider.cpp \
	$(SRCDIR)/world/entity/monster/Zombie.cpp \
	$(SRCDIR)/world/entity/projectile/Arrow.cpp \
	$(SRCDIR)/world/entity/projectile/Throwable.cpp \
	$(SRCDIR)/world/entity/player/Inventory.cpp \
	$(SRCDIR)/world/entity/player/Player.cpp \
	$(SRCDIR)/world/food/SimpleFoodData.cpp \
	$(SRCDIR)/world/inventory/BaseContainerMenu.cpp \
	$(SRCDIR)/world/inventory/ContainerMenu.cpp \
	$(SRCDIR)/world/inventory/FillingContainer.cpp \
	$(SRCDIR)/world/inventory/FurnaceMenu.cpp \
	$(SRCDIR)/world/item/ArmorItem.cpp \
	$(SRCDIR)/world/item/BedItem.cpp \
	$(SRCDIR)/world/item/DyePowderItem.cpp \
	$(SRCDIR)/world/item/Item.cpp \
	$(SRCDIR)/world/item/ItemInstance.cpp \
	$(SRCDIR)/world/item/HangingEntityItem.cpp \
	$(SRCDIR)/world/item/HatchetItem.cpp \
	$(SRCDIR)/world/item/HoeItem.cpp \
	$(SRCDIR)/world/item/PickaxeItem.cpp \
	$(SRCDIR)/world/item/ShovelItem.cpp \
	$(SRCDIR)/world/item/crafting/Recipe.cpp \
	$(SRCDIR)/world/item/crafting/ArmorRecipes.cpp \
	$(SRCDIR)/world/item/crafting/Recipes.cpp \
	$(SRCDIR)/world/item/crafting/FurnaceRecipes.cpp \
	$(SRCDIR)/world/item/crafting/OreRecipes.cpp \
	$(SRCDIR)/world/item/crafting/StructureRecipes.cpp \
	$(SRCDIR)/world/item/crafting/ToolRecipes.cpp \
	$(SRCDIR)/world/item/crafting/WeaponRecipes.cpp \
	$(SRCDIR)/world/level/Explosion.cpp \
	$(SRCDIR)/world/level/Level.cpp \
	$(SRCDIR)/world/level/LightLayer.cpp \
	$(SRCDIR)/world/level/LightUpdate.cpp \
	$(SRCDIR)/world/level/MobSpawner.cpp \
	$(SRCDIR)/world/level/Region.cpp \
	$(SRCDIR)/world/level/TickNextTickData.cpp \
	$(SRCDIR)/world/level/biome/Biome.cpp \
	$(SRCDIR)/world/level/biome/BiomeSource.cpp \
	$(SRCDIR)/world/level/chunk/LevelChunk.cpp \
	$(SRCDIR)/world/level/dimension/Dimension.cpp \
	$(SRCDIR)/world/level/levelgen/CanyonFeature.cpp \
	$(SRCDIR)/world/level/levelgen/DungeonFeature.cpp \
	$(SRCDIR)/world/level/levelgen/LargeCaveFeature.cpp \
	$(SRCDIR)/world/level/levelgen/LargeFeature.cpp \
	$(SRCDIR)/world/level/levelgen/RandomLevelSource.cpp \
	$(SRCDIR)/world/level/levelgen/feature/Feature.cpp \
	$(SRCDIR)/world/level/levelgen/synth/ImprovedNoise.cpp \
	$(SRCDIR)/world/level/levelgen/synth/PerlinNoise.cpp \
	$(SRCDIR)/world/level/levelgen/synth/Synth.cpp \
	$(SRCDIR)/world/level/material/Material.cpp \
	$(SRCDIR)/world/level/pathfinder/Path.cpp \
	$(SRCDIR)/world/level/storage/ExternalFileLevelStorage.cpp \
	$(SRCDIR)/world/level/storage/ExternalFileLevelStorageSource.cpp \
	$(SRCDIR)/world/level/storage/FolderMethods.cpp \
	$(SRCDIR)/world/level/storage/LevelData.cpp \
	$(SRCDIR)/world/level/storage/LevelStorageSource.cpp \
	$(SRCDIR)/world/level/storage/RegionFile.cpp \
	$(SRCDIR)/world/level/tile/BedTile.cpp \
	$(SRCDIR)/world/level/tile/ChestTile.cpp \
	$(SRCDIR)/world/level/tile/CropTile.cpp \
	$(SRCDIR)/world/level/tile/DoorTile.cpp \
	$(SRCDIR)/world/level/tile/EntityTile.cpp \
	$(SRCDIR)/world/level/tile/FurnaceTile.cpp \
	$(SRCDIR)/world/level/tile/GrassTile.cpp \
	$(SRCDIR)/world/level/tile/HeavyTile.cpp \
	$(SRCDIR)/world/level/tile/LightGemTile.cpp \
	$(SRCDIR)/world/level/tile/MelonTile.cpp \
	$(SRCDIR)/world/level/tile/Mushroom.cpp \
	$(SRCDIR)/world/level/tile/NetherReactor.cpp \
	$(SRCDIR)/world/level/tile/NetherReactorPattern.cpp \
	$(SRCDIR)/world/level/tile/StairTile.cpp \
	$(SRCDIR)/world/level/tile/StemTile.cpp \
	$(SRCDIR)/world/level/tile/StoneSlabTile.cpp \
	$(SRCDIR)/world/level/tile/TallGrass.cpp \
	$(SRCDIR)/world/level/tile/Tile.cpp \
	$(SRCDIR)/world/level/tile/TrapDoorTile.cpp \
	$(SRCDIR)/world/level/tile/entity/ChestTileEntity.cpp \
	$(SRCDIR)/world/level/tile/entity/NetherReactorTileEntity.cpp \
	$(SRCDIR)/world/level/tile/entity/SignTileEntity.cpp \
	$(SRCDIR)/world/level/tile/entity/TileEntity.cpp \
	$(SRCDIR)/world/level/tile/entity/FurnaceTileEntity.cpp \
	$(SRCDIR)/world/phys/HitResult.cpp

# ── RakNet — compiled from source (no prebuilt macOS library exists) ─────────
# Exclude Windows-only and Vita-only files that won't compile on macOS.
RAKNET_ALL := $(wildcard $(SRCDIR)/raknet/*.cpp)
RAKNET_EXCLUDE := \
	$(SRCDIR)/raknet/gettimeofday.cpp \
	$(SRCDIR)/raknet/VitaIncludes.cpp
RAKNET_SOURCES := $(filter-out $(RAKNET_EXCLUDE), $(RAKNET_ALL))

# Objective-C++ sources (compiled with clang as ObjC++)
OBJCXX_SOURCES := \
	$(SRCDIR)/world/level/storage/MoveFolder.mm

SOURCES := $(GAME_SOURCES) $(RAKNET_SOURCES)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp, obj/%.o, $(SOURCES)) \
           $(patsubst $(SRCDIR)/%.mm, obj/%.o, $(OBJCXX_SOURCES))

# ── Targets ───────────────────────────────────────────────────────────────────
.PHONY: all clean

all: $(MODULE)

$(MODULE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

obj/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

# Objective-C++ rule
obj/%.o: $(SRCDIR)/%.mm
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) -ObjC++ $< -o $@

clean:
	rm -rf obj $(MODULE)
