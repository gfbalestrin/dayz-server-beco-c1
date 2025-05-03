void SpawnVehicleWithParts(PlayerBase player, string vehicleType)
{
    vector spawnOffset = "2 0 2";
    vector pos = player.GetPosition() + spawnOffset;
    pos[1] = GetGame().SurfaceY(pos[0], pos[2]);

    Car vehicle = Car.Cast(GetGame().CreateObject(vehicleType, pos));
    if (!vehicle)
    {
        player.MessageStatus("⚠️ Falha ao spawnar veículo: " + vehicleType);
        return;
    }

    vehicle.SetOrientation("0 0 0");
    vehicle.SetDirection(player.GetDirection());

    vehicle.Fill(CarFluid.FUEL, 1000.0);
    vehicle.Fill(CarFluid.OIL, 1000.0);
    vehicle.Fill(CarFluid.BRAKE, 1000.0);
    vehicle.Fill(CarFluid.COOLANT, 1000.0);
    vehicle.SetHealth("", "", 1000);
    vehicle.SetLifetime(3888000);
    vehicle.SetAffectPathgraph(true, false);

    string battery, plug, wheel;
    ref array<string> parts = new array<string>;
    int wheels = 4;

    switch (vehicleType)
    {
        case "CivilianSedan":
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "CivSedanWheel";
            parts = {
                "CivSedanDoors_Driver",
                "CivSedanDoors_CoDriver",
                "CivSedanDoors_BackLeft",
                "CivSedanDoors_BackRight",
                "CivSedanHood",
                "CivSedanTrunk",
                "CarRadiator",
                "HeadlightH7"
            };
            break;

        case "Sedan_02":
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "Sedan_02_Wheel";
            parts = {
                "Sedan_02_Door_1_1",
                "Sedan_02_Door_1_2",
                "Sedan_02_Door_2_1",
                "Sedan_02_Door_2_2",
                "Sedan_02_Hood",
                "Sedan_02_Trunk",
                "CarRadiator",
                "HeadlightH7"
            };
            break;

        case "OffroadHatchback":
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "HatchbackWheel";
            parts = {
                "Hatchback_02_Door_1_1",
                "Hatchback_02_Door_1_2",
                "Hatchback_02_Door_2_1",
                "Hatchback_02_Door_2_2",
                "Hatchback_02_Hood",
                "Hatchback_02_Trunk",
                "CarRadiator",
                "HeadlightH7"
            };
            break;

        case "Truck_01_Covered":
        case "Truck_01_Open":
            battery = "TruckBattery";
            plug = "GlowPlug";
            wheel = "Truck_01_Wheel";
            wheels = 6;
            parts = {
                "Truck_01_Door_1_1",
                "Truck_01_Door_2_1",
                "Truck_01_Hood"
            };
            break;

        default:
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "CarWheel";
            break;
    }

    vehicle.GetInventory().CreateAttachment(battery);
    vehicle.GetInventory().CreateAttachment(plug);
    for (int i = 0; i < wheels; i++)
        vehicle.GetInventory().CreateAttachment(wheel);

    foreach (string part : parts)
        vehicle.GetInventory().CreateAttachment(part);

    player.MessageStatus("🚗 Veículo spawnado com sucesso: " + vehicleType);

    // Salvar veículo na persistência
    GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SaveVehicle, 1000, false, vehicle);

}

void SaveVehicle(Car vehicle)
{
    if (vehicle)
    {
        vehicle.SetAffectPathgraph(true, false);
        vehicle.SetLifetime(3888000);
    }
}
