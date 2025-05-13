class SafeZoneData {
	string customMessage;
	string regionStr;
    vector areaMin;
    vector areaMax;
    ref array<vector> safeZones;
	ref array<vector> wallZones;

    void SafeZoneData() {
        safeZones = new array<vector>();
		wallZones = new array<vector>();
    }
}