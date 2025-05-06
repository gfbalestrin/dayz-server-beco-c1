from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from typing import List
from datetime import datetime
import sqlite3
import os

app = FastAPI()

# Servir arquivos estáticos
app.mount("/static", StaticFiles(directory="static"), name="static")

@app.get("/")
def serve_index():
    return FileResponse("static/index.html")

DB_PATH = "databases/players_beco_c1.db"

def get_db_connection():
    if not os.path.exists(DB_PATH):
        raise FileNotFoundError("Banco de dados não encontrado.")
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

@app.get("/api/jogadores_online")
def get_jogadores_online():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM players_online")
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]

@app.get("/api/jogadores")
def get_jogadores():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM players_database ORDER BY PlayerName ASC")  
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]

@app.get("/api/datas/{player_id}")
def get_datas_por_player(player_id: str):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT DISTINCT DATE(Data) as Data
        FROM players_coord
        WHERE PlayerID = ?
        ORDER BY Data DESC
    """, (player_id,))
    rows = cursor.fetchall()
    conn.close()
    return [row["Data"] for row in rows]

@app.get("/api/coords/{player_id}/{data}")
def get_coords_por_playerid_data(player_id: str, data: str):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT * FROM players_coord
        WHERE PlayerID = ? AND DATE(Data) = ?
        ORDER BY Data ASC
    """, (player_id, data))
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]

@app.get("/api/coords/{data}")
def get_coords_por_data(data: str):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT Distinct PlayerID FROM players_coord
        WHERE DATE(Data) = ?
        ORDER BY Data ASC
    """, (data,))
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]

@app.get("/api/coords_events/{player_id}/{data}")
def get_coords_events_por_playerid_data(player_id: str, data: str):
    conn = get_db_connection()
    cursor = conn.cursor()

    # Coordenadas do jogador
    cursor.execute("""
        SELECT * FROM players_coord
        WHERE PlayerID = ? AND DATE(Data) = ?
        ORDER BY Data ASC
    """, (player_id, data))
    coords = cursor.fetchall()

    # Eventos de Killed
    cursor.execute("""
        SELECT Data FROM players_killfeed
        WHERE PlayerIDKilled = ? AND DATE(Data) = ?
        ORDER BY Data ASC
    """, (player_id, data))
    killed_events = [datetime.strptime(row["Data"], "%Y-%m-%d %H:%M:%S") for row in cursor.fetchall()]

    # Eventos de Killer
    cursor.execute("""
        SELECT Data FROM players_killfeed
        WHERE PlayerIDKiller = ? AND DATE(Data) = ?
        ORDER BY Data ASC
    """, (player_id, data))
    killer_events = [datetime.strptime(row["Data"], "%Y-%m-%d %H:%M:%S") for row in cursor.fetchall()]

    conn.close()

    result = []
    killed_idx = 0
    killer_idx = 0

    for row in coords:
        coord_time = datetime.strptime(row["Data"], "%Y-%m-%d %H:%M:%S")
        event_label = ""

        # Verifica se há próximo evento Killed e se coord_time é depois dele
        if killed_idx < len(killed_events) and coord_time >= killed_events[killed_idx]:
            event_label = "Killed"
            killed_idx += 1  # Avança para o próximo evento

        # Verifica se há próximo evento Killer e se coord_time é depois dele
        if killer_idx < len(killer_events) and coord_time >= killer_events[killer_idx]:
            event_label = "Killer"
            killer_idx += 1

        result.append({
            "PlayerCoordId": row["PlayerCoordId"],
            "PlayerID": row["PlayerID"],
            "CoordX": row["CoordX"],
            "CoordZ": row["CoordZ"],
            "CoordY": row["CoordY"],
            "Data": row["Data"],
            "Evento": event_label
        })

    return result

@app.get("/api/backups/{player_coord_id}")
def get_backup(player_coord_id: int):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT * FROM players_coord_backup
        WHERE PlayerCoordId = ? 
    """, (player_coord_id,))
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]