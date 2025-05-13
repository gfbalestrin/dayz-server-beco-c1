bool GiveCustomLoadout(PlayerBase player, string playerId)
{
    string jsonPath = "$mission:custom_loadouts.json";
    ref map<string, ref LoadoutData> loadoutMap = new map<string, ref LoadoutData>;

    // Carregar JSON
    JsonFileLoader<map<string, ref LoadoutData>>.JsonLoadFile(jsonPath, loadoutMap);

    if (!loadoutMap || !loadoutMap.Contains(playerId)) {
        WriteToLog("Nenhum loadout personalizado para o jogador com playerId: " + playerId);
        return false;
    }

    LoadoutData data = loadoutMap.Get(playerId);
    if (!data) {
        WriteToLog("Erro ao obter dados de loadout para: " + playerId);
        return false;
    }

    WriteToLog("Iniciando custom loadout");

    // Itens extras
    if (data.items) {
        foreach (LoadoutItem li : data.items) {
            CreateItemWithSubitems(null, li, player);
        }
    }

    // Armas
    HandleWeaponLoadout(data.weapons, player, playerId);

    // Explosivos
    if (data.explosives) {
        WriteToLog("Criando explosivos...");
        foreach (Explosive explosive : data.explosives) {
            for (int e = 0; e < explosive.quantity; e++) {
                EntityAI ex = player.GetInventory().CreateInInventory(explosive.name_type);
                if (ex)
                    WriteToLog("Criado explosivo: " + explosive.name_type);
                else
                    WriteToLog("Erro ao criar explosivo: " + explosive.name_type);
            }
        }
    }

    WriteToLog("Loadout aplicado com sucesso");
    return true;
}

void HandleWeaponLoadout(Weapons weapons, PlayerBase player, string playerId)
{
    if (!weapons) return;

    if (weapons.primary_weapon)
        HandleWeaponData(weapons.primary_weapon, player, 0, "primary", playerId);

    if (weapons.secondary_weapon)
        HandleWeaponData(weapons.secondary_weapon, player, 1, "secondary", playerId);

    if (weapons.small_weapon)
        HandleWeaponData(weapons.small_weapon, player, 2, "small", playerId);
}

void HandleWeaponData(WeaponData weaponData, PlayerBase player, int quickBarSlot, string label, string playerId)
{
    EntityAI weaponEntity = player.GetInventory().CreateInInventory(weaponData.name_type);

    if (!weaponEntity) {
        WriteToLog("Falha ao criar arma: " + weaponData.name_type);
        return;
    }

    WriteToLog("Criada arma " + label + ": " + weaponData.name_type);

    if (weaponData.attachments) {
        foreach (WeaponAttachment att : weaponData.attachments) {
            EntityAI attEntity = weaponEntity.GetInventory().CreateAttachment(att.name_type);
            if (attEntity) {
                WriteToLog("Anexado: " + att.name_type);
                if (att.battery) {
                    attEntity.GetInventory().CreateAttachment("Battery9V");
                    WriteToLog("Bateria adicionada a: " + att.name_type);
                }
            } else {
                WriteToLog("Falha ao anexar: " + att.name_type);
            }
        }
    }

    if (weaponData.magazine) {
        EntityAI mag = weaponEntity.GetInventory().CreateAttachment(weaponData.magazine.name_type);
        if (mag && weaponData.ammunitions) {
            for (int i = 0; i < weaponData.magazine.capacity; i++) {
                mag.GetInventory().CreateInInventory(weaponData.ammunitions.name_type);
            }
            WriteToLog("Munição adicionada ao carregador: " + weaponData.magazine.name_type);
        } else if (weaponData.ammunitions) {
            player.GetInventory().CreateInInventory(weaponData.ammunitions.name_type);
        }
    }

    player.SetQuickBarEntityShortcut(weaponEntity, quickBarSlot, true);
}

EntityAI CreateItemWithSubitems(EntityAI parent, LoadoutItem itemData, PlayerBase player)
{
    EntityAI item;
    if (parent) {
        WriteToLog("Criando item como attachment: " + itemData.name_type);
        item = parent.GetInventory().CreateAttachment(itemData.name_type);
    } else {
        WriteToLog("Criando item no inventário: " + itemData.name_type);
        item = player.GetInventory().CreateInInventory(itemData.name_type);
    }

    if (!item) {
        WriteToLog("Erro ao criar item: " + itemData.name_type);
        return null;
    }

    if (itemData.subitems) {
        foreach (LoadoutItem sub : itemData.subitems) {
            CreateItemWithSubitems(item, sub, player);
        }
    }

    return item;
}

void GiveDefaultLoadout(PlayerBase player)
{
    // Roupas e Proteção
    player.GetInventory().CreateInInventory("TacticalShirt_Black");
    player.GetInventory().CreateInInventory("CargoPants_Black");
    player.GetInventory().CreateInInventory("CombatBoots_Black");
    player.GetInventory().CreateInInventory("BalaclavaMask_Black");
    player.GetInventory().CreateInInventory("TacticalGloves_Black");
    player.GetInventory().CreateInInventory("TacticalGoggles");
    
     // Anexando os óculos de visão noturna ao capacete
    EntityAI helmet = player.GetInventory().CreateInInventory("Mich2001Helmet");
    EntityAI nvgoggles = helmet.GetInventory().CreateAttachment("NVGoggles");

    // Colocando a bateria no NVGoggles
    if (nvgoggles)
    {
        EntityAI battery = player.GetInventory().CreateInInventory("Battery9V");
        nvgoggles.GetInventory().CreateAttachment("Battery9V");
    }

    // Equipamentos úteis
    player.GetInventory().CreateInInventory("Battery9V");
    player.GetInventory().CreateInInventory("Binoculars");
    player.GetInventory().CreateInInventory("Canteen"); // água

    // Mochila com espaço
    player.GetInventory().CreateInInventory("AliceBag_Camo");
    player.GetInventory().CreateInInventory("StarlightOptic");
    player.GetInventory().CreateInInventory("Rangefinder");    

    // Itens medicos
    player.GetInventory().CreateInInventory("BandageDressing");
    player.GetInventory().CreateInInventory("Morphine");
    player.GetInventory().CreateInInventory("TetracyclineAntibiotics");
    player.GetInventory().CreateInInventory("Painkiller");

    // Arma principal + miras + munição
    EntityAI m4 = player.GetInventory().CreateInInventory("M4A1");
    m4.GetInventory().CreateAttachment("ACOGOptic");
    //m4.GetInventory().CreateAttachment("M4_Suppressor");
    m4.GetInventory().CreateAttachment("M4_RISHndgrd_Black");
    m4.GetInventory().CreateAttachment("M4_MPBttstck_Black");
    m4.GetInventory().CreateAttachment("Mag_STANAG_30Rnd");
    player.SetQuickBarEntityShortcut(m4, 0, true);

    // Munições e carregadores
    for (int i = 0; i < 4; i++)
    {
        EntityAI mag = player.GetInventory().CreateInInventory("Mag_STANAG_30Rnd");
        if (mag)
        {
            Magazine magazineM4 = Magazine.Cast(mag);
            magazineM4.ServerSetAmmoCount(30);
        }
    }    

    // Anexando o carregador à pistola
    EntityAI vest = player.GetInventory().CreateInInventory("PlateCarrierVest");
    vest.GetInventory().CreateAttachment("PlateCarrierHolster");  // Cria o holster no colete
    vest.GetInventory().CreateAttachment("PlateCarrierPouches");  // Cria as bolsas no colete
    vest.GetInventory().CreateAttachment("Glock19");

    // Arma secundária (pistola Glock19)
    EntityAI pistol = vest.GetInventory().CreateInInventory("Glock19");

    EntityAI pistolMag = pistol.GetInventory().CreateAttachment("Mag_Glock_15Rnd");
    if (pistolMag)
    {
        Magazine.Cast(pistolMag).ServerSetAmmoCount(15); // Definindo munição do carregador
    }

    pistol.GetInventory().CreateAttachment("PistolSuppressor");
    pistol.GetInventory().CreateAttachment("UniversalLight");
    player.SetQuickBarEntityShortcut(pistol, 3, true);

    // Equipamento de munição adicional
    player.GetInventory().CreateInInventory("Ammo_9x19");  // Reserva
    player.GetInventory().CreateInInventory("Ammo_556x45"); // Reserva
    player.GetInventory().CreateInInventory("Ammo_308WinTracer"); // Reserva
    player.GetInventory().CreateInInventory("Ammo_762x39"); 

    EntityAI tundra = player.GetInventory().CreateInInventory("Winchester70");
    tundra.GetInventory().CreateAttachment("HuntingOptic");
    tundra.GetInventory().CreateAttachment("ImprovisedSuppressor");
    player.SetQuickBarEntityShortcut(tundra, 2, true);

    EntityAI akm = player.GetInventory().CreateInInventory("AKM"); // ou: player.SpawnEntityOnGroundPos("AKM", akmPos);

    if (akm) {
        // Supressor
        //akm.GetInventory().CreateAttachment("AK_Suppressor");

        // Miras (coloque só uma das opções, ou troque conforme quiser)
        akm.GetInventory().CreateAttachment("KobraOptic");
        // akm.GetInventory().CreateAttachment("PUScopeOptic");

        // Empunhadura (handguard)
        akm.GetInventory().CreateAttachment("AK_WoodHandguard"); // ou: AK_RailHndgrd

        // Coronha (buttstock)
        akm.GetInventory().CreateAttachment("AK_WoodBttstck"); // ou: AK_FoldingBttstck, AK_PlasticBttstck

        // Carregador tambor de 75 balas
        Magazine drumMag = Magazine.Cast(akm.GetInventory().CreateAttachment("Mag_AKM_Drum75Rnd"));
        if (drumMag) {
            drumMag.ServerSetAmmoCount(75); // Enche o tambor
        }

        // Munição extra no inventário do jogador (opcional)
        player.GetInventory().CreateInInventory("Ammo_762x39"); // Caixa de munição extra

        player.SetQuickBarEntityShortcut(akm, 1, true);
    }

    // Cria o cinto
    EntityAI belt = player.GetInventory().CreateInInventory("UtilityBelt");

    // Cria a faca e anexa no slot do cinto
    if (belt)
    {
        belt.GetInventory().CreateAttachment("CombatKnife");
    }
}


void GiveAdminLoadout(PlayerBase player)
{
    player.GetInventory().CreateInInventory("TacticalShirt_Black");
    player.GetInventory().CreateInInventory("CargoPants_Black");
    player.GetInventory().CreateInInventory("MilitaryBoots_Black");
    player.GetInventory().CreateInInventory("Battery9V");
    player.GetInventory().CreateInInventory("PersonalRadio");
    player.GetInventory().CreateInInventory("NVGoggles");
    player.GetInventory().CreateInInventory("Mich2001Helmet");
    player.GetInventory().CreateInInventory("Binoculars");
    player.GetInventory().CreateInInventory("TacticalGloves_Black");
    player.GetInventory().CreateInInventory("TacticalGoggles");
    player.GetInventory().CreateInInventory("BalaclavaMask_Black");
}

// ==== Classes ====

class Explosive {
    string name_type;
    int slots;
    int width;
    int height;
    int quantity;
}

class WeaponAttachment {
    string name_type;
    string type;
    int slots;
    int width;
    int height;
    bool battery;
}

class WeaponMagazine {
    string name_type;
    int capacity;
    int slots;
    int width;
    int height;
}

class WeaponAmmunition {
    string name_type;
    int slots;
    int width;
    int height;
}

class WeaponData {
    string name_type;
    string feed_type;
    int slots;
    int width;
    int height;
    ref WeaponAmmunition ammunitions;
    ref WeaponMagazine magazine;
    ref array<ref WeaponAttachment> attachments;
}

class Weapons {
    ref WeaponData primary_weapon;
    ref WeaponData secondary_weapon;
    ref WeaponData small_weapon;
}

class LoadoutItem {
    string name_type;
    string type_name;
    int slots;
    int width;
    int height;
    int storage_slots;
    int storage_width;
    int storage_height;
    string localization;
    ref array<ref LoadoutItem> subitems;
}

class LoadoutData {
    ref Weapons weapons;
    ref array<ref Explosive> explosives;
    ref array<ref LoadoutItem> items;
}
