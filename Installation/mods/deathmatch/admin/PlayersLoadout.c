bool GiveCustomLoadout(PlayerBase player, string playerId)
{
    string jsonPath = "$mission:custom_loadouts.json";
    ref map<string, ref LoadoutData> loadoutMap;

    JsonFileLoader<map<string, ref LoadoutData>>.JsonLoadFile(jsonPath, loadoutMap);
    if (!loadoutMap || !loadoutMap.Contains(playerId)) {
        WriteToLog("Nenhum loadout personalizado para o jogador com playerId: " + playerId, LogFile.INIT, false, LogType.INFO);
        return false;
    }    

    LoadoutData data = loadoutMap.Get(playerId);
    if (!data) {
        WriteToLog("Erro ao obter dados de loadout para: " + playerId, LogFile.INIT, false, LogType.ERROR);
        return false;
    }

    WriteToLog("Iniciando custom loadout para player " + playerId, LogFile.INIT, false, LogType.INFO);

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
        WriteToLog("Criando explosivos...", LogFile.INIT, false, LogType.INFO);
        foreach (Explosive explosive : data.explosives) {
            for (int e = 0; e < explosive.quantity; e++) {
                EntityAI ex = player.GetInventory().CreateInInventory(explosive.name_type);
                if (ex)
                    WriteToLog("Criado explosivo: " + explosive.name_type, LogFile.INIT, false, LogType.INFO);
                else
                    WriteToLog("Erro ao criar explosivo: " + explosive.name_type, LogFile.INIT, false, LogType.ERROR);
            }
        }
    }

    WriteToLog("Loadout aplicado com sucesso", LogFile.INIT, false, LogType.INFO);
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
    bool possuiAttachments = false;
    bool possuiMagazine = false;
    bool possuiAmmo = false;

    EntityAI weaponEntity; 
    if (label == "primary")
        weaponEntity = player.GetHumanInventory().CreateInHands(weaponData.name_type);
    else
        weaponEntity = player.GetInventory().CreateInInventory(weaponData.name_type);

    if (!weaponEntity) {
        WriteToLog("Falha ao criar arma: " + weaponData.name_type, LogFile.INIT, false, LogType.ERROR);
        return;
    }

    player.SetQuickBarEntityShortcut(weaponEntity, quickBarSlot, true);

    WriteToLog("Criada arma " + label + ": " + weaponData.name_type, LogFile.INIT, false, LogType.INFO);
    if (weaponData.attachments) {
        if (weaponData.attachments.Count() > 0)
            possuiAttachments = true;
    }
    if (weaponData.magazine) {
        if (weaponData.magazine.name_type != "")
            possuiMagazine = true;
    }
    if (weaponData.ammunitions) {
        if (weaponData.ammunitions.name_type != "")
            possuiAmmo = true;
    }

    if (possuiAttachments) {
        foreach (WeaponAttachment att : weaponData.attachments) {
            if (!att || att.name_type == "") continue;

            EntityAI attEntity = weaponEntity.GetInventory().CreateAttachment(att.name_type);
            if (attEntity) {
                WriteToLog("Anexado: " + att.name_type, LogFile.INIT, false, LogType.INFO);
                if (att.battery) {
                    EntityAI battery = attEntity.GetInventory().CreateAttachment("Battery9V");
                    if (battery)
                        WriteToLog("Bateria adicionada a: " + att.name_type, LogFile.INIT, false, LogType.INFO);
                    else
                        WriteToLog("Falha ao adicionar bateria à: " + att.name_type, LogFile.INIT, false, LogType.INFO);
                }
            } else {
                WriteToLog("Falha ao anexar: " + att.name_type, LogFile.INIT, false, LogType.ERROR);
                WriteToLog("Tentando criar no inventário do jogador...", LogFile.INIT, false, LogType.INFO);
                EntityAI attEntity2 = player.GetInventory().CreateInInventory(att.name_type);
                if (attEntity2) {
                    WriteToLog("Criado no inventário: " + att.name_type, LogFile.INIT, false, LogType.INFO);
                    if (att.battery) {
                        EntityAI battery2 = attEntity2.GetInventory().CreateAttachment("Battery9V");
                        if (battery2)
                            WriteToLog("Bateria adicionada a: " + att.name_type, LogFile.INIT, false, LogType.INFO);
                        else
                            WriteToLog("Falha ao adicionar bateria à: " + att.name_type, LogFile.INIT, false, LogType.INFO);
                    }
                } else {
                    WriteToLog("Falha ao anexar: " + att.name_type, LogFile.INIT, false, LogType.ERROR);
                }
            }
        }
    }

    Weapon_Base weapon_base = Weapon_Base.Cast(weaponEntity);
    if (!weapon_base) {
        WriteToLog("Falha no cast de Weapon_Base para: " + weaponData.name_type, LogFile.INIT, false, LogType.ERROR);
        return;
    }

    if (possuiMagazine) {
        Magazine mag = weapon_base.SpawnAttachedMagazine(weaponData.magazine.name_type);
        if (!mag) {
            WriteToLog("Falha ao anexar pente " + weaponData.magazine.name_type + " para arma: " + weaponData.name_type, LogFile.INIT, false, LogType.ERROR);
            return;
        }
        
        // FillInnerMagazine	(	string 	ammoType = "",
        // int 	flags = WeaponWithAmmoFlags.CHAMBER 
        // )	

        
        // FillChamber	(	string 	ammoType = "",
        // int 	flags = WeaponWithAmmoFlags.CHAMBER 
        // )		

        //ForceSyncSelectionState

        if (possuiAmmo)
        {
            // Funciona mas munição aleatória
            int amountAmmo = mag.GetAmmoMax() - 1;
            if (amountAmmo > 0) {
                mag.LocalSetAmmoCount(amountAmmo);	
                mag.ServerSetAmmoCount(amountAmmo);
                WriteToLog("Pente carregado com " + amountAmmo.ToString() + " munições.", LogFile.INIT, false, LogType.INFO);
            }

            // // Não funcionou
            // mag.LocalSetAmmoCount(0);	
            // mag.ServerSetAmmoCount(0);            
            // weapon_base.FillInnerMagazine(weaponData.ammunitions.name_type, WeaponWithAmmoFlags.MAX_CAPACITY_MAG);

        }        
    } else if (weaponData.feed_type == "manual" && possuiAmmo) {
        // Shotguns, revolvers
        WriteToLog("Arma sem suporte a pente. Tentando criar munição no chamber... ", LogFile.INIT, false, LogType.INFO);
        // Funciona mas munição aleatória
        //weapon_base.SpawnAmmo("", WeaponWithAmmoFlags.CHAMBER);	

        int muzzCount = weapon_base.GetMuzzleCount();
        WriteToLog("Quantidade suportada no chamber: " + muzzCount, LogFile.INIT, false, LogType.INFO);
        for (int imuzzCount = 0; imuzzCount < muzzCount; ++imuzzCount)
        {   
            WriteToLog("Inserindo municao " + weaponData.ammunitions.name_type + " no chamber... " + imuzzCount, LogFile.INIT, false, LogType.INFO);
            weapon_base.FillChamber(weaponData.ammunitions.name_type);
        }   

    } else if (weaponData.feed_type == "internal" && possuiAmmo) {
        WriteToLog("Arma sem suporte a pente. Tentando criar munição no pente interno... ", LogFile.INIT, false, LogType.INFO);
        // Funciona mas munição aleatória
        //weapon_base.SpawnAmmo("", WeaponWithAmmoFlags.CHAMBER);	

        weapon_base.FillInnerMagazine(weaponData.ammunitions.name_type);
    } else if (possuiAmmo)
    {
        WriteToLog("O tipo de alimentação da arma não foi identificado. Tentando criar munição com o método SpawnAmmo... ", LogFile.INIT, false, LogType.INFO);
        weapon_base.SpawnAmmo(weaponData.ammunitions.name_type, WeaponWithAmmoFlags.CHAMBER);
    }

    // Extra cria no inventário enquanto não tem customização
    if (possuiMagazine)
    {        
        int qtdMagazineExtra = 3;
        if (weaponData.magazine.slots > 4)
            qtdMagazineExtra = 1;

        WriteToLog("Criando pentes extras...", LogFile.INIT, false, LogType.INFO);
        for (int magExtraI = 0; magExtraI < qtdMagazineExtra; magExtraI++) {
            EntityAI magExtra = player.GetInventory().CreateInInventory(weaponData.magazine.name_type);
            if (!magExtra)
            {
                WriteToLog("Erro ao criar pente extra!", LogFile.INIT, false, LogType.ERROR);
                break;
            }
            Magazine magExtraCast = Magazine.Cast(magExtra);
            if (!magExtraCast)
            {
                WriteToLog("Erro ao criar pente extra!", LogFile.INIT, false, LogType.ERROR);
                break;
            }
            magExtraCast.ServerSetAmmoCount(weaponData.magazine.capacity);
            WriteToLog("Pente extra criado e carregado!", LogFile.INIT, false, LogType.INFO);
        }
    }
    if (possuiAmmo)
    {
        for (int ammoExtraI = 0; ammoExtraI < 5; ammoExtraI++) {
            EntityAI ammoExtra = player.GetInventory().CreateInInventory(weaponData.ammunitions.name_type);
            if (!ammoExtra)
            {
                WriteToLog("Erro ao criar munição extra!", LogFile.INIT, false, LogType.ERROR);
                break;
            }
        }
        
    }

}


EntityAI CreateItemWithSubitems(EntityAI parent, LoadoutItem itemData, PlayerBase player)
{
    EntityAI item;

    if (parent) {
        WriteToLog("Criando item como attachment: " + itemData.name_type, LogFile.INIT, false, LogType.INFO);
        item = parent.GetInventory().CreateAttachment(itemData.name_type);
    } else {
        WriteToLog("Criando item no inventário: " + itemData.name_type, LogFile.INIT, false, LogType.INFO);
        item = player.GetInventory().CreateInInventory(itemData.name_type);
    }

    if (!item) {
        WriteToLog("Erro ao criar item: " + itemData.name_type, LogFile.INIT, false, LogType.ERROR);
        return null;
    }

    if (itemData.subitems) {
        foreach (LoadoutItem sub : itemData.subitems) {
            if (sub) {
                CreateItemWithSubitems(item, sub, player);
            } else {
                WriteToLog("Subitem nulo detectado para: " + itemData.name_type, LogFile.INIT, false, LogType.ERROR);
            }
        }
    }

    return item;
}


void GiveDefaultLoadout(PlayerBase player)
{
    WriteToLog("Iniciando entrega de loadout padrão para o jogador.", LogFile.INIT, false, LogType.INFO);

    // Vestimenta e proteção
    ref array<string> roupas = {
        "TacticalShirt_Black", "CargoPants_Black", "CombatBoots_Black",
        "BalaclavaMask_Black", "TacticalGloves_Black", "TacticalGoggles"
    };

    foreach (string itemName : roupas) {
        EntityAI item = player.GetInventory().CreateInInventory(itemName);
        if (item)
            WriteToLog("Equipado: " + itemName, LogFile.INIT, false, LogType.INFO);
        else
            WriteToLog("Erro ao equipar: " + itemName, LogFile.INIT, false, LogType.ERROR);
    }

    // Capacete + NVG
    EntityAI helmet = player.GetInventory().CreateInInventory("Mich2001Helmet");
    EntityAI nvg = null;
    if (helmet) {
        nvg = helmet.GetInventory().CreateAttachment("NVGoggles");
        if (nvg) {
            nvg.GetInventory().CreateAttachment("Battery9V");
            WriteToLog("NVG equipado com bateria.", LogFile.INIT, false, LogType.INFO);
        } else {
            WriteToLog("Erro ao anexar NVG.", LogFile.INIT, false, LogType.ERROR);
        }
    }

    // Equipamentos úteis
    ref array<string> utilitarios = {
        "Battery9V", "Binoculars", "Canteen", "StarlightOptic", "Rangefinder"
    };

    foreach (string util : utilitarios) {
        player.GetInventory().CreateInInventory(util);
    }

    // Medicamentos
    ref array<string> medics = {
        "BandageDressing", "Morphine", "TetracyclineAntibiotics", "Painkiller"
    };

    foreach (string med : medics) {
        player.GetInventory().CreateInInventory(med);
    }

    // Mochila
    player.GetInventory().CreateInInventory("AliceBag_Camo");

    // Arma primária: M4A1
    EntityAI m4 = player.GetHumanInventory().CreateInHands("M4A1");
    //EntityAI m4 = player.GetInventory().CreateInInventory("M4A1");
    if (m4) {   
        
        ref array<string> m4Attachments = {
            "ACOGOptic", "M4_RISHndgrd_Black", "M4_MPBttstck_Black"
        };

        foreach (string att : m4Attachments) {
            m4.GetInventory().CreateAttachment(att);
        }

        Weapon_Base weapon_base = Weapon_Base.Cast(m4);
        Magazine magM4 = weapon_base.SpawnAttachedMagazine("Mag_STANAG_30Rnd");	// ja cria com o pente cheio
        int amountAmmo = magM4.GetAmmoMax() - 1;
        magM4.LocalSetAmmoCount(amountAmmo);	
        magM4.ServerSetAmmoCount(amountAmmo);	
        // magM4.LocalSetAmmoCount(0);	
        // magM4.ServerSetAmmoCount(0);	
        // for (int i = 0; i < 29; i++) {
        //     magM4.GetInventory().CreateInInventory("Ammo_556x45Tracer");
        // }

        // string ammoTypeName = "Ammo_556x45Tracer";
        // Weapon_Base magazine_base = Magazine_Base.Cast(magM4);
        // for (int imuzzCount = 0; imuzzCount < 19; ++imuzzCount)
        // {   
        //     WriteToLog("Inserindo municao " + ammoTypeName + " no pente... " + imuzzCount);
        //     magM4.PushCartridgeToChamber(imuzzCount, 0, ammoTypeName);
        // }  
        
        // FUNCIONA MAS SEM BALA NO CHAMBER
        // EntityAI mag = m4.GetInventory().CreateAttachment("Mag_STANAG_30Rnd");
        // if (mag) {
        //     Magazine castMag = Magazine.Cast(mag);
        //     Weapon_Base weapon = Weapon_Base.Cast(m4);

        //     if (castMag && weapon) {
        //         castMag.ServerSetAmmoCount(20);
                
        //         // TENTATIVA FALHA DE BOTAR BALA NO CHAMBER
        //         // string ammoTypeName = weapon.GetChamberAmmoTypeName(0); // obtém o nome da munição para esse slot, pode pegar do json
        //         // if (ammoTypeName == "")
        //         // {
        //         //     WriteToLog("ammoTypeName não identificado. Usando Ammo_556x45");
        //         //     ammoTypeName = "Ammo_556x45";
        //         // }                 
        //         // int muzzCount = weapon.GetMuzzleCount();
        //         // WriteToLog("Quantidade suportada no chamber: " + muzzCount);
        //         // for (int imuzzCount = 0; imuzzCount < muzzCount; ++imuzzCount)
        //         // {   
        //         //     WriteToLog("Inserindo municao " + ammoTypeName + " no chamber... " + imuzzCount);
        //         //     weapon.PushCartridgeToChamber(imuzzCount, 0, ammoTypeName);
        //         // }                

        //         WriteToLog("M4A1 com carregador e munição na câmara pronta para disparo.");
        //     } else {
        //         WriteToLog("Erro ao converter carregador ou arma.");
        //     }
        // } else {
        //     WriteToLog("Erro ao criar carregador na M4A1.");
        // }


        // // Carregador separado e seguro
        // EntityAI mag = player.GetInventory().CreateInInventory("Mag_STANAG_30Rnd");
        // Magazine castMag = Magazine.Cast(mag);
        // if (castMag) {
        //     castMag.ServerSetAmmoCount(20);
        //     bool okToAdd = m4.GetInventory().CanAddAttachment(castMag);
        //     if (okToAdd)
        //         WriteToLog("Pode ser adicionado!");
        //     else
        //         WriteToLog("Não pode ser adicionado!");
            
        //     // Coloca a arma na mão antes de anexar o carregador, com delay
        //     //GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MoveWeaponToHandsAndAttachMag, 1000, false, player, m4, castMag);

        //     // Tentativa 1 - Não funciona
        //     // bool PredictiveTakeEntityAsAttachment = m4.PredictiveTakeEntityAsAttachment(castMag);
        //     // if (PredictiveTakeEntityAsAttachment)
        //     //     WriteToLog("M4A1 com carregador funcional carregado.");
        //     // else
        //     //     WriteToLog("Erro ao criar carregador M4A1.");

        //     // // Tentativa 12 - Não funciona
        //     // m4.LocalTakeEntityAsAttachment(mag);
        //     // m4.ServerTakeEntityAsAttachment(mag);

        //     //player.TakeEntityToHandsImpl(InventoryMode.LOCAL, mag);
        //     // bool LocalTakeEntityToTargetAttachment = m4.LocalTakeEntityToTargetAttachment(mag);
        //     // LocalTakeEntityToTargetAttachment (notnull EntityAI target, notnull EntityAI item)

        //     // {
        //     //         TakeEntityToHandsImpl(InventoryMode.LOCAL, item);
        //     //     }
                
        //     //     void ServerTakeEntityToHands (EntityAI item)
        //     //     {
        //     //         TakeEntityToHandsImpl(InventoryMode.SERVER, item);
        //     //     }
            
        // } else {
        //     WriteToLog("Erro ao criar carregador M4A1.");
        // }

        player.SetQuickBarEntityShortcut(m4, 0, true);

        EntityAI AKM_Entity = player.GetInventory().CreateInInventory("AKM");
        AKM_Entity.SetHealth(AKM_Entity.GetMaxHealth());		// Remove any damage from item
        EntityAI Kobra = AKM_Entity.GetInventory().CreateAttachment("KobraOptic");	// set attachment
        Kobra.GetInventory().CreateAttachment("Battery9V");							// add battary for attachment
        AKM_Entity.GetInventory().CreateAttachment("AK_Bayonet");			// let's add some muzle attachment
        AKM_Entity.GetInventory().CreateAttachment("AK_PlasticHndgrd");		// I hate to feel the smell of my hands, let's set handguard
        AKM_Entity.GetInventory().CreateAttachment("AK_PlasticBttstck");	// I wish to kick zmbs a**es so I need the buttstock!
        player.SetQuickBarEntityShortcut(AKM_Entity, 1, true);		// Set ours AKM to prime slot of quick bar.
        Weapon wpn = Weapon.Cast(AKM_Entity);			// Now we cast or AKM to Weapon-instance class
        Weapon_Base wpn_bs1 = Weapon_Base.Cast(wpn);	// For safe way recast to WeaponBase
        Magazine magAKM = wpn_bs1.SpawnAttachedMagazine("Mag_AKM_Drum75Rnd");	// Attach mag to ours AKM = 75-1 (coz 1 cartrige placed ain camber
        magAKM.LocalSetAmmoCount(magAKM.GetAmmoMax());		// Intent our Mag has full cap for local player instance
        magAKM.ServerSetAmmoCount(magAKM.GetAmmoMax());		// Same fr Server instance
        // On that stage we have a full AKM with mag and chambered!

        // Munições adicionais
        // for (int i = 0; i < 4; i++) {
        //     EntityAI magExtra = player.GetInventory().CreateInInventory("Mag_STANAG_30Rnd");
        //     Magazine magCasted = Magazine.Cast(magExtra);
        //     if (magCasted)
        //         magCasted.ServerSetAmmoCount(30);
        // }
    }

    // // Colete e pistola
    // EntityAI vest = player.GetInventory().CreateInInventory("PlateCarrierVest");
    // if (vest) {
    //     vest.GetInventory().CreateAttachment("PlateCarrierHolster");
    //     vest.GetInventory().CreateAttachment("PlateCarrierPouches");

    //     EntityAI pistol = vest.GetInventory().CreateInInventory("Glock19");
    //     if (pistol) {
    //         EntityAI magPistol = player.GetInventory().CreateInInventory("Mag_Glock_15Rnd");
    //         Magazine pistolMag = Magazine.Cast(magPistol);
    //         if (pistolMag) {
    //             pistolMag.ServerSetAmmoCount(15);
    //             pistol.LocalTakeEntityAsAttachment(magPistol);
    //         } else {
    //             WriteToLog("Erro ao cast de Mag_Glock_15Rnd.");
    //         }

    //         pistol.GetInventory().CreateAttachment("PistolSuppressor");
    //         pistol.GetInventory().CreateAttachment("UniversalLight");

    //         player.SetQuickBarEntityShortcut(pistol, 3, true);
    //     } else {
    //         WriteToLog("Erro ao criar Glock19.");
    //     }
    // }

    // // Munições soltas
    // ref array<string> ammo = {
    //     "Ammo_9x19", "Ammo_556x45", "Ammo_308WinTracer", "Ammo_762x39"
    // };

    // foreach (string a : ammo) {
    //     player.GetInventory().CreateInInventory(a);
    // }

    // // Rifle Tundra
    // EntityAI tundra = player.GetInventory().CreateInInventory("Winchester70");
    // if (tundra) {
    //     tundra.GetInventory().CreateAttachment("HuntingOptic");
    //     tundra.GetInventory().CreateAttachment("ImprovisedSuppressor");
    //     player.SetQuickBarEntityShortcut(tundra, 2, true);
    // }

    // // Rifle AKM
    // EntityAI akm = player.GetInventory().CreateInInventory("AKM");
    // if (akm) {
    //     akm.GetInventory().CreateAttachment("KobraOptic");
    //     akm.GetInventory().CreateAttachment("AK_WoodHandguard");
    //     akm.GetInventory().CreateAttachment("AK_WoodBttstck");

    //     EntityAI akmMag = player.GetInventory().CreateInInventory("Mag_AKM_Drum75Rnd");
    //     Magazine akmMagCast = Magazine.Cast(akmMag);
    //     if (akmMagCast) {
    //         akmMagCast.ServerSetAmmoCount(75);
    //         akm.LocalTakeEntityAsAttachment(akmMag);
    //     } else {
    //         WriteToLog("Erro ao cast de tambor AKM.");
    //     }

    //     player.GetInventory().CreateInInventory("Ammo_762x39");
    //     player.SetQuickBarEntityShortcut(akm, 1, true);
    // }

    // // Cinto + faca
    // EntityAI belt = player.GetInventory().CreateInInventory("UtilityBelt");
    // if (belt)
    //     belt.GetInventory().CreateAttachment("CombatKnife");

    WriteToLog("Loadout padrão entregue com sucesso.", LogFile.INIT, false, LogType.INFO);
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


// Criou o pente com munição bugada
// Magazine CreateMagazineWithAmmo(string magType, string ammoType, int count, EntityAI target = null)
// {
// 	// Cria o magazine no inventário do player ou diretamente no mundo
// 	Magazine mag;
// 	if (target)
// 		mag = Magazine.Cast(target.GetInventory().CreateInInventory(magType));
// 	else
// 		mag = Magazine.Cast(GetGame().CreateObject(magType, Vector(0,0,0)));

// 	if (!mag) {
// 		Print("Falha ao criar magazine: " + magType);
// 		return null;
// 	}

// 	// Garante que está vazio antes de carregar
// 	mag.ServerSetAmmoCount(0);

// 	// Adiciona as munições do tipo especificado
// 	for (int i = 0; i < count; i++) {
// 		bool success = mag.ServerStoreCartridge(0, ammoType);
// 		if (!success) {
// 			Print("Falha ao inserir munição '" + ammoType + "' no pente '" + magType + "'");
// 			break;
// 		}
// 	}

// 	Print("Magazine '" + magType + "' criado com " + count.ToString() + " munições do tipo '" + ammoType + "'");
// 	return mag;
// }
