#pragma once

#include "resource.h"

enum class DistributionType {
	UNIFORM,
	NORMAL
};


float Distance(Vector2f, Vector2f);
float Distance(Vector2i, Vector2i);

int RandInt(int, int, DistributionType = DistributionType::UNIFORM);
bool Chance(int);
template<typename T>
inline T RandomFromVector(std::vector<T>& vec) {
	return vec[RandInt(0, vec.size() - 1)];
}


template<typename T>
inline bool Contains(std::vector<T>& vec, T val)
{
	for (T v : vec)
		if (v == val)
			return true;
	return false;
}

template<typename T>
void Print(T msg) {
	std::string message = std::to_string(msg) + "\n";
	OutputDebugStringA(message.c_str());
}

//---------------------------

enum class BIOME {
	PLAINS = RGB(80, 219, 72),
	OCEAN = RGB(45, 155, 235),
	DEEP_OCEAN = RGB(55, 133, 196),
	SNOW = RGB(232, 247, 252),
	DESERT = RGB(245, 240, 144),
	FOREST = RGB(101, 176, 58),
};

struct Hex {
	const float outerRadius = 50, innerRadius = outerRadius * 0.866025404f;
	const Vector2f size = { innerRadius * 2,outerRadius * 1.5f };
	const Vector2f corners[6];
	Vector2i mapPosition;
	//----------
	int height = 0;
	float temperature = 18;
	float humidity = 63;

	BIOME biome = BIOME::OCEAN;
	//----------
	Hex(Vector2i);

	void Draw(Canvas&) const;
	bool isVisible();
	Vector2f GetDisplayPosition();
};

enum class LandmassGenerator {
	RANDOM, CONTINENTS,
};
class GameMap {
private:
	Vector2i size = { 150,150 };
	Hex*** tiles;
	void createMap();

	std::vector<Hex*> visible;
public:
	struct GeneratorSettings {
		int waterLevel = 1;
		int minHeight = 0;
		int maxHeight = 8;

		LandmassGenerator lg = LandmassGenerator::RANDOM;

		int skipWhileSpreadingMaxHeight = 4;
		int skipWhileSpreadingChance = 50;

		float edgeTemperature = -32;
		float equatorTemperature = 40;

		float maxTemperatureDeviation = 2;
		float maxHumidityDeviation = 1;

		float snowMaxTemperature = 0;

		float desertMinTemperature = 37;
		float desertMaxHumidity = 50;

		int forestCount = 50;
		int averageForestSize = 30;
		int forestSizeDeviation = 15;
	};
	GeneratorSettings genPreset;

	Vector2i cameraPosition = { 0,0 };
	float cameraScale = 1;

	GameMap(Vector2i);
	void DrawMap(Canvas&);
	void MoveCamera(Vector2i);
	void GenerateMap(unsigned int);
	Hex* GetClosestTile(Vector2i);
	Hex* GetTile(Vector2i pos) const { return tiles[pos.x][pos.y]; };
	Hex* GetRandomTile(DistributionType type = DistributionType::NORMAL) const {
		return GetTile({ RandInt(0, size.x - 1,type) ,RandInt(0, size.y - 1, type) });
	};
	std::vector<Hex*> GetNeighbouringTiles(Vector2i) const;
	int CountNeighboursOfType(Vector2i, BIOME);

	// GENERATION
	void SpreadHeight(Hex*, int, GeneratorSettings&);
	void CreateTemperatureMap(GeneratorSettings&);
	void CreateHumidityMap(GeneratorSettings&);
	void ContinentGenerator(GeneratorSettings&);
	void SpawnForest(GeneratorSettings&);
};
