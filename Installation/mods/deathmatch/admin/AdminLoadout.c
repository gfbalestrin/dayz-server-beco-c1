
void GiveAdminLoadout(PlayerBase player)
{
    player.GetInventory().CreateInInventory("TacticalShirt_Black");
    player.GetInventory().CreateInInventory("CargoPants_Black");
    player.GetInventory().CreateInInventory("MilitaryBoots_Black");
    player.GetInventory().CreateInInventory("Battery9V");
    player.GetInventory().CreateInInventory("GPSReceiver");
    player.GetInventory().CreateInInventory("PersonalRadio");
    player.GetInventory().CreateInInventory("NVGoggles");
    player.GetInventory().CreateInInventory("Mich2001Helmet");
    player.GetInventory().CreateInInventory("Binoculars");
    player.GetInventory().CreateInInventory("TacticalGloves_Black");
    player.GetInventory().CreateInInventory("TacticalGoggles");
    player.GetInventory().CreateInInventory("BalaclavaMask_Black");
}

void GiveSurvivorLoadout(PlayerBase player)
{
    // Roupas e Proteção
    player.GetInventory().CreateInInventory("TacticalShirt_Black");
    player.GetInventory().CreateInInventory("CargoPants_Black");
    player.GetInventory().CreateInInventory("CombatBoots_Black");
    player.GetInventory().CreateInInventory("PlateCarrierVest"); // Melhor proteção
    player.GetInventory().CreateInInventory("Mich2001Helmet");
    player.GetInventory().CreateInInventory("NVGoggles");
    player.GetInventory().CreateInInventory("BalaclavaMask_Black");
    player.GetInventory().CreateInInventory("TacticalGloves_Black");
    player.GetInventory().CreateInInventory("TacticalGoggles");

    // Equipamentos úteis
    player.GetInventory().CreateInInventory("Battery9V");
    player.GetInventory().CreateInInventory("GPSReceiver");
    player.GetInventory().CreateInInventory("PersonalRadio");
    player.GetInventory().CreateInInventory("Binoculars");
    player.GetInventory().CreateInInventory("Canteen"); // água
    player.GetInventory().CreateInInventory("FieldShovel"); // base building básico

    // Mochila com espaço
    player.GetInventory().CreateInInventory("AssaultBag_Black");

    // Arma principal + miras + munição
    EntityAI weapon = player.GetInventory().CreateInInventory("M4A1");
    weapon.GetInventory().CreateAttachment("ACOGOptic");
    weapon.GetInventory().CreateAttachment("M4_Suppressor");
    weapon.GetInventory().CreateAttachment("M4_RISHndgrd_Black");
    weapon.GetInventory().CreateAttachment("M4_MPBttstck_Black");

    // Munições e carregadores
    for (int i = 0; i < 4; i++)
    {
        EntityAI mag = player.GetInventory().CreateInInventory("Mag_STANAG_30Rnd");
        if (mag)
        {
            Magazine magazine = Magazine.Cast(mag);
            magazine.ServerSetAmmoCount(30);
        }
    }

    // Arma secundária (opcional)
    EntityAI pistol = player.GetInventory().CreateInInventory("Glock19");
    pistol.GetInventory().CreateAttachment("PistolSuppressor");
    pistol.GetInventory().CreateAttachment("UniversalLight");

    EntityAI pistolMag = player.GetInventory().CreateInInventory("Mag_Glock_15Rnd");
    Magazine.Cast(pistolMag).ServerSetAmmoCount(15);

    player.GetInventory().CreateInInventory("Ammo_9x19");  // Reserva
    player.GetInventory().CreateInInventory("Ammo_556x45"); // Reserva
}

