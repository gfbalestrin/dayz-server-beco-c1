from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from typing import List
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
    cursor.execute("SELECT * FROM players_database")
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
def get_coords_por_data(player_id: str, data: str):
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
