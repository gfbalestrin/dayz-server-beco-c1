
void SpawnVehicleWithParts(PlayerBase player, string vehicleType)
{
    vector pos = player.GetPosition() + "2 0 2";
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

    string battery, plug, wheel;
    int wheels = 4;

    switch (vehicleType)
    {
        case "CivilianSedan":
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "CivSedanWheel";
            break;
        case "OffroadHatchback":
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "HatchbackWheel";
            break;
        case "Sedan_02":
            battery = "CarBattery";
            plug = "SparkPlug";
            wheel = "Sedan_02_Wheel";
            break;
        case "Truck_01_Covered":
        case "Truck_01_Open":
            battery = "TruckBattery";
            plug = "GlowPlug";
            wheel = "Truck_01_Wheel";
            wheels = 6;
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

    player.MessageStatus("🚗 Veículo spawnado com sucesso: " + vehicleType);
}
