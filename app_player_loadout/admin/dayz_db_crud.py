import sqlite3

# === CONEXÃO E CRIAÇÃO DO BANCO ===
conn = sqlite3.connect("dayz_items.db")
cursor = conn.cursor()

# === CRIAÇÃO DAS TABELAS ===
def criar_tabelas():
    cursor.executescript("""
    CREATE TABLE IF NOT EXISTS weapons (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        name_type TEXT UNIQUE NOT NULL,
        feed_type TEXT NOT NULL,
        slots INTEGER NOT NULL,
        width INTEGER NOT NULL,
        height INTEGER NOT NULL,
        img TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS explosives (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        name_type TEXT UNIQUE NOT NULL,
        slots INTEGER NOT NULL,
        width INTEGER NOT NULL,
        height INTEGER NOT NULL,
        img TEXT NOT NULL
    );
    
    CREATE TABLE IF NOT EXISTS calibers (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL        
    );

    CREATE TABLE IF NOT EXISTS ammunitions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        name_type TEXT UNIQUE NOT NULL,
        caliber_id INTEGER NOT NULL,
        slots INTEGER NOT NULL,
        width INTEGER NOT NULL,
        height INTEGER NOT NULL,
        img TEXT NOT NULL,
        FOREIGN KEY (caliber_id) REFERENCES weapons(id)
    );

    CREATE TABLE IF NOT EXISTS magazines (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        name_type TEXT UNIQUE NOT NULL,
        capacity INTEGER,
        slots INTEGER NOT NULL,
        width INTEGER NOT NULL,
        height INTEGER NOT NULL,
        img TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS attachments (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        name_type TEXT UNIQUE NOT NULL,
        type TEXT NOT NULL,
        slots INTEGER NOT NULL,
        width INTEGER NOT NULL,
        height INTEGER NOT NULL,
        img TEXT NOT NULL,
        battery INTEGER NOT NULL DEFAULT 0
    );

    CREATE TABLE IF NOT EXISTS weapon_attachments (
        weapon_id INTEGER,
        attachment_id INTEGER,
        PRIMARY KEY (weapon_id, attachment_id),
        FOREIGN KEY (weapon_id) REFERENCES weapons(id),
        FOREIGN KEY (attachment_id) REFERENCES attachments(id)
    );

    CREATE TABLE IF NOT EXISTS weapon_magazines (
        weapon_id INTEGER,
        magazine_id INTEGER,
        PRIMARY KEY (weapon_id, magazine_id),
        FOREIGN KEY (weapon_id) REFERENCES weapons(id),
        FOREIGN KEY (magazine_id) REFERENCES magazines(id)
    );

    CREATE TABLE IF NOT EXISTS weapon_ammunitions (
        weapon_id INTEGER,
        ammo_id INTEGER,
        PRIMARY KEY (weapon_id, ammo_id),
        FOREIGN KEY (weapon_id) REFERENCES weapons(id),
        FOREIGN KEY (ammo_id) REFERENCES ammunitions(id)
    );
    
    CREATE TABLE IF NOT EXISTS loadout_rules_weapons (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        weapon_id INTEGER UNIQUE NOT NULL,
        is_banned BOOLEAN NOT NULL DEFAULT 0,
        FOREIGN KEY (weapon_id) REFERENCES weapons(id) ON DELETE CASCADE
    );
    CREATE TABLE IF NOT EXISTS player_loadouts_weapons (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        player_id TEXT UNIQUE NOT NULL,
        
        primary_weapon_id INTEGER,
        primary_magazine_id INTEGER,
        primary_ammo_id INTEGER,
        
        secondary_weapon_id INTEGER,
        secondary_magazine_id INTEGER,
        secondary_ammo_id INTEGER,
        
        small_weapon_id INTEGER,
        small_magazine_id INTEGER,
        small_ammo_id INTEGER,
        
        FOREIGN KEY (primary_weapon_id) REFERENCES weapons(id),
        FOREIGN KEY (primary_magazine_id) REFERENCES magazines(id),
        FOREIGN KEY (primary_ammo_id) REFERENCES ammunitions(id),
        
        FOREIGN KEY (secondary_weapon_id) REFERENCES weapons(id),
        FOREIGN KEY (secondary_magazine_id) REFERENCES magazines(id),
        FOREIGN KEY (secondary_ammo_id) REFERENCES ammunitions(id),
        
        FOREIGN KEY (small_weapon_id) REFERENCES weapons(id),
        FOREIGN KEY (small_magazine_id) REFERENCES magazines(id),
        FOREIGN KEY (small_ammo_id) REFERENCES ammunitions(id)
    );
    
    CREATE TABLE IF NOT EXISTS player_loadouts_weapon_attachments (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        player_loadouts_weapons_id INTEGER NOT NULL,
        attachment_id INTEGER NOT NULL,
        weapon_slot TEXT CHECK(weapon_slot IN ('primary', 'secondary', 'small')),
        FOREIGN KEY (player_loadouts_weapons_id) REFERENCES player_loadouts_weapons(id),
        FOREIGN KEY (attachment_id) REFERENCES attachments(id)
    );    

    CREATE TABLE IF NOT EXISTS player_loadouts_weapon_explosives (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        player_loadouts_weapons_id INTEGER NOT NULL,
        explosive_id INTEGER NOT NULL,
        quantity INTEGER NOT NULL,
        FOREIGN KEY (player_loadouts_weapons_id) REFERENCES player_loadouts_weapons(id),
        FOREIGN KEY (explosive_id) REFERENCES explosives(id)
    );   
    
    -- Tipos de item
    CREATE TABLE IF NOT EXISTS item_types (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL
    );

    -- Itens genéricos
    CREATE TABLE IF NOT EXISTS item (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        name_type TEXT UNIQUE NOT NULL,
        type_id INTEGER NOT NULL,
        slots INTEGER NOT NULL,
        width INTEGER NOT NULL,
        height INTEGER NOT NULL,
        img TEXT NOT NULL,
        storage_slots INTEGER DEFAULT 0,
        storage_width INTEGER DEFAULT 0,
        storage_height INTEGER DEFAULT 0,
        localization TEXT,
        FOREIGN KEY (type_id) REFERENCES item_types(id)
    );

    -- Auto-relacionamento: item que pode ser encaixado em outro
    CREATE TABLE IF NOT EXISTS item_compatibility (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        parent_item_id INTEGER NOT NULL, -- item que "recebe"
        child_item_id INTEGER NOT NULL,  -- item que "encaixa"
        FOREIGN KEY (parent_item_id) REFERENCES item(id),
        FOREIGN KEY (child_item_id) REFERENCES item(id),
        UNIQUE (parent_item_id, child_item_id) -- evita duplicatas
    );
    
    CREATE TABLE player_items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        player_id TEXT NOT NULL,
        item_id INTEGER NOT NULL,
        quantity INTEGER NOT NULL DEFAULT 1,
        FOREIGN KEY (item_id) REFERENCES item(id),
        UNIQUE (player_id, item_id) -- evita múltiplas linhas duplicadas
    );

    CREATE TABLE player_logins (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        player_id TEXT UNIQUE NOT NULL,
        login TEXT UNIQUE NOT NULL,
        password TEXT NOT NULL,
        token TEXT,
        active INTEGER DEFAULT 0,
        admin INTEGER DEFAULT 0
    );


    """)
    conn.commit()
    print("Tabelas criadas com sucesso.")

if __name__ == "__main__":
    criar_tabelas()

