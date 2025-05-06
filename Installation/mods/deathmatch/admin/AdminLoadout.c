void GiveSurvivorLoadout(PlayerBase player)
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





