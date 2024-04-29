// winciv.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "winciv.h"

Window* win;
GameMap* map;
// HEX

Hex::Hex(Vector2i mapPosition) : mapPosition(mapPosition), corners{ {0,outerRadius},{innerRadius,.5f * outerRadius},{innerRadius,-.5f * outerRadius},{0,-outerRadius},{-innerRadius,-.5f * outerRadius},{-innerRadius,.5f * outerRadius} } {}

void Hex::Draw(Canvas& c) const {
	bool isOddRow = false;
	Vector2f move = { mapPosition.y % 2 == 1 ? innerRadius : 0 ,0 };

	POINT points[6]{};
	for (int i = 0; i < 6; i++) {
		Vector2f pointPosition = (mapPosition * size + corners[i] + move + map->cameraPosition) * map->cameraScale;
		points[i] = { (long)pointPosition.x,(long)pointPosition.y };
	}

	HBRUSH hBrush = CreateSolidBrush((int)biome);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(c.hdc_, hBrush);
	Polygon(c.hdc_, points, 6);
	SelectObject(c.hdc_, hOldBrush);
	DeleteObject(hBrush);
}

bool Hex::isVisible()
{
	Vector2f realPosition = GetDisplayPosition();
	float rInnerRadius = innerRadius * map->cameraScale;
	float rOuterRadius = outerRadius * map->cameraScale;
	return (realPosition.x >= -rInnerRadius && realPosition.y >= -rOuterRadius && realPosition.x <= win->getSize().x + rInnerRadius && realPosition.y <= win->getSize().y + rOuterRadius);
}

Vector2f Hex::GetDisplayPosition()
{
	Vector2f move = { mapPosition.y % 2 == 1 ? innerRadius : 0 ,0 };
	return (mapPosition * size + move + map->cameraPosition) * map->cameraScale;
}

// MAP

GameMap::GameMap(Vector2i size) : size(size) {
	createMap();
}

void GameMap::createMap() {
	tiles = new Hex * *[size.x];
	for (int x = 0; x < size.x; ++x) {
		tiles[x] = new Hex * [size.y];
		for (int y = 0; y < size.y; ++y) {
			tiles[x][y] = new Hex({ x, y });
		}
	}
}

void GameMap::DrawMap(Canvas& c) {
	visible.clear();
	for (int x = 0; x < size.x; ++x)
		for (int y = 0; y < size.y; ++y) {
			Hex* current = tiles[x][y];
			if (current->isVisible())
				visible.push_back(current);
		}

	for (Hex* hex : visible)
		hex->Draw(c);
}

void GameMap::MoveCamera(Vector2i moveVector)
{
	cameraPosition += moveVector;
}

void GameMap::GenerateMap(unsigned int seed)
{
	CreateTemperatureMap(genPreset);
	CreateHumidityMap(genPreset);

	switch (genPreset.lg)
	{
	case LandmassGenerator::CONTINENTS:
		ContinentGenerator(genPreset);
		break;
	case LandmassGenerator::RANDOM:
	default:
		for (int i = 0; i < size.x; i++) {
			Vector2i randomLandmassPosition = { RandInt(0, size.x - 1) ,RandInt(0, size.y - 1, DistributionType::NORMAL) };
			SpreadHeight(GetTile(randomLandmassPosition), RandInt(genPreset.waterLevel + 1, genPreset.maxHeight), genPreset);
		}
	}

	for (int x = 0; x < size.x; ++x)
		for (int y = 0; y < size.y; ++y) {
			Hex* current = tiles[x][y];
			BIOME biome = BIOME::PLAINS;
			if (current->height <= genPreset.waterLevel)biome = BIOME::OCEAN;

			if (biome == BIOME::PLAINS) {
				if (current->temperature <= genPreset.snowMaxTemperature) biome = BIOME::SNOW;
				if (current->temperature >= genPreset.desertMinTemperature &&
					current->humidity <= genPreset.desertMaxHumidity) biome = BIOME::DESERT;
			}

			current->biome = biome;
		}

	for (int x = 0; x < size.x; ++x)
		for (int y = 0; y < size.y; ++y) {
			Hex* current = tiles[x][y];
			if (CountNeighboursOfType({ x,y }, BIOME::OCEAN) + CountNeighboursOfType({ x,y }, BIOME::DEEP_OCEAN) >= GetNeighbouringTiles({ x,y }).size())
				current->biome = BIOME::DEEP_OCEAN;
			if (CountNeighboursOfType({ x,y }, BIOME::DESERT) >= 5)
				current->biome = BIOME::DESERT;
		}
}

Hex* GameMap::GetClosestTile(Vector2i position)
{
	for (Hex* hex : visible)
		if (Distance({ (float)position.x,(float)position.y }, hex->GetDisplayPosition()) <= hex->outerRadius * cameraScale)
			return hex;
	return nullptr;
}

std::vector<Hex*> GameMap::GetNeighbouringTiles(Vector2i pos) const
{
	std::vector<Hex*> neighbours;

	std::vector<Vector2i> offsets_y0 = { {-1,0},{1,0},{0,1},{0, -1},{1,1},{1,-1} };
	std::vector<Vector2i> offsets_y1 = { {-1,0},{1,0},{0,1},{0, -1},{-1,1},{-1,-1} };

	for (Vector2i offset : pos.y % 2 ? offsets_y0 : offsets_y1) {
		Vector2i neighbourPos = pos + offset;
		if (neighbourPos.x >= 0 && neighbourPos.y >= 0 && neighbourPos.x < size.x && neighbourPos.y < size.y)
			neighbours.push_back(tiles[neighbourPos.x][neighbourPos.y]);
	}

	return neighbours;
}

int GameMap::CountNeighboursOfType(Vector2i pos, BIOME biome)
{
	int count = 0;
	for (Hex* h : GetNeighbouringTiles(pos))
		if (h->biome == biome) count++;
	return count;
}

void GameMap::SpreadHeight(Hex* origin, int val, GeneratorSettings& preset)
{
	if (!origin) return;
	std::vector<Hex*> visited;
	std::vector<Hex*> currentRing = { origin };
	while (val > preset.minHeight) {
		std::vector<Hex*> newRing;
		for (Hex* tile : currentRing) {
			if (tile == nullptr) continue;
			visited.push_back(tile);
			if (tile->height < val)
				tile->height = val;
			Vector2i tilePos = tile->mapPosition;
			for (Hex* neighbour : GetNeighbouringTiles(tilePos)) {
				if (Contains(visited, neighbour)) continue;
				if (val <= preset.skipWhileSpreadingMaxHeight && Chance(preset.skipWhileSpreadingChance)) continue;
				newRing.push_back(neighbour);
			}
		}
		val--;

		currentRing = newRing;
	}
}

void GameMap::CreateTemperatureMap(GeneratorSettings& preset)
{
	float a = (preset.edgeTemperature - preset.equatorTemperature) / -pow(size.x / 2.0f, 2);
	for (int x = 0; x < size.x; ++x)
		for (int y = 0; y < size.y; ++y) {
			Hex* current = tiles[x][y];
			float expectedTemperature = -a * (y - size.x) * y + preset.edgeTemperature + RandInt(-preset.maxTemperatureDeviation, preset.maxTemperatureDeviation);
			current->temperature = expectedTemperature;
		}
}

void GameMap::CreateHumidityMap(GeneratorSettings& preset)
{
	float a = 0.04f;
	for (int x = 0; x < size.x; ++x)
		for (int y = 0; y < size.y; ++y) {
			Hex* current = tiles[x][y];
			float defaultHumidity = a * (y - 100) * y + 100 + RandInt(0, preset.maxHumidityDeviation);
			float humidityMultiplier = abs(50 * sin(1 / (1 * 3.1415f) * x) + 50 * cos(1 / (4 * 3.1415f) * x));
			current->humidity = clamp(defaultHumidity * humidityMultiplier, 0.0f, 100.0f);
		}
}

void GameMap::ContinentGenerator(GeneratorSettings& preset)
{
	int numberOfContinents = 5;

	// Determine Continent Positions
	float minDistenceBetweenContinents = size.x / numberOfContinents;
	std::vector<Hex*> continents;
	while (numberOfContinents > 0) {

		Hex* tile = GetTile({ RandInt(0, size.x - 1) ,RandInt(size.y * 0.1f, size.y - size.y * 0.1f) });
		bool canPlace = true;

		for (Hex* h : continents)
			if (Distance(tile->mapPosition, h->mapPosition) <= minDistenceBetweenContinents) { canPlace = false; break; }
		if (!canPlace) continue;

		tile->height = 5;
		continents.push_back(tile);
		numberOfContinents--;
	}

	for (Hex* c : continents) {
		SpreadHeight(c, 10, preset);
	}
}

// UTIL

float Distance(Vector2f a, Vector2f b)
{
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

float Distance(Vector2i a, Vector2i b)
{
	return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

int RandInt(int min, int max, DistributionType distType) {
	std::random_device rd;
	std::mt19937 eng(rd());

	switch (distType) {
	case DistributionType::UNIFORM: {
		std::uniform_int_distribution<> distr(min, max);
		return distr(eng);
	}
	case DistributionType::NORMAL: {
		std::normal_distribution<> distr((min + max) / 2.0, (max - min) / 6.0);
		return static_cast<int>(distr(eng));
	}
	default:
		throw std::invalid_argument("Invalid distribution type");
	}
}

bool Chance(int prc)
{
	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<> distr(0, 100);
	int rand_num = distr(eng);
	return (rand_num < prc);
}

// MAIN



Vector2i lastMousePosition = { 0,0 };
static void Update(HDC& hdc) {
	Canvas canvas(hdc);

	Vector2i currentMousePosition = { win->inputState.mouseX,win->inputState.mouseY };
	if (win->inputState.leftButtonDown)
		map->MoveCamera(currentMousePosition - lastMousePosition);
	if (win->inputState.rightButtonDown) {
		Hex* clicked = map->GetClosestTile(currentMousePosition);
		if (clicked) {
			//clicked->biome = BIOME::PLAINS;
			Print(clicked->temperature);
		}
	}

	map->cameraScale = clamp(map->cameraScale + (win->inputState.scrollDirection / static_cast<float>(10)), .1f, 1.5f);

	lastMousePosition = currentMousePosition;
	map->DrawMap(canvas);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	win = new Window(hInstance, nCmdShow);
	map = new GameMap({ 100,100 });
	map->GenerateMap(2137);

	Window::RegisterUpdateFunction(30, Update);
	win->CreateWindowAndRun();

	return 0;
}
