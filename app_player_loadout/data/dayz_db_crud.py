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
    """)
    conn.commit()
    print("Tabelas criadas com sucesso.")

if __name__ == "__main__":
    criar_tabelas()

