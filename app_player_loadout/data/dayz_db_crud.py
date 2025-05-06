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
        feed_type TEXT
    );

    CREATE TABLE IF NOT EXISTS ammunitions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        caliber TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS magazines (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        ammo_type TEXT NOT NULL,
        capacity INTEGER
    );

    CREATE TABLE IF NOT EXISTS attachments (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        type TEXT NOT NULL
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

# === FUNÇÕES DE CRUD SIMPLES ===

def adicionar_arma(nome, feed_type):
    cursor.execute("INSERT OR IGNORE INTO weapons (name, feed_type) VALUES (?, ?)", (nome, feed_type))
    conn.commit()

def adicionar_municao(nome, caliber):
    cursor.execute("INSERT OR IGNORE INTO ammunitions (name, caliber) VALUES (?, ?)", (nome, caliber))
    conn.commit()

def adicionar_carregador(nome, ammo_type, capacidade):
    cursor.execute("INSERT OR IGNORE INTO magazines (name, ammo_type, capacity) VALUES (?, ?, ?)", (nome, ammo_type, capacidade))
    conn.commit()

def adicionar_acessorio(nome, tipo):
    cursor.execute("INSERT OR IGNORE INTO attachments (name, type) VALUES (?, ?)", (nome, tipo))
    conn.commit()

def vincular_arma_municao(arma_nome, municao_nome):
    cursor.execute("SELECT id FROM weapons WHERE name = ?", (arma_nome,))
    arma_id = cursor.fetchone()
    cursor.execute("SELECT id FROM ammunitions WHERE name = ?", (municao_nome,))
    muni_id = cursor.fetchone()
    if arma_id and muni_id:
        cursor.execute("INSERT OR IGNORE INTO weapon_ammunitions (weapon_id, ammo_id) VALUES (?, ?)", (arma_id[0], muni_id[0]))
        conn.commit()

def vincular_arma_carregador(arma_nome, mag_nome):
    cursor.execute("SELECT id FROM weapons WHERE name = ?", (arma_nome,))
    arma_id = cursor.fetchone()
    cursor.execute("SELECT id FROM magazines WHERE name = ?", (mag_nome,))
    mag_id = cursor.fetchone()
    if arma_id and mag_id:
        cursor.execute("INSERT OR IGNORE INTO weapon_magazines (weapon_id, magazine_id) VALUES (?, ?)", (arma_id[0], mag_id[0]))
        conn.commit()

def listar_armas():
    cursor.execute("SELECT * FROM weapons")
    armas = cursor.fetchall()
    for arma in armas:
        print(f"ID: {arma[0]} | Nome: {arma[1]} | Feed: {arma[2]}")

# === EXEMPLO DE USO ===

if __name__ == "__main__":
    criar_tabelas()

    adicionar_arma("M4A1", "magazine")
    adicionar_municao("5.56x45mm", "5.56")
    adicionar_carregador("STANAG 30Rnd", "5.56x45mm", 30)
    adicionar_acessorio("ACOG Scope", "optic")

    vincular_arma_municao("M4A1", "5.56x45mm")
    vincular_arma_carregador("M4A1", "STANAG 30Rnd")

    print("\n--- Armas cadastradas ---")
    listar_armas()
