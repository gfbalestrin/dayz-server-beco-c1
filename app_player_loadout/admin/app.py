import requests
import xml.etree.ElementTree as ET
from flask import Flask, render_template, request, redirect, url_for, session, flash, jsonify
from werkzeug.security import check_password_hash, generate_password_hash
from apscheduler.schedulers.background import BackgroundScheduler
from collections import defaultdict
from functools import wraps
import os
import json
import sqlite3
import traceback


USERS = {
    "admin": generate_password_hash("xxxxxxxxxxxxxxx")
}

app = Flask(__name__)
app.secret_key = 'xxxxxxxxxxxxxx'  # Altere para uma chave forte na produção

JSON_PATH = 'custom_loadouts.json'

def login_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'login' not in session:
            flash("Você precisa estar logado para acessar esta página.", "warning")
            return redirect(url_for('login'))
        return f(*args, **kwargs)
    return decorated_function

def admin_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if not session.get('is_admin'):
            flash("Acesso restrito a administradores.", "danger")
            return redirect(url_for('loadout_players'))
        return f(*args, **kwargs)
    return decorated_function

# Carrega o XML e extrai os nomes uma vez ao iniciar
def load_type_names():
    url = "https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/mpmissions/dayzOffline.chernarusplus/db/types.xml"
    response = requests.get(url)
    response.raise_for_status()
    root = ET.fromstring(response.content)
    return [elem.get("name") for elem in root.findall("type")]

# Armazena em memória
type_names = load_type_names()

def get_db_connection():
    conn = sqlite3.connect('../databases/dayz_items.db')
    conn.row_factory = sqlite3.Row
    return conn

def get_db_connection_players_beco_c1():
    conn = sqlite3.connect('../databases/players_beco_c1.db')
    conn.row_factory = sqlite3.Row
    return conn

def query_db(query, args=(), one=False):
    conn = get_db_connection()
    cur = conn.execute(query, args)
    rv = cur.fetchall()
    cur.close()
    conn.close()
    return (rv[0] if rv else None) if one else rv


def get_weapon_data(weapon_id, magazine_id, ammo_id, attachments):
    if not weapon_id:
        return None

    weapon = query_db("SELECT * FROM weapons WHERE id = ?", [weapon_id], one=True)
    if not weapon:
        return None

    weapon_data = {
        "name_type": weapon["name_type"],
        "feed_type": weapon["feed_type"],
        "slots": weapon["slots"],
        "width": weapon["width"],
        "height": weapon["height"]
    }

    if magazine_id:
        mag = query_db("SELECT * FROM magazines WHERE id = ?", [magazine_id], one=True)
        if mag:
            weapon_data["magazine"] = {
                "name_type": mag["name_type"],
                "capacity": mag["capacity"],
                "slots": mag["slots"],
                "width": mag["width"],
                "height": mag["height"]
            }

    if ammo_id:
        ammo = query_db("SELECT * FROM ammunitions WHERE id = ?", [ammo_id], one=True)
        if ammo:
            weapon_data["ammunitions"] = {
                "name_type": ammo["name_type"],
                "slots": ammo["slots"],
                "width": ammo["width"],
                "height": ammo["height"]
            }

    if attachments:
        attachment_list = []
        for att in attachments:
            attachment_list.append({
                "name_type": att["name_type"],
                "type": att["type"],
                "slots": att["slots"],
                "width": att["width"],
                "height": att["height"],
                "battery": bool(att["battery"])
            })
        weapon_data["attachments"] = attachment_list

    return weapon_data

def export_loadouts_json():
    # Pega todos os player_ids únicos com armas OU itens
    player_rows = query_db("""
        SELECT DISTINCT player_id FROM player_loadouts_weapons
        UNION
        SELECT DISTINCT player_id FROM player_items
    """)

    full_data = {}

    for row in player_rows:
        player_id = row["player_id"]
        player_data = {"weapons": {}}

        # Tenta buscar dados de armas desse jogador
        weapon_data = query_db("SELECT * FROM player_loadouts_weapons WHERE player_id = ? LIMIT 1", [player_id])
        
        if weapon_data:
            weapon_data = weapon_data[0]

            all_attachments = query_db("""
                SELECT a.*, lwa.weapon_slot 
                FROM player_loadouts_weapon_attachments lwa
                JOIN attachments a ON a.id = lwa.attachment_id
                WHERE lwa.player_loadouts_weapons_id = ?
            """, [weapon_data["id"]])

            attachments_by_slot = {"primary": [], "secondary": [], "small": []}
            for att in all_attachments:
                attachments_by_slot[att["weapon_slot"]].append(att)

            player_data["weapons"]["primary_weapon"] = get_weapon_data(
                weapon_data["primary_weapon_id"],
                weapon_data["primary_magazine_id"],
                weapon_data["primary_ammo_id"],
                attachments_by_slot["primary"]
            )
            player_data["weapons"]["secondary_weapon"] = get_weapon_data(
                weapon_data["secondary_weapon_id"],
                weapon_data["secondary_magazine_id"],
                weapon_data["secondary_ammo_id"],
                attachments_by_slot["secondary"]
            )
            player_data["weapons"]["small_weapon"] = get_weapon_data(
                weapon_data["small_weapon_id"],
                weapon_data["small_magazine_id"],
                weapon_data["small_ammo_id"],
                attachments_by_slot["small"]
            )
        else:
            # Preenche None caso o jogador não tenha armas
            player_data["weapons"]["primary_weapon"] = None
            player_data["weapons"]["secondary_weapon"] = None
            player_data["weapons"]["small_weapon"] = None

        # Buscar itens do jogador
        items = query_db("""
            SELECT i.*, it.name as type_name, pi.quantity
            FROM player_items pi
            JOIN item i ON i.id = pi.item_id
            JOIN item_types it ON it.id = i.type_id
            WHERE pi.player_id = ?
        """, [player_id])

        # Se o jogador tem pelo menos armas ou itens, ele é processado
        if len(items) == 0 and not any(player_data["weapons"].values()):
            continue  # Nenhuma arma e nenhum item, então o jogador não é incluído

        # Montar mapa auxiliar
        item_map = {item["id"]: dict(item) for item in items}
        quantities = {item["id"]: item["quantity"] for item in items}

        # Relações de compatibilidade
        compat = query_db("SELECT parent_item_id, child_item_id FROM item_compatibility")
        compat_map = {}
        for row in compat:
            compat_map.setdefault(row["parent_item_id"], []).append(row["child_item_id"])

        used_counts = defaultdict(int)
        item_list = []

        def build_item_json(item):
            return {
                "name_type": item["name_type"],
                "type_name": item["type_name"],
                "slots": item["slots"],
                "width": item["width"],
                "height": item["height"],
                "storage_slots": item["storage_slots"],
                "storage_width": item["storage_width"],
                "storage_height": item["storage_height"],
                "localization": item["localization"],
                "subitems": []
            }

        def build_item_tree(item, compat_map, item_map, used_counts, depth=0, max_depth=5, ancestry=None):
            if depth >= max_depth:
                return build_item_json(item)

            ancestry = ancestry or set()
            ancestry.add(item["id"])

            item_json = build_item_json(item)

            children = compat_map.get(item["id"], [])
            for child_id in children:
                if used_counts[child_id] >= quantities.get(child_id, 0) or child_id in ancestry:
                    continue

                child_item = item_map.get(child_id)
                if not child_item:
                    continue

                used_counts[child_id] += 1
                child_json = build_item_tree(
                    child_item, compat_map, item_map, used_counts,
                    depth + 1, max_depth, ancestry.copy()
                )
                item_json["subitems"].append(child_json)

            return item_json
        for item_id, quantity in quantities.items():
            for _ in range(quantity):
                if used_counts[item_id] >= quantity:
                    continue
                used_counts[item_id] += 1
                item = item_map[item_id]
                item_list.append(build_item_tree(item, compat_map, item_map, used_counts))

        player_data["items"] = item_list
        full_data[player_id] = player_data

    # Salvar JSON
    with open(JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(full_data, f, indent=2)

    return True

def start_scheduler():
    scheduler = BackgroundScheduler()
    scheduler.add_job(func=export_loadouts_json, trigger="interval", seconds=60)
    scheduler.start()

    # Garante que o scheduler pare quando o app parar
    import atexit
    atexit.register(lambda: scheduler.shutdown())

@app.route('/register', methods=['GET', 'POST'])
@login_required
@admin_required
def register():
    if request.method == 'POST':
        login = request.form['login'].strip()
        player_id = request.form['player_id'].strip()
        password = request.form['password'].strip()

        if not login or not player_id or not password:
            flash('Todos os campos são obrigatórios.', 'danger')
            return render_template('register.html')

        conn = get_db_connection()
        cursor = conn.cursor()

        # Verifica se o usuário já existe
        cursor.execute("SELECT * FROM player_logins WHERE player_id = ?", (player_id,))
        existing_user = cursor.fetchone()

        if existing_user:
            flash('Nome de usuário já está em uso.', 'danger')
            conn.close()
            return render_template('register.html')

        # Hash da senha
        hashed_password = generate_password_hash(password)

        # Inserir novo usuário
        cursor.execute("""
            INSERT INTO player_logins (login, player_id, password, active, admin)
            VALUES (?, ?, ?, 1, 0)
        """, (login, player_id, hashed_password))

        conn.commit()
        conn.close()

        flash('Registro realizado com sucesso! Faça login.', 'success')
        return redirect(url_for('login'))

    return render_template('register.html')

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        login = request.form['username']
        password = request.form['password']

        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM player_logins WHERE login = ?", (login,))
        user = cursor.fetchone()
        conn.close()

        if user and check_password_hash(user['password'], password):
            session['login'] = login
            session['player_id'] = user['player_id']
            session['is_admin'] = bool(user['admin'])  # <== Aqui
            flash('Login realizado com sucesso!', 'success')

            if session['is_admin']:
                return redirect(url_for('index'))
            else:
                return redirect(url_for('loadout_players'))
        else:
            flash("Usuário ou senha inválidos", "danger")

    return render_template('login.html')

@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('login'))

@app.route('/')
@login_required
@admin_required
def index():
    try:
        # Conectar ao banco de dados
        conn = get_db_connection()

    except DatabaseError as e:
        flash(f"Ocorreu um erro ao acessar o banco de dados: {str(e)}", 'danger')
        return redirect(url_for('login'))

    finally:
        # Fechar a conexão com o banco de dados de forma segura
        conn.close()

    # Renderizar a página com os dados obtidos
    return render_template('index.html')

# Arma
@app.route("/api/weapons")
@login_required
def api_weapons():
    page = int(request.args.get("page", 1))
    query = request.args.get("q", "").strip()
    feed_type = request.args.get("feed_type", "").strip().lower()
    per_page = 6
    offset = (page - 1) * per_page

    conn = get_db_connection()
    cur = conn.cursor()

    base_query = "SELECT * FROM weapons"
    filters = []
    params = []

    if query:
        filters.append("(name LIKE ? OR name_type LIKE ?)")
        params.extend([f"%{query}%", f"%{query}%"])

    if feed_type:
        filters.append("feed_type = ?")
        params.append(feed_type)

    if filters:
        base_query += " WHERE " + " AND ".join(filters)

    count_query = f"SELECT COUNT(*) FROM ({base_query})"
    total = cur.execute(count_query, params).fetchone()[0]
    total_pages = (total + per_page - 1) // per_page

    final_query = f"{base_query} LIMIT ? OFFSET ?"
    rows = cur.execute(final_query, (*params, per_page, offset)).fetchall()

    conn.close()

    return jsonify({
        "weapons": [dict(r) for r in rows],
        "current_page": page,
        "total_pages": total_pages
    })

@app.route('/add_weapon', methods=['GET', 'POST'])
@login_required
@admin_required
def add_weapon():
    

    if request.method == 'POST':
        # Obter dados do formulário
        name = request.form['name']
        name_type = request.form['name_type']
        feed_type = request.form['feed_type']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        
        # Verificar se todos os campos necessários foram preenchidos
        if not name or not name_type or not feed_type or not slots or not width or not height:
            flash('Todos os campos são obrigatórios!', 'danger')
            return redirect(url_for('add_weapon'))

        # Validar o tipo de nome
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400

        try:
            # Conectar ao banco de dados e adicionar a arma
            conn = get_db_connection()
            conn.execute('INSERT INTO weapons (name, name_type, feed_type, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', 
                         (name, name_type, feed_type, slots, width, height, img))
            conn.commit()
            flash('Arma adicionada com sucesso!', 'success')

        except Exception as e:
            # Em caso de erro, enviar mensagem de erro
            flash(f'Ocorreu um erro ao adicionar a arma: {str(e)}', 'danger')
        
        finally:
            # Garantir que a conexão seja fechada
            conn.close()

        return redirect('/')

    return render_template('add_weapon.html')

@app.route('/edit_weapon/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def edit_weapon(id):
    
    
    conn = get_db_connection()
    
    try:
        # Verificar se a arma existe
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (id,)).fetchone()
        if not weapon:
            flash('Arma não encontrada!', 'danger')
            return redirect('/')
        
        # Obter dados adicionais
        magazines = conn.execute('SELECT * FROM magazines').fetchall()    
        attachments = conn.execute('SELECT * FROM attachments').fetchall()
        ammunitions = conn.execute("""
            SELECT
                ammunitions.id,
                ammunitions.name,
                ammunitions.name_type,
                calibers.name as caliber_name,
                ammunitions.slots,
                ammunitions.width,
                ammunitions.height,
                ammunitions.img
            FROM
                ammunitions
            JOIN
                calibers ON ammunitions.caliber_id = calibers.id;
        """).fetchall()
        
        weapon_ammunitions = conn.execute("""
            SELECT 
                weapon_ammunitions.weapon_id,
                ammunitions.id AS ammo_id,
                ammunitions.name AS ammo_name,
                ammunitions.name_type,
                ammunitions.caliber_id,
                ammunitions.slots,
                ammunitions.width,
                ammunitions.height,
                ammunitions.img,
                calibers.name AS caliber_name
            FROM weapon_ammunitions
            JOIN ammunitions ON weapon_ammunitions.ammo_id = ammunitions.id
            JOIN calibers ON ammunitions.caliber_id = calibers.id WHERE weapon_id = ?;
        """, (id,)).fetchall()
        
        weapon_magazines = conn.execute("""
            SELECT 
                weapon_magazines.weapon_id,
                magazines.id AS magazine_id,
                magazines.name,
                magazines.name_type,
                magazines.capacity,
                magazines.slots,
                magazines.width,
                magazines.height,
                magazines.img
            FROM 
                weapon_magazines
            JOIN 
                magazines ON weapon_magazines.magazine_id = magazines.id WHERE weapon_id = ?;
        """, (id,)).fetchall()
        
        weapon_attachments = conn.execute("""
            SELECT 
                weapon_attachments.weapon_id,
                attachments.id AS attachment_id,
                attachments.name,
                attachments.name_type,
                attachments.type,
                attachments.slots,
                attachments.width,
                attachments.height,
                attachments.img,
                attachments.battery
            FROM 
                weapon_attachments
            JOIN 
                attachments ON weapon_attachments.attachment_id = attachments.id WHERE weapon_id = ?;
        """, (id,)).fetchall()
        
        if request.method == 'POST':
            name = request.form['name']
            name_type = request.form['name_type']
            
            # Validação de tipo de nome
            if name_type not in type_names:
                return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400
            
            feed_type = request.form['feed_type']
            slots = request.form['slots']
            width = request.form['width']
            height = request.form['height']
            img = request.form['img']
            
            # Atualizar a arma no banco de dados
            conn.execute('UPDATE weapons SET name = ?, name_type = ?, feed_type = ?, slots = ?, width = ?, height = ?, img = ? WHERE id = ?', 
                         (name, name_type, feed_type, slots, width, height, img, id))
            conn.commit()
            flash('Arma atualizada com sucesso!', 'success')
            return redirect('/')
        
    except Exception as e:
        flash(f'Ocorreu um erro ao editar a arma: {str(e)}', 'danger')
    
    finally:
        conn.close()
    
    return render_template('edit_weapon.html', 
                           weapon=weapon, 
                           magazines=magazines, 
                           ammunitions=ammunitions, 
                           attachments=attachments, 
                           weapon_ammunitions=weapon_ammunitions, 
                           weapon_magazines=weapon_magazines, 
                           weapon_attachments=weapon_attachments)

@app.route('/delete_weapon/<int:id>')
@login_required
@admin_required
def delete_weapon(id):
    
    
    conn = get_db_connection()
    
    try:
        # Verifica se a arma existe antes de tentar excluir
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (id,)).fetchone()
        
        if not weapon:
            flash('Arma não encontrada!', 'danger')
            return redirect('/')
        
        # Exclui a arma
        conn.execute('DELETE FROM weapons WHERE id = ?', (id,))
        conn.commit()
        flash('Arma excluída com sucesso!', 'success')
    
    except Exception as e:
        conn.rollback()
        flash(f'Ocorreu um erro ao excluir a arma: {str(e)}', 'danger')
    
    finally:
        conn.close()

    return redirect('/')

# Explosivos
@app.route("/api/explosives_all")
@login_required
def api_explosives_all():
    conn = get_db_connection()
    cur = conn.cursor()
    explosives = cur.execute(f"SELECT * FROM explosives").fetchall()
    conn.close()

    return jsonify({
        "explosives": [dict(r) for r in explosives]
    })

@app.route("/api/explosives")
@login_required
def api_explosives():
    page = int(request.args.get("page", 1))
    query = request.args.get("q", "").strip()
    feed_type = request.args.get("feed_type", "").strip().lower()
    per_page = 6
    offset = (page - 1) * per_page

    conn = get_db_connection()
    cur = conn.cursor()

    base_query = "SELECT * FROM explosives"
    filters = []
    params = []

    if query:
        filters.append("(name LIKE ? OR name_type LIKE ?)")
        params.extend([f"%{query}%", f"%{query}%"])

    if filters:
        base_query += " WHERE " + " AND ".join(filters)

    count_query = f"SELECT COUNT(*) FROM ({base_query})"
    total = cur.execute(count_query, params).fetchone()[0]
    total_pages = (total + per_page - 1) // per_page

    final_query = f"{base_query} LIMIT ? OFFSET ?"
    rows = cur.execute(final_query, (*params, per_page, offset)).fetchall()

    conn.close()

    return jsonify({
        "explosives": [dict(r) for r in rows],
        "current_page": page,
        "total_pages": total_pages
    })

@app.route('/add_explosive', methods=['GET', 'POST'])
@login_required
@admin_required
def add_explosive():
    

    if request.method == 'POST':
        # Obter dados do formulário
        name = request.form['name']
        name_type = request.form['name_type']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        
        # Verificar se todos os campos necessários foram preenchidos
        if not name or not name_type or not slots or not width or not height:
            flash('Todos os campos são obrigatórios!', 'danger')
            return redirect(url_for('add_weapon'))

        # Validar o tipo de nome
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400

        try:
            # Conectar ao banco de dados e adicionar a arma
            conn = get_db_connection()
            conn.execute('INSERT INTO explosives (name, name_type, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?)', 
                         (name, name_type, slots, width, height, img))
            conn.commit()
            flash('Explosivo adicionado com sucesso!', 'success')

        except Exception as e:
            # Em caso de erro, enviar mensagem de erro
            flash(f'Ocorreu um erro ao adicionar a explosivo: {str(e)}', 'danger')
        
        finally:
            # Garantir que a conexão seja fechada
            conn.close()

        return redirect('/')

    return render_template('add_explosive.html')

@app.route('/edit_explosive/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def edit_explosive(id):
        
    conn = get_db_connection()
    
    try:
        # Verificar se a explosive existe
        explosive = conn.execute('SELECT * FROM explosives WHERE id = ?', (id,)).fetchone()
        if not explosive:
            flash('Explosivo não encontrada!', 'danger')
            return redirect('/')
       
        if request.method == 'POST':
            name = request.form['name']
            name_type = request.form['name_type']
            
            # Validação de tipo de nome
            if name_type not in type_names:
                return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400
            
            slots = request.form['slots']
            width = request.form['width']
            height = request.form['height']
            img = request.form['img']
            
            # Atualizar a arma no banco de dados
            conn.execute('UPDATE explosives SET name = ?, name_type = ?, slots = ?, width = ?, height = ?, img = ? WHERE id = ?', 
                         (name, name_type, slots, width, height, img, id))
            conn.commit()
            flash('Explosivo atualizada com sucesso!', 'success')
            return redirect('/')
        
    except Exception as e:
        flash(f'Ocorreu um erro ao editar a explosivo: {str(e)}', 'danger')
    
    finally:
        conn.close()
    
    return render_template('edit_explosive.html', 
                           explosive=explosive)

@app.route('/delete_explosive/<int:id>')
@login_required
@admin_required
def delete_explosive(id):
        
    conn = get_db_connection()
    
    try:
        # Verifica se a arma existe antes de tentar excluir
        explosive = conn.execute('SELECT * FROM explosives WHERE id = ?', (id,)).fetchone()
        
        if not weapon:
            flash('Explosivo não encontrado!', 'danger')
            return redirect('/')
        
        # Exclui a arma
        conn.execute('DELETE FROM explosives WHERE id = ?', (id,))
        conn.commit()
        flash('Explosivo excluída com sucesso!', 'success')
    
    except Exception as e:
        conn.rollback()
        flash(f'Ocorreu um erro ao excluir a explosivo: {str(e)}', 'danger')
    
    finally:
        conn.close()

    return redirect('/')

# Calibre
@app.route("/api/calibers")
@login_required
@admin_required
def api_calibers():
    page = int(request.args.get("page", 1))
    query = request.args.get("q", "").strip()
    per_page = 6
    offset = (page - 1) * per_page

    conn = get_db_connection()
    cur = conn.cursor()

    # Query base
    base_query = "SELECT * FROM calibers"
    filters = []
    params = []

    # Filtra pelo nome do calibre, se houver
    if query:
        filters.append("name LIKE ?")
        params.append(f"%{query}%")

    # Aplica os filtros, se existirem
    if filters:
        base_query += " WHERE " + " AND ".join(filters)

    # Query para contar o total de calibres
    count_query = f"SELECT COUNT(*) FROM ({base_query})"
    total = cur.execute(count_query, params).fetchone()[0]
    total_pages = (total + per_page - 1) // per_page

    # Query para obter os calibres paginados
    final_query = f"{base_query} LIMIT ? OFFSET ?"
    rows = cur.execute(final_query, (*params, per_page, offset)).fetchall()

    conn.close()

    # Retorna os calibres com informações de paginação
    return jsonify({
        "calibers": [dict(r) for r in rows],
        "current_page": page,
        "total_pages": total_pages
    })


@app.route('/add_caliber', methods=['GET', 'POST'])
@login_required
@admin_required
def add_caliber():
    
    
    conn = get_db_connection()
    
    if request.method == 'POST':
        name = request.form['name']
        
        # Validação simples para garantir que o nome não esteja vazio
        if not name:
            flash('O nome do calibre não pode estar vazio.', 'danger')
            return redirect(request.referrer)
        
        try:
            conn.execute('INSERT INTO calibers (name) VALUES (?)', (name,))
            conn.commit()
            flash('Calibre adicionado com sucesso!', 'success')
        except Exception as e:
            conn.rollback()
            flash(f'Ocorreu um erro ao adicionar o calibre: {str(e)}', 'danger')
        finally:
            conn.close()

        return redirect('/')
    
    # Caso a requisição seja GET, exibe o formulário
    return render_template('add_caliber.html')


@app.route('/edit_caliber/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def edit_caliber(id):
    
    
    conn = get_db_connection()
    
    # Recupera o calibre com o id fornecido
    caliber = conn.execute('SELECT * FROM calibers WHERE id = ?', (id,)).fetchone()
    
    # Caso o calibre não seja encontrado, redireciona
    if not caliber:
        flash('Calibre não encontrado.', 'danger')
        return redirect('/')
    
    if request.method == 'POST':
        # Recupera o novo nome enviado pelo formulário
        name = request.form['name']
        
        # Atualiza o nome do calibre
        conn.execute('UPDATE calibers SET name = ? WHERE id = ?', (name, id))
        conn.commit()
        
        # Redireciona para a página inicial após a atualização
        flash('Calibre atualizado com sucesso!', 'success')
        conn.close()
        return redirect('/')
    
    # Fecha a conexão quando a requisição não for POST
    conn.close()
    
    # Renderiza a página de edição com os dados do calibre
    return render_template('edit_caliber.html', caliber=caliber)


@app.route('/delete_caliber/<int:id>')
@login_required
@admin_required
def delete_caliber(id):
    
    
    conn = get_db_connection()
    try:
        # Deleta o calibre da tabela calibers
        conn.execute('DELETE FROM calibers WHERE id = ?', (id,))
        
        # Caso o calibre seja usado em munições, você pode optar por deletá-las ou desassociá-las
        conn.execute('UPDATE ammunitions SET caliber_id = NULL WHERE caliber_id = ?', (id,))
        
        conn.commit()
        flash('Calibre deletado com sucesso!', 'success')
    except Exception as e:
        flash(f'Ocorreu um erro ao deletar o calibre: {str(e)}', 'danger')
    finally:
        conn.close()
    
    return redirect('/')

# Munição
@app.route("/api/ammunitions")
@login_required
def api_ammunitions():
    page = int(request.args.get("page", 1))
    query = request.args.get("q", "").strip()
    per_page = 6
    offset = (page - 1) * per_page

    conn = get_db_connection()
    cur = conn.cursor()

    base_query = """
        SELECT a.*, c.name as caliber_name
        FROM ammunitions a
        JOIN calibers c ON a.caliber_id = c.id
    """
    filters = []
    params = []

    if query:
        filters.append("(a.name LIKE ? OR a.name_type LIKE ? OR c.name LIKE ?)")
        params.extend([f"%{query}%", f"%{query}%", f"%{query}%"])

    if filters:
        base_query += " WHERE " + " AND ".join(filters)

    count_query = f"SELECT COUNT(*) FROM ({base_query})"
    total = cur.execute(count_query, params).fetchone()[0]
    total_pages = (total + per_page - 1) // per_page

    final_query = f"{base_query} LIMIT ? OFFSET ?"
    rows = cur.execute(final_query, (*params, per_page, offset)).fetchall()

    conn.close()

    return jsonify({
        "ammunitions": [dict(r) for r in rows],
        "current_page": page,
        "total_pages": total_pages
    })


@app.route('/add_ammo', methods=['GET', 'POST'])
@login_required
@admin_required
def add_ammo():
    
    
    conn = get_db_connection()
    calibers = conn.execute('SELECT * FROM calibers').fetchall()

    if request.method == 'POST':
        name = request.form['name']
        name_type = request.form['name_type']
        
        # Valida o tipo de munição
        if name_type not in type_names:
            flash(f"'{name_type}' não é um item válido do types.xml.", 'danger')
            return redirect(request.url)
        
        # Validação adicional para os campos
        caliber_id = request.form['caliber_id']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        
        try:
            # Insere a munição no banco de dados
            conn.execute('INSERT INTO ammunitions (name, name_type, caliber_id, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', 
                         (name, name_type, caliber_id, slots, width, height, img))
            conn.commit()
            flash('Munição adicionada com sucesso!', 'success')
            return redirect('/')
        
        except Exception as e:
            # Trata erros durante a operação
            flash(f'Ocorreu um erro ao adicionar a munição: {str(e)}', 'danger')
            return redirect(request.url)
        
        finally:
            conn.close()

    return render_template('add_ammo.html', calibers=calibers)


@app.route('/edit_ammo/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def edit_ammo(id):
    
    
    conn = get_db_connection()
    
    try:
        # Verifica se a munição existe
        ammo = conn.execute('SELECT * FROM ammunitions WHERE id = ?', (id,)).fetchone()
        if not ammo:
            flash('Munição não encontrada.', 'danger')
            return redirect('/')
        
        calibers = conn.execute('SELECT * FROM calibers').fetchall()

        if request.method == 'POST':
            name = request.form['name']
            name_type = request.form['name_type']
            
            if name_type not in type_names:
                flash(f"'{name_type}' não é um item válido do types.xml.", 'danger')
                return redirect(request.url)
            
            caliber_id = request.form['caliber_id']
            slots = request.form['slots']
            width = request.form['width']
            height = request.form['height']
            img = request.form['img']
            
            # Atualiza os dados da munição
            conn.execute('UPDATE ammunitions SET name = ?, name_type = ?, caliber_id = ?, slots = ?, width = ?, height = ?, img = ? WHERE id = ?',
                         (name, name_type, caliber_id, slots, width, height, img, id))
            conn.commit()

            flash('Munição atualizada com sucesso!', 'success')
            return redirect('/')
    
    except Exception as e:
        # Captura qualquer erro e exibe uma mensagem
        flash(f'Ocorreu um erro ao editar a munição: {str(e)}', 'danger')
    
    finally:
        conn.close()

    return render_template('edit_ammo.html', ammo=ammo, calibers=calibers)


@app.route('/delete_ammo/<int:id>')
@login_required
@admin_required
def delete_ammo(id):
    

    conn = get_db_connection()
    try:
        # Verifica se o registro de munição existe
        ammo = conn.execute('SELECT * FROM ammunitions WHERE id = ?', (id,)).fetchone()
        if not ammo:
            flash('Munição não encontrada.', 'danger')
            return redirect('/')

        # Exclui a munição
        conn.execute('DELETE FROM ammunitions WHERE id = ?', (id,))
        conn.commit()

        flash('Munição excluída com sucesso!', 'success')
    except Exception as e:
        # Captura qualquer erro e exibe uma mensagem de erro
        flash(f'Ocorreu um erro ao excluir a munição: {str(e)}', 'danger')
    finally:
        conn.close()

    return redirect('/')

# Carregador
@app.route("/api/magazines")
@login_required
def api_magazines():
    page = int(request.args.get("page", 1))
    query = request.args.get("q", "").strip()
    per_page = 6
    offset = (page - 1) * per_page

    conn = get_db_connection()
    cur = conn.cursor()

    base_query = "SELECT * FROM magazines"
    filters = []
    params = []

    if query:
        filters.append("(name LIKE ? OR name_type LIKE ?)")
        params.extend([f"%{query}%", f"%{query}%"])

    if filters:
        base_query += " WHERE " + " AND ".join(filters)

    count_query = f"SELECT COUNT(*) FROM ({base_query})"
    total = cur.execute(count_query, params).fetchone()[0]
    total_pages = (total + per_page - 1) // per_page

    final_query = f"{base_query} LIMIT ? OFFSET ?"
    magazines = cur.execute(final_query, (*params, per_page, offset)).fetchall()

    conn.close()

    return jsonify({
        "magazines": [dict(m) for m in magazines],
        "current_page": page,
        "total_pages": total_pages
    })


@app.route('/add_magazine', methods=['GET', 'POST'])
@login_required
@admin_required
def add_magazine():
    
    
    conn = get_db_connection()
    
    try:
        ammunitions = conn.execute('SELECT * FROM ammunitions').fetchall()

        if request.method == 'POST':
            name = request.form['name']      
            name_type = request.form['name_type']  
            
            # Validação do name_type
            if name_type not in type_names:
                return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400   
            
            # Validação de dados
            try:
                capacity = int(request.form['capacity'])
                slots = int(request.form['slots'])
                width = int(request.form['width'])
                height = int(request.form['height'])
            except ValueError:
                flash('Capacidade, slots, largura e altura devem ser números inteiros.', 'danger')
                return redirect(request.url)
            
            img = request.form['img']

            # Inserção no banco de dados
            conn.execute('INSERT INTO magazines (name, name_type, capacity, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', 
                         (name, name_type, capacity, slots, width, height, img))
            conn.commit()

            flash('Carregador adicionado com sucesso!', 'success')
            return redirect('/')

    except Exception as e:
        flash(f'Ocorreu um erro ao adicionar o carregador: {str(e)}', 'danger')

    finally:
        conn.close()

    return render_template('add_magazine.html', ammunitions=ammunitions)


@app.route('/edit_magazine/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def edit_magazine(id):
    
    
    conn = get_db_connection()
    
    try:
        # Obter dados da revista
        magazine = conn.execute('SELECT * FROM magazines WHERE id = ?', (id,)).fetchone()
        if not magazine:
            flash('Revista não encontrada.', 'danger')
            return redirect('/')
        
        # Obter as munições disponíveis
        ammunitions = conn.execute('SELECT * FROM ammunitions').fetchall()

        if request.method == 'POST':
            # Coletando dados do formulário
            name = request.form['name']  
            name_type = request.form['name_type']
            
            # Validação de name_type
            if name_type not in type_names:
                return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400

            # Validações adicionais para outros campos
            try:
                capacity = int(request.form['capacity'])
                slots = int(request.form['slots'])
                width = int(request.form['width'])
                height = int(request.form['height'])
            except ValueError:
                flash('Os campos de capacidade, slots, largura e altura devem ser numéricos.', 'danger')
                return redirect(request.url)

            img = request.form['img']

            # Atualizando a revista no banco de dados
            conn.execute('UPDATE magazines SET name = ?, name_type = ?, capacity = ?, slots = ?, width = ?, height = ?, img = ? WHERE id = ?',
                         (name, name_type, capacity, slots, width, height, img, id))
            conn.commit()

            flash('Revista atualizada com sucesso!', 'success')
            return redirect('/')

    except Exception as e:
        flash(f'Ocorreu um erro ao editar a revista: {str(e)}', 'danger')
    
    finally:
        conn.close()

    return render_template('edit_magazine.html', magazine=magazine, ammunitions=ammunitions)


@app.route('/delete_magazine/<int:id>')
@login_required
@admin_required
def delete_magazine(id):
    
    
    conn = get_db_connection()
    try:
        # Verifica se a revista existe
        magazine = conn.execute('SELECT * FROM magazines WHERE id = ?', (id,)).fetchone()
        if not magazine:
            flash('Revista não encontrada.', 'danger')
            return redirect('/')
        
        # Executa a exclusão
        conn.execute('DELETE FROM magazines WHERE id = ?', (id,))
        conn.commit()
        flash('Revista excluída com sucesso!', 'success')
    except Exception as e:
        flash(f'Ocorreu um erro ao tentar excluir a revista: {str(e)}', 'danger')
    finally:
        conn.close()
    
    return redirect('/')


# Acessórios
@app.route('/api/attachments')
@login_required
@admin_required
def api_attachments():
    search_query = request.args.get('q', '')
    type_filter = request.args.get('type', '')
    page = int(request.args.get('page', 1))
    per_page = 6
    offset = (page - 1) * per_page

    db = get_db_connection()
    base_query = "SELECT * FROM attachments WHERE 1=1"
    count_query = "SELECT COUNT(*) FROM attachments WHERE 1=1"
    params = []
    
    if search_query:
        base_query += " AND (name LIKE ? OR name_type LIKE ?)"
        count_query += " AND (name LIKE ? OR name_type LIKE ?)"
        params += [f"%{search_query}%", f"%{search_query}%"]

    if type_filter:
        base_query += " AND type = ?"
        count_query += " AND type = ?"
        params.append(type_filter)

    total = db.execute(count_query, params).fetchone()[0]
    total_pages = (total + per_page - 1) // per_page

    base_query += " LIMIT ? OFFSET ?"
    params += [per_page, offset]
    rows = db.execute(base_query, params).fetchall()
    db.close()

    data = [{
        "id": row["id"],
        "name": row["name"],
        "name_type": row["name_type"],
        "type": row["type"],
        "battery": row["battery"],
        "slots": row["slots"],
        "width": row["width"],
        "height": row["height"],
        "img": row["img"]
    } for row in rows]

    return jsonify({
        "attachments": data,
        "total_pages": total_pages,
        "current_page": page
    })
    
@app.route('/add_attachment', methods=['GET', 'POST'])
@login_required
@admin_required
def add_attachment():
    
    
    conn = get_db_connection()
    if request.method == 'POST':
        try:
            # Coleta os dados do formulário
            name = request.form['name']
            name_type = request.form['name_type']
            if name_type not in type_names:
                flash(f"'{name_type}' não é um item válido do types.xml.", "danger")
                return redirect(request.referrer)

            type = request.form['type']
            slots = request.form['slots']
            width = request.form['width']
            height = request.form['height']
            img = request.form['img']
            battery = request.form.get('battery')
            
            # Validação de campos obrigatórios
            if not all([name, name_type, type, slots, width, height, img, battery]):
                flash("Todos os campos devem ser preenchidos.", "danger")
                return redirect(request.referrer)

            # Inserção no banco de dados
            conn.execute('INSERT INTO attachments (name, name_type, type, slots, width, height, img, battery) VALUES (?, ?, ?, ?, ?, ?, ?, ?)', 
                         (name, name_type, type, slots, width, height, img, battery))
            conn.commit()
            flash("Anexo adicionado com sucesso!", "success")
        except Exception as e:
            flash(f"Ocorreu um erro ao adicionar o anexo: {str(e)}", "danger")
        finally:
            conn.close()
        
        return redirect('/')

    # Exibe o formulário de adição de anexo (GET)
    return render_template('add_attachment.html')

@app.route('/edit_attachment/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def edit_attachment(id):
    
    
    conn = get_db_connection()
    
    # Verificar se o anexo existe
    attachment = conn.execute('SELECT * FROM attachments WHERE id = ?', (id,)).fetchone()
    
    if not attachment:
        flash("Anexo não encontrado.", "danger")
        conn.close()
        return redirect(request.referrer or url_for('index'))
    
    if request.method == 'POST':
        try:
            # Coletar dados do formulário
            name = request.form['name']
            name_type = request.form['name_type']
            if name_type not in type_names:
                flash(f"'{name_type}' não é um item válido do types.xml.", "danger")
                return redirect(request.referrer)
            
            type = request.form['type']
            slots = request.form['slots']
            width = request.form['width']
            height = request.form['height']
            img = request.form['img']
            battery = request.form.get('battery')
            
            
            # Atualizar o anexo no banco de dados
            conn.execute('UPDATE attachments SET name = ?, name_type = ?, type = ?, slots = ?, width = ?, height = ?, img = ?, battery = ? WHERE id = ?',
                         (name, name_type, type, slots, width, height, img, battery, id))
            conn.commit()
            flash("Anexo atualizado com sucesso!", "success")
        except Exception as e:
            flash(f"Ocorreu um erro ao atualizar o anexo: {str(e)}", "danger")
        finally:
            conn.close()
        
        return redirect('/')

    # Se a requisição for GET, exibe o formulário com os dados do anexo
    conn.close()
    return render_template('edit_attachment.html', attachment=attachment)

@app.route('/delete_attachment/<int:id>', methods=['GET', 'POST'])
@login_required
@admin_required
def delete_attachment(id):
    
    
    conn = get_db_connection()
    
    # Verificar se o anexo existe antes de tentar deletar
    attachment = conn.execute('SELECT * FROM attachments WHERE id = ?', (id,)).fetchone()
    
    if not attachment:
        flash("Anexo não encontrado.", "danger")
        conn.close()
        return redirect(request.referrer or url_for('index'))  # Redirecionar para a página anterior ou página inicial
    
    try:
        # Realiza a exclusão do anexo
        conn.execute('DELETE FROM attachments WHERE id = ?', (id,))
        conn.commit()
        flash("Anexo excluído com sucesso!", "success")
    except Exception as e:
        flash(f"Ocorreu um erro ao excluir o anexo: {str(e)}", "danger")
    
    conn.close()
    return redirect(request.referrer or url_for('index'))  # Redireciona para a página anterior ou página inicial

# weapon_ammunitions
@app.route('/add_weapon_ammunitions', methods=['GET', 'POST'])
@login_required
@admin_required
def add_weapon_ammunitions():
    
    
    conn = get_db_connection()
    if request.method == 'POST':      
        weapon_id = request.form['weapon_id']      
        ammo_id = request.form['ammo_id']  

        # Verificando se a arma e a munição existem
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
        ammo = conn.execute('SELECT * FROM ammunitions WHERE id = ?', (ammo_id,)).fetchone()

        if not weapon:
            flash("A arma especificada não existe.", "danger")
            return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial ou referrer
        
        if not ammo:
            flash("A munição especificada não existe.", "danger")
            return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial ou referrer
        
        try:
            # Inserindo a associação entre a arma e a munição
            conn.execute('INSERT INTO weapon_ammunitions (weapon_id, ammo_id) VALUES (?, ?)', (weapon_id, ammo_id))
            conn.commit()
            flash("Associação de munição adicionada com sucesso!", "success")
        except Exception as e:
            flash(f"Ocorreu um erro ao adicionar a associação: {str(e)}", "danger")
        
        conn.close()
        return redirect(request.referrer or url_for('index'))
    
    return redirect(request.referrer or url_for('index'))

@app.route('/delete_weapon_ammunitions/<int:ammo_id>/<int:weapon_id>', methods=['GET'])
@login_required
def delete_weapon_ammunitions(ammo_id, weapon_id):
    
    
    conn = get_db_connection()

    # Verificar se a arma e a munição existem
    weapon = conn.execute('SELECT id FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
    ammo = conn.execute('SELECT id FROM ammunitions WHERE id = ?', (ammo_id,)).fetchone()

    if not weapon:
        flash("A arma especificada não existe.", "danger")
        return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial ou referrer
    
    if not ammo:
        flash("A munição especificada não existe.", "danger")
        return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial ou referrer

    try:
        # Excluir a associação entre a munição e a arma
        conn.execute('DELETE FROM weapon_ammunitions WHERE ammo_id = ? AND weapon_id = ?', (ammo_id, weapon_id))
        conn.commit()
        flash("Associação de munição excluída com sucesso!", "success")
    except Exception as e:
        flash(f"Ocorreu um erro ao excluir a associação: {str(e)}", "danger")
    
    conn.close()

    return redirect(request.referrer or url_for('index'))

# weapon_magazines
@app.route('/add_weapon_magazines', methods=['GET', 'POST'])
@login_required
@admin_required
def add_weapon_magazines():
    
    
    conn = get_db_connection()
    
    if request.method == 'POST':      
        weapon_id = request.form['weapon_id']      
        magazine_id = request.form['magazine_id']
        
        # Verificar se a arma e o carregador existem no banco
        weapon = conn.execute('SELECT id FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
        magazine = conn.execute('SELECT id FROM magazines WHERE id = ?', (magazine_id,)).fetchone()

        if not weapon:
            flash("A arma especificada não existe.", "danger")
            return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial ou referrer
        
        if not magazine:
            flash("O carregador especificado não existe.", "danger")
            return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial ou referrer

        try:
            # Inserir o carregador para a arma no banco de dados
            conn.execute('INSERT INTO weapon_magazines (weapon_id, magazine_id) VALUES (?, ?)', (weapon_id, magazine_id))
            conn.commit()
            flash("Carregador adicionado à arma com sucesso!", "success")
        except Exception as e:
            flash(f"Ocorreu um erro ao adicionar o carregador: {str(e)}", "danger")
        
        conn.close()
        return redirect(request.referrer or url_for('index'))
    
    # Se o método não for POST, redireciona o usuário
    return redirect(request.referrer or url_for('index'))

@app.route('/delete_weapon_magazines/<int:magazine_id>/<int:weapon_id>')
@login_required
@admin_required
def delete_weapon_magazines(magazine_id, weapon_id):
    
    
    conn = get_db_connection()
    
    try:
        # Verificar se o registro existe antes de deletar
        weapon_magazine = conn.execute(
            'SELECT * FROM weapon_magazines WHERE magazine_id = ? AND weapon_id = ?',
            (magazine_id, weapon_id)
        ).fetchone()

        if not weapon_magazine:
            flash("Carregador não encontrado para esta arma.", "danger")
            return redirect(request.referrer or url_for('index'))  # Redireciona para a página inicial se o referrer não existir

        # Deletar o registro
        conn.execute('DELETE FROM weapon_magazines WHERE magazine_id = ? AND weapon_id = ?', (magazine_id, weapon_id))
        conn.commit()
        flash("Carregador excluído com sucesso!", "success")
    except Exception as e:
        flash(f"Ocorreu um erro ao tentar excluir o carregador: {str(e)}", "danger")
    finally:
        conn.close()
    
    # Redireciona para a página anterior (ou a página inicial se o referrer não estiver presente)
    return redirect(request.referrer or url_for('index'))

# weapon_attachments
@app.route('/add_weapon_attachments', methods=['GET', 'POST'])
@login_required
@admin_required
def add_weapon_attachments():
    
    
    conn = get_db_connection()
    
    if request.method == 'POST':      
        weapon_id = request.form['weapon_id']      
        attachment_id = request.form['attachment_id']  
        
        # Verificar se o weapon_id existe na tabela de armas
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
        if not weapon:
            flash("Arma não encontrada!", "danger")
            return redirect(request.referrer)
        
        # Verificar se o attachment_id existe na tabela de acessórios
        attachment = conn.execute('SELECT * FROM attachments WHERE id = ?', (attachment_id,)).fetchone()
        if not attachment:
            flash("Acessório não encontrado!", "danger")
            return redirect(request.referrer)
        
        # Inserir a associação na tabela de weapon_attachments
        try:
            conn.execute('INSERT INTO weapon_attachments (weapon_id, attachment_id) VALUES (?, ?)', (weapon_id, attachment_id))
            conn.commit()
            flash("Acessório adicionado à arma com sucesso!", "success")
        except Exception as e:
            flash(f"Erro ao adicionar o acessório: {str(e)}", "danger")
        
        conn.close()
        return redirect(request.referrer)
    
    return redirect(request.referrer)

@app.route('/delete_weapon_attachments/<int:attachment_id>/<int:weapon_id>')
@login_required
@admin_required
def delete_weapon_attachments(attachment_id, weapon_id):
    
    
    conn = get_db_connection()
    
    # Verificar se o attachment e weapon existem
    attachment = conn.execute('SELECT * FROM attachments WHERE id = ?', (attachment_id,)).fetchone()
    weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
    
    if not attachment or not weapon:
        conn.close()
        flash("Arma ou acessório não encontrado.", "danger")
        return redirect(request.referrer or url_for('home'))
    
    # Deletar a associação
    conn.execute('DELETE FROM weapon_attachments WHERE attachment_id = ? AND weapon_id = ?', (attachment_id, weapon_id))
    conn.commit()
    
    # Feedback para o usuário
    if conn.total_changes > 0:
        flash("Acessório removido com sucesso.", "success")
    else:
        flash("Erro ao tentar remover o acessório. Ele pode já ter sido removido.", "danger")
    
    conn.close()
    
    return redirect(request.referrer or url_for('home'))

@app.route('/loadout_rules', methods=['GET', 'POST'])
@login_required
@admin_required
def loadout_rules():
    conn = get_db_connection()
    weapons = conn.execute('SELECT * FROM weapons').fetchall()
    if request.method == 'POST':
        data = request.form
        weapon_id = data['weapon_id']
        is_banned = int(data.get('is_banned', 0))
        
        conn.execute('''
            INSERT INTO loadout_rules_weapons (weapon_id, is_banned)
            VALUES (?, ?)
            ON CONFLICT(weapon_id) DO UPDATE SET
                is_banned = excluded.is_banned;
        ''', (weapon_id, is_banned))
        conn.commit()
        conn.close()
        return redirect('/loadout_rules')

    # Método GET
    rules = conn.execute('''
        SELECT r.*, w.name FROM loadout_rules_weapons r
        JOIN weapons w ON r.weapon_id = w.id
    ''').fetchall()
    conn.close()
    return render_template('loadout_rules.html', rules=rules, weapons=weapons)

@app.route('/delete_loadout_rules_weapons', methods=['GET'])
@login_required
@admin_required
def delete_loadout_rules_weapons():
    
    
    conn = get_db_connection()
    
    try:
        # Realiza a exclusão do anexo
        conn.execute('DELETE FROM loadout_rules_weapons')
        conn.commit()
        flash("Anexo excluído com sucesso!", "success")
    except Exception as e:
        flash(f"Ocorreu um erro ao excluir o anexo: {str(e)}", "danger")
    
    conn.close()
    return redirect(request.referrer or url_for('index'))  # Redireciona para a página anterior ou página inicial

def get_player_weapons(player_id):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT 
            plw.id AS loadout_id,
            plw.player_id,
            wp_primary.id AS primary_weapon_id,
            wp_primary.name AS primary_weapon_name,
            wp_primary.img AS primary_weapon_img,
            mag_primary.id AS primary_magazine_id,
            mag_primary.name AS primary_magazine_name,
            mag_primary.img AS primary_magazine_img,
            ammo_primary.id AS primary_ammo_id,
            ammo_primary.name AS primary_ammo_name,
            ammo_primary.img AS primary_ammo_img,
            wp_secondary.id AS secondary_weapon_id,
            wp_secondary.name AS secondary_weapon_name,
            wp_secondary.img AS secondary_weapon_img,
            mag_secondary.id AS secondary_magazine_id,
            mag_secondary.name AS secondary_magazine_name,
            mag_secondary.img AS secondary_magazine_img,
            ammo_secondary.id AS secondary_ammo_id,
            ammo_secondary.name AS secondary_ammo_name,
            ammo_secondary.img AS secondary_ammo_img,
            wp_small.id AS small_weapon_id,
            wp_small.name AS small_weapon_name,
            wp_small.img AS small_weapon_img,
            mag_small.id AS small_magazine_id,
            mag_small.name AS small_magazine_name,
            mag_small.img AS small_magazine_img,
            ammo_small.id AS small_ammo_id,
            ammo_small.name AS small_ammo_name,
            ammo_small.img AS small_ammo_img
        FROM player_loadouts_weapons plw
        LEFT JOIN weapons wp_primary ON plw.primary_weapon_id = wp_primary.id
        LEFT JOIN weapons wp_secondary ON plw.secondary_weapon_id = wp_secondary.id
        LEFT JOIN weapons wp_small ON plw.small_weapon_id = wp_small.id
        LEFT JOIN magazines mag_primary ON plw.primary_magazine_id = mag_primary.id
        LEFT JOIN magazines mag_secondary ON plw.secondary_magazine_id = mag_secondary.id
        LEFT JOIN magazines mag_small ON plw.small_magazine_id = mag_small.id
        LEFT JOIN ammunitions ammo_primary ON plw.primary_ammo_id = ammo_primary.id
        LEFT JOIN ammunitions ammo_secondary ON plw.secondary_ammo_id = ammo_secondary.id
        LEFT JOIN ammunitions ammo_small ON plw.small_ammo_id = ammo_small.id
        WHERE plw.player_id = ?
    """, (player_id,))
    weapons = cursor.fetchall()
    return weapons

@app.route('/players/<player_id>/weapons', methods=['GET'])
@login_required
def list_player_weapons(player_id):
    try:
        weapons = get_player_weapons(player_id)
        return jsonify([dict(weapon) for weapon in weapons])
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/player_loadout_weapons/<player_id>', methods=['GET', 'POST'])
@login_required
def player_loadout_weapons(player_id):
    conn = get_db_connection()
    conn2 = get_db_connection_players_beco_c1()

    if request.method == 'POST':
        primary = request.form.get('primary_weapon_id')
        primary_magazine = request.form.get('primary_magazine_id')
        primary_ammo = request.form.get('primary_ammo_id')

        secondary = request.form.get('secondary_weapon_id')
        secondary_magazine = request.form.get('secondary_magazine_id')
        secondary_ammo = request.form.get('secondary_ammo_id')

        small = request.form.get('small_weapon_id')
        small_magazine = request.form.get('small_magazine_id')
        small_ammo = request.form.get('small_ammo_id')

        # Validação das armas
        errors = []
        for weapon_id, slot_type in [
            (primary, 'primary'),
            (secondary, 'secondary'),
            (small, 'small')
        ]:
            if weapon_id:
                rule = conn.execute('SELECT * FROM loadout_rules_weapons WHERE weapon_id = ?', (weapon_id,)).fetchone()
                if not rule:
                    errors.append(f"A arma selecionada para {slot_type} é inválida.")
                elif rule['is_banned']:
                    errors.append(f"A arma selecionada para {slot_type} está banida.")

        if errors:
            for err in errors:
                flash(err, 'danger')
            return redirect(request.url)

        conn.execute('''
            INSERT INTO player_loadouts_weapons (
                player_id,
                primary_weapon_id, primary_magazine_id, primary_ammo_id,
                secondary_weapon_id, secondary_magazine_id, secondary_ammo_id,
                small_weapon_id, small_magazine_id, small_ammo_id
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(player_id) DO UPDATE SET
                primary_weapon_id = excluded.primary_weapon_id,
                primary_magazine_id = excluded.primary_magazine_id,
                primary_ammo_id = excluded.primary_ammo_id,                
                secondary_weapon_id = excluded.secondary_weapon_id,
                secondary_magazine_id = excluded.secondary_magazine_id,
                secondary_ammo_id = excluded.secondary_ammo_id,                
                small_weapon_id = excluded.small_weapon_id,
                small_magazine_id = excluded.small_magazine_id,
                small_ammo_id = excluded.small_ammo_id;
        ''', (
            player_id,
            primary, primary_magazine, primary_ammo,
            secondary, secondary_magazine, secondary_ammo,
            small, small_magazine, small_ammo
        ))

        conn.commit()
        conn.close()
        conn2.close()
        flash("Loadout atualizado com sucesso!", "success")
        return redirect(url_for('player_loadout', player_id=player_id))

    weapons = conn.execute('''
        SELECT w.*, r.is_banned
        FROM weapons w
        LEFT JOIN loadout_rules_weapons r ON w.id = r.weapon_id
    ''').fetchall()

    # Criar um dicionário de loadout padrão
    default_loadout = {
        'loadout_id': None,
        'player_id': player_id,
        'primary_weapon_id': None,
        'primary_weapon_name': None,
        'primary_weapon_img': None,
        'primary_magazine_id': None,
        'primary_magazine_name': None,
        'primary_magazine_img': None,
        'primary_ammo_id': None,
        'primary_ammo_name': None,
        'primary_ammo_img': None,
        'secondary_weapon_id': None,
        'secondary_weapon_name': None,
        'secondary_weapon_img': None,
        'secondary_magazine_id': None,
        'secondary_magazine_name': None,
        'secondary_magazine_img': None,
        'secondary_ammo_id': None,
        'secondary_ammo_name': None,
        'secondary_ammo_img': None,
        'small_weapon_id': None,
        'small_weapon_name': None,
        'small_weapon_img': None,
        'small_magazine_id': None,
        'small_magazine_name': None,
        'small_magazine_img': None,
        'small_ammo_id': None,
        'small_ammo_name': None,
        'small_ammo_img': None
    }

    # Buscar loadout do banco de dados
    db_loadout = conn.execute("""
        SELECT 
            plw.id AS loadout_id,
            plw.player_id,
            wp_primary.id AS primary_weapon_id,
            wp_primary.name AS primary_weapon_name,
            wp_primary.img AS primary_weapon_img,
            mag_primary.id AS primary_magazine_id,
            mag_primary.name AS primary_magazine_name,
            mag_primary.img AS primary_magazine_img,
            ammo_primary.id AS primary_ammo_id,
            ammo_primary.name AS primary_ammo_name,
            ammo_primary.img AS primary_ammo_img,
            wp_secondary.id AS secondary_weapon_id,
            wp_secondary.name AS secondary_weapon_name,
            wp_secondary.img AS secondary_weapon_img,
            mag_secondary.id AS secondary_magazine_id,
            mag_secondary.name AS secondary_magazine_name,
            mag_secondary.img AS secondary_magazine_img,
            ammo_secondary.id AS secondary_ammo_id,
            ammo_secondary.name AS secondary_ammo_name,
            ammo_secondary.img AS secondary_ammo_img,
            wp_small.id AS small_weapon_id,
            wp_small.name AS small_weapon_name,
            wp_small.img AS small_weapon_img,
            mag_small.id AS small_magazine_id,
            mag_small.name AS small_magazine_name,
            mag_small.img AS small_magazine_img,
            ammo_small.id AS small_ammo_id,
            ammo_small.name AS small_ammo_name,
            ammo_small.img AS small_ammo_img
        FROM player_loadouts_weapons plw
        LEFT JOIN weapons wp_primary ON plw.primary_weapon_id = wp_primary.id
        LEFT JOIN weapons wp_secondary ON plw.secondary_weapon_id = wp_secondary.id
        LEFT JOIN weapons wp_small ON plw.small_weapon_id = wp_small.id
        LEFT JOIN magazines mag_primary ON plw.primary_magazine_id = mag_primary.id
        LEFT JOIN magazines mag_secondary ON plw.secondary_magazine_id = mag_secondary.id
        LEFT JOIN magazines mag_small ON plw.small_magazine_id = mag_small.id
        LEFT JOIN ammunitions ammo_primary ON plw.primary_ammo_id = ammo_primary.id
        LEFT JOIN ammunitions ammo_secondary ON plw.secondary_ammo_id = ammo_secondary.id
        LEFT JOIN ammunitions ammo_small ON plw.small_ammo_id = ammo_small.id
        WHERE plw.player_id = ?
    """, (player_id,)).fetchone()

    # Combinar o loadout padrão com os dados do banco (se existirem)
    loadout = default_loadout
    if db_loadout:
        loadout.update(db_loadout)

    # Buscar attachments apenas se houver um loadout_id
    attachments_by_slot = {'primary': [], 'secondary': [], 'small': []}
    if loadout['loadout_id'] is not None:
        attachments_raw = conn.execute('''
            SELECT a.*, plwa.weapon_slot
            FROM player_loadouts_weapon_attachments plwa
            JOIN attachments a ON plwa.attachment_id = a.id
            WHERE plwa.player_loadouts_weapons_id = ?
        ''', (loadout['loadout_id'],)).fetchall()
        
        # Agrupar attachments por slot
        for att in attachments_raw:
            slot = att['weapon_slot']
            if slot in attachments_by_slot:
                attachments_by_slot[slot].append(att)
    
    # Buscar explosives apenas se houver um loadout_id
    explosives = conn.execute('''
            SELECT a.*, plwa.quantity
            FROM player_loadouts_weapon_explosives plwa
            JOIN explosives a ON plwa.explosive_id = a.id
            WHERE plwa.player_loadouts_weapons_id = ?
        ''', (loadout['loadout_id'],)).fetchall()

    player = conn2.execute('SELECT * FROM players_database WHERE PlayerID = ?', (player_id,)).fetchone()
    rules = conn.execute('''
        SELECT r.*, w.name FROM loadout_rules_weapons r
        JOIN weapons w ON r.weapon_id = w.id
    ''').fetchall()

    conn.close()
    conn2.close()

    return render_template(
        'player_loadout_weapons.html',
        player=player,
        weapons=weapons,
        loadout=loadout,
        rules=rules,
        attachments=attachments_by_slot,
        explosives=explosives
    )

@app.route('/delete_loadout_weapons/<player_id>', methods=['GET'])
@login_required
def delete_loadout_weapons(player_id):
    conn = get_db_connection()
    
    try:
        # Primeiro deleta os attachments (se houver relacionamento)
        conn.execute('DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id IN (SELECT id FROM player_loadouts_weapons WHERE player_id = ?)', (player_id,))
        conn.execute('DELETE FROM player_loadouts_weapon_explosives WHERE player_loadouts_weapons_id IN (SELECT id FROM player_loadouts_weapons WHERE player_id = ?)', (player_id,))
        
        # Depois deleta o loadout principal
        conn.execute('DELETE FROM player_loadouts_weapons WHERE player_id = ?', (player_id,))
        
        conn.commit()
        flash('Loadout excluído com sucesso!', 'success')
    except Exception as e:
        conn.rollback()
        flash(f'Erro ao excluir loadout: {str(e)}', 'danger')
    finally:
        conn.close()
    
    return redirect(url_for('player_loadout_weapons', player_id=player_id))

@app.route('/loadout_players', methods=['GET'])
@login_required
def loadout_players():
    conn = get_db_connection_players_beco_c1()
    players = conn.execute('SELECT * FROM players_database').fetchall()
    conn.close()
    return render_template('loadout_players.html', players=players)

@app.route('/get_magazines/<int:weapon_id>')
@login_required
def get_magazines(weapon_id):
    conn = get_db_connection()
    query = '''
        SELECT m.*
        FROM magazines m
        JOIN weapon_magazines wm ON wm.magazine_id = m.id
        WHERE wm.weapon_id = ?
    '''
    magazines = conn.execute(query, (weapon_id,)).fetchall()
    conn.close()

    return jsonify([{'id': m['id'], 'name': m['name'], 'img': m['img'], 'capacity': m['capacity'], 'slots': m['slots'], 'width': m['width'], 'height': m['height']} for m in magazines])

@app.route('/get_ammos/<int:weapon_id>')
@login_required
def get_ammos(weapon_id):
    conn = get_db_connection()
    query = '''
        SELECT m.*
        FROM ammunitions m
        JOIN weapon_ammunitions wm ON wm.ammo_id = m.id
        WHERE wm.weapon_id = ?
    '''
    ammos = conn.execute(query, (weapon_id,)).fetchall()
    conn.close()

    return jsonify([{'id': m['id'], 'name': m['name'], 'img': m['img'], 'slots': m['slots'], 'width': m['width'], 'height': m['height']} for m in ammos])

@app.route('/get_attachments/<int:weapon_id>')
@login_required
def weapon_attachments(weapon_id):
    conn = get_db_connection()
    attachments = conn.execute('''
        SELECT a.*
        FROM attachments a
        JOIN weapon_attachments wa ON wa.attachment_id = a.id
        WHERE wa.weapon_id = ?
    ''', (weapon_id,)).fetchall()
    conn.close()

    return jsonify([dict(a) for a in attachments])

@app.route('/save_loadout_weapons/<player_id>', methods=['POST'])
@login_required
def save_loadout_weapons(player_id):
    conn = get_db_connection()

    try:
        # Obtém os dados do formulário
        primary_weapon_id = request.form.get('primary_weapon_id') or None
        primary_magazine_id = request.form.get('primary_magazine_id') or None
        primary_ammo_id = request.form.get('primary_ammo_id') or None

        secondary_weapon_id = request.form.get('secondary_weapon_id') or None
        secondary_magazine_id = request.form.get('secondary_magazine_id') or None
        secondary_ammo_id = request.form.get('secondary_ammo_id') or None

        small_weapon_id = request.form.get('small_weapon_id') or None
        small_magazine_id = request.form.get('small_magazine_id') or None
        small_ammo_id = request.form.get('small_ammo_id') or None

        def parse(val): return int(val) if val else None

        # Verifica se os dados de loadout já existem
        existing_loadout = conn.execute('SELECT * FROM player_loadouts_weapons WHERE player_id = ?', (player_id,)).fetchone()

        # Prepara a consulta para inserir ou atualizar
        if existing_loadout:            
            print("Loadout ja existe")
            player_loadouts_weapons_id = existing_loadout['id']
            print("player_loadouts_weapons_id: " + str(player_loadouts_weapons_id))

            update_data = []
            update_query = "UPDATE player_loadouts_weapons SET "

            # Se primary_weapon_id for diferente do existing_loadout['primary_weapon_id'] - atualize magazine, ammo e attachs (deletar tudo da tabela antes) mesmo que esteja nulos

            if primary_weapon_id:
                if (primary_weapon_id != existing_loadout['primary_weapon_id']):
                    conn.execute("DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id = ? and weapon_slot = 'primary'", (player_loadouts_weapons_id,))
                    update_query += "primary_magazine_id = ?, "
                    update_data.append(parse(primary_magazine_id))
                    update_query += "primary_ammo_id = ?, "
                    update_data.append(parse(primary_ammo_id))
                update_query += "primary_weapon_id = ?, "
                update_data.append(parse(primary_weapon_id))
            if primary_magazine_id:
                update_query += "primary_magazine_id = ?, "
                update_data.append(parse(primary_magazine_id))
            if primary_ammo_id:
                update_query += "primary_ammo_id = ?, "
                update_data.append(parse(primary_ammo_id))
            attachment_ids = request.form.getlist(f'primary_attachments')
            if (attachment_ids):
                conn.execute("DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id = ? and weapon_slot = 'primary'", (player_loadouts_weapons_id,))
                for aid in attachment_ids:
                    conn.execute(''' 
                        INSERT INTO player_loadouts_weapon_attachments (player_loadouts_weapons_id, attachment_id, weapon_slot)
                        VALUES (?, ?, ?)
                    ''', (player_loadouts_weapons_id, int(aid), "primary"))

            if secondary_weapon_id:
                if (secondary_weapon_id != existing_loadout['secondary_weapon_id']):
                    conn.execute("DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id = ? and weapon_slot = 'secondary'", (player_loadouts_weapons_id,))
                    update_query += "secondary_magazine_id = ?, "
                    update_data.append(parse(secondary_magazine_id))
                    update_query += "secondary_ammo_id = ?, "
                    update_data.append(parse(secondary_ammo_id))
                update_query += "secondary_weapon_id = ?, "
                update_data.append(parse(secondary_weapon_id))
            if secondary_magazine_id:
                update_query += "secondary_magazine_id = ?, "
                update_data.append(parse(secondary_magazine_id))
            if secondary_ammo_id:
                update_query += "secondary_ammo_id = ?, "
                update_data.append(parse(secondary_ammo_id))
            attachment_ids = request.form.getlist(f'secondary_attachments')
            if (attachment_ids):
                conn.execute("DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id = ? and weapon_slot = 'secondary'", (player_loadouts_weapons_id,))
                for aid in attachment_ids:
                    conn.execute(''' 
                        INSERT INTO player_loadouts_weapon_attachments (player_loadouts_weapons_id, attachment_id, weapon_slot)
                        VALUES (?, ?, ?)
                    ''', (player_loadouts_weapons_id, int(aid), "secondary"))

            if small_weapon_id:
                if (small_weapon_id != existing_loadout['small_weapon_id']):
                    conn.execute("DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id = ? and weapon_slot = 'small'", (player_loadouts_weapons_id,))
                    update_query += "small_magazine_id = ?, "
                    update_data.append(parse(small_magazine_id))
                    update_query += "small_ammo_id = ?, "
                    update_data.append(parse(small_ammo_id))
                update_query += "small_weapon_id = ?, "
                update_data.append(parse(small_weapon_id))
            if small_magazine_id:
                update_query += "small_magazine_id = ?, "
                update_data.append(parse(small_magazine_id))
            if small_ammo_id:
                update_query += "small_ammo_id = ?, "
                update_data.append(parse(small_ammo_id))
            attachment_ids = request.form.getlist(f'small_attachments')
            if (attachment_ids):
                conn.execute("DELETE FROM player_loadouts_weapon_attachments WHERE player_loadouts_weapons_id = ? and weapon_slot = 'small'", (player_loadouts_weapons_id,))
                for aid in attachment_ids:
                    conn.execute(''' 
                        INSERT INTO player_loadouts_weapon_attachments (player_loadouts_weapons_id, attachment_id, weapon_slot)
                        VALUES (?, ?, ?)
                    ''', (player_loadouts_weapons_id, int(aid), "small"))
            explosives_json = request.form.get('explosives')
            if (explosives_json):
                conn.execute("DELETE FROM player_loadouts_weapon_explosives WHERE player_loadouts_weapons_id = ?", (player_loadouts_weapons_id,))              
                explosives = json.loads(explosives_json) if explosives_json else []
                for explosive in explosives:
                    conn.execute('''
                        INSERT INTO player_loadouts_weapon_explosives (player_loadouts_weapons_id, explosive_id, quantity)
                        VALUES (?, ?, ?)
                    ''', (
                        player_loadouts_weapons_id,
                        int(explosive['id']),
                        int(explosive['quantity'])
                    ))
            

            # Remove a vírgula extra no final da consulta de atualização
            update_query = update_query.rstrip(', ') + " WHERE player_id = ?"            
            update_data.append(player_id)
            print("update_query: " + update_query)

            # Realiza o update
            if (update_query != "UPDATE player_loadouts_weapons SET WHERE player_id = ?"):
                conn.execute(update_query, tuple(update_data))

            print("Realizou update")
        else:
            print("Loadout não ja existe")
            # Caso o loadout não exista, insere um novo
            conn.execute(''' 
                INSERT INTO player_loadouts_weapons (
                    player_id, primary_weapon_id, primary_magazine_id, primary_ammo_id,
                    secondary_weapon_id, secondary_magazine_id, secondary_ammo_id,
                    small_weapon_id, small_magazine_id, small_ammo_id
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (
                player_id,
                parse(primary_weapon_id), parse(primary_magazine_id), parse(primary_ammo_id),
                parse(secondary_weapon_id), parse(secondary_magazine_id), parse(secondary_ammo_id),
                parse(small_weapon_id), parse(small_magazine_id), parse(small_ammo_id)
            ))
            # Recupera o ID do loadout
            row = conn.execute('SELECT id FROM player_loadouts_weapons WHERE player_id = ?', (player_id,)).fetchone()
            player_loadouts_weapons_id = row['id']
            # Loop para cada slot de arma
            for slot in ['primary', 'secondary', 'small']:
                attachment_ids = request.form.getlist(f'{slot}_attachments')
                for aid in attachment_ids:
                    conn.execute(''' 
                        INSERT INTO player_loadouts_weapon_attachments (player_loadouts_weapons_id, attachment_id, weapon_slot)
                        VALUES (?, ?, ?)
                    ''', (player_loadouts_weapons_id, int(aid), slot))  

            explosives_json = request.form.get('explosives')
            explosives = json.loads(explosives_json) if explosives_json else []
            for explosive in explosives:
                conn.execute('''
                    INSERT INTO player_loadouts_weapon_explosives (player_loadouts_weapons_id, explosive_id, quantity)
                    VALUES (?, ?, ?)
                ''', (
                    player_loadouts_weapons_id,
                    int(explosive['id']),
                    int(explosive['quantity'])
                ))

        conn.commit()
        flash("Loadout salvo com sucesso!", "success")

    except Exception as e:
        conn.rollback()
        traceback.print_exc()
        print(f"Erro ao salvar loadout: {e}", "danger")
        flash(f"Erro ao salvar loadout: {e}", "danger")

    finally:
        conn.close()

    return redirect(url_for('player_loadout_weapons', player_id=player_id))

# Items
# @app.route('/items', methods=['GET'])
# @login_required
# def get_items():
#     conn = get_db_connection()
#     items = conn.execute('''
#         SELECT item.*, item_types.name AS type_name
#         FROM item
#         JOIN item_types ON item.type_id = item_types.id
#     ''').fetchall()
#     conn.close()
#     return jsonify([dict(item) for item in items])

# Com subitem
@app.route('/items', methods=['GET'])
@login_required
def get_items():
    conn = get_db_connection()

    # Buscar todos os itens com seus tipos
    items = conn.execute('''
        SELECT item.*, item_types.name AS type_name
        FROM item
        JOIN item_types ON item.type_id = item_types.id
    ''').fetchall()

    # Buscar as relações de compatibilidade (parent -> child)
    compat_rows = conn.execute('SELECT parent_item_id, child_item_id FROM item_compatibility').fetchall()
    conn.close()

    # Mapear itens por ID
    item_dict = {item['id']: dict(item) for item in items}

    # Inicializar subitems vazios
    for item in item_dict.values():
        item['subitems'] = []

    # Criar mapa de compatibilidade: parent_id -> [child_id1, child_id2, ...]
    compat_map = {}
    for row in compat_rows:
        compat_map.setdefault(row['parent_item_id'], []).append(row['child_item_id'])

    # Função recursiva para montar árvore de subitens
    def build_subtree(item_id):
        item = item_dict[item_id]
        children = compat_map.get(item_id, [])
        item['subitems'] = [build_subtree(child_id) for child_id in children] if children else None
        return item

    # Identificar os itens que são apenas "pais" (não são filhos de ninguém)
    child_ids = {row['child_item_id'] for row in compat_rows}
    top_level_ids = [item_id for item_id in item_dict if item_id not in child_ids]

    # Montar lista final com subitens recursivos
    result = [build_subtree(item_id) for item_id in top_level_ids]

    return jsonify(result)



@app.route('/items_pagination', methods=['GET'])
@login_required
def get_items_pagination():
    type_id = request.args.get('type_id')
    name = request.args.get('name')
    min_slots = request.args.get('min_slots', type=int)
    max_slots = request.args.get('max_slots', type=int)
    page = int(request.args.get('page', 1))
    per_page = int(request.args.get('per_page', 12))
    offset = (page - 1) * per_page

    filters = []
    params = []

    if type_id:
        filters.append("item.type_id = ?")
        params.append(type_id)
    if name:
        filters.append("item.name LIKE ?")
        params.append(f"%{name}%")
    if min_slots is not None:
        filters.append("item.slots >= ?")
        params.append(min_slots)
    if max_slots is not None:
        filters.append("item.slots <= ?")
        params.append(max_slots)

    where_clause = "WHERE " + " AND ".join(filters) if filters else ""

    query = f'''
        SELECT item.*, item_types.name AS name_type
        FROM item
        JOIN item_types ON item.type_id = item_types.id
        {where_clause}
        LIMIT ? OFFSET ?
    '''
    total_query = f'''
        SELECT COUNT(*) FROM item
        {where_clause}
    '''

    conn = get_db_connection()
    total = conn.execute(total_query, params).fetchone()[0]
    items = conn.execute(query, params + [per_page, offset]).fetchall()
    conn.close()

    return jsonify({
        "items": [dict(item) for item in items],
        "total": total,
        "page": page,
        "per_page": per_page
    })

@app.route('/items/<int:item_id>', methods=['GET'])
@login_required
def get_item(item_id):
    conn = get_db_connection()
    item = conn.execute('''
        SELECT item.*, item_types.name AS type_name
        FROM item
        JOIN item_types ON item.type_id = item_types.id
        WHERE item.id = ?
    ''', (item_id,)).fetchone()
    conn.close()
    return jsonify(dict(item))

@app.route('/items/<int:item_id>/compatible-parents', methods=['GET'])
@login_required
def get_compatible_parents(item_id):
    conn = get_db_connection()
    results = conn.execute('''
        SELECT i.id, i.name, i.img
        FROM item_compatibility ic
        JOIN item i ON ic.parent_item_id = i.id
        WHERE ic.child_item_id = ?
    ''', (item_id,)).fetchall()
    conn.close()
    return jsonify([dict(row) for row in results])

@app.route('/items', methods=['POST'])
@login_required
@admin_required
def add_item():
    data = request.json
    # Valida o tipo de munição
    name_type=data['name_type']
    if name_type not in type_names:
        e=f"'{name_type}' não é um item válido do types.xml."
        return jsonify({"error": str(e)}), 400
    
    conn = get_db_connection()
    try:
        conn.execute('''
            INSERT INTO item (name, name_type, type_id, slots, width, height, img, storage_slots, storage_width, storage_height, localization)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            data['name'], data['name_type'], data['type_id'],
            data['slots'], data['width'], data['height'], data['img'], data['storage_slots'], data['storage_width'], data['storage_height'], data['localization'] 
        ))
        conn.commit()
        return jsonify({"message": "Item adicionado com sucesso"}), 201
    except Exception as e:
        conn.rollback()
        return jsonify({"error": str(e)}), 400
    finally:
        conn.close()

@app.route('/items/<int:item_id>', methods=['PUT'])
@login_required
@admin_required
def update_item(item_id):
    data = request.json
    # Valida o tipo de munição
    name_type=data['name_type']
    if name_type not in type_names:
        e=f"'{name_type}' não é um item válido do types.xml."
        return jsonify({"error": str(e)}), 400
    
    conn = get_db_connection()
    try:
        conn.execute('''
            UPDATE item SET name = ?, name_type = ?, type_id = ?, 
                slots = ?, width = ?, height = ?, img = ?, 
                storage_slots = ?, storage_width = ?, storage_height = ?, localization = ?
            WHERE id = ?
        ''', (
            data['name'], data['name_type'], data['type_id'],
            data['slots'], data['width'], data['height'], data['img'], 
            data['storage_slots'], data['storage_width'], data['storage_height'], data['localization'],
            item_id
        ))
        conn.commit()
        return jsonify({"message": "Item atualizado com sucesso"})
    except Exception as e:
        conn.rollback()
        return jsonify({"error": str(e)}), 400
    finally:
        conn.close()

@app.route('/items/<int:item_id>', methods=['DELETE'])
@login_required
@admin_required
def delete_item(item_id):
    conn = get_db_connection()
    try:
        conn.execute('DELETE FROM item WHERE id = ?', (item_id,))
        conn.commit()
        return jsonify({"message": "Item deletado com sucesso"})
    except Exception as e:
        conn.rollback()
        return jsonify({"error": str(e)}), 400
    finally:
        conn.close()

@app.route('/item_types', methods=['GET'])
@login_required
def get_item_types():
    conn = get_db_connection()
    types = conn.execute('SELECT * FROM item_types').fetchall()
    conn.close()
    return jsonify([dict(row) for row in types])

@app.route('/item_types', methods=['POST'])
@login_required
@admin_required
def create_item_type():
    data = request.json
    conn = get_db_connection()
    try:
        conn.execute('INSERT INTO item_types (name) VALUES (?)', (data['name'],))
        conn.commit()
        return jsonify({'message': 'Tipo criado com sucesso'}), 201
    except Exception as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 400
    finally:
        conn.close()

@app.route('/item_types/<int:type_id>', methods=['PUT'])
@login_required
@admin_required
def update_item_type(type_id):
    data = request.get_json()
    new_name = data.get('name')
    if not new_name:
        return jsonify({'error': 'Nome é obrigatório'}), 400

    conn = get_db_connection()
    conn.execute('UPDATE item_types SET name = ? WHERE id = ?', (new_name, type_id))
    conn.commit()
    conn.close()
    return jsonify({'success': True})

@app.route('/item_types/<int:type_id>', methods=['DELETE'])
@login_required
@admin_required
def delete_item_type(type_id):
    conn = get_db_connection()
    try:
        # Desassociar todos os itens que usam este tipo (setar type_id como NULL ou 0)
        conn.execute('UPDATE item SET type_id = NULL WHERE type_id = ?', (type_id,))
        
        # Excluir o tipo
        conn.execute('DELETE FROM item_types WHERE id = ?', (type_id,))
        
        conn.commit()
        return jsonify({'message': 'Tipo de item excluído com sucesso'})
    except Exception as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 400
    finally:
        conn.close()

@app.route('/items/<int:item_id>/compatibilities', methods=['GET'])
@login_required
def get_compatibilities(item_id):
    conn = get_db_connection()
    results = conn.execute('''
        SELECT i.id, i.name, i.img
        FROM item_compatibility ic
        JOIN item i ON ic.child_item_id = i.id
        WHERE ic.parent_item_id = ?
    ''', (item_id,)).fetchall()
    conn.close()
    return jsonify([dict(row) for row in results])

@app.route('/items/<int:item_id>/compatibilities', methods=['POST'])
@login_required
@admin_required
def add_compatibility(item_id):
    data = request.json  # Ex: {"child_item_id": 2}
    child_item_id = data['child_item_id']

    conn = get_db_connection()
    try:
        conn.execute('''
            INSERT INTO item_compatibility (parent_item_id, child_item_id)
            VALUES (?, ?)
        ''', (item_id, child_item_id))
        conn.commit()
        return jsonify({'message': 'Compatibilidade adicionada com sucesso'}), 201
    except Exception as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 400
    finally:
        conn.close()

@app.route('/items/<int:item_id>/compatibilities/<int:child_item_id>', methods=['DELETE'])
@login_required
@admin_required
def delete_compatibility(item_id, child_item_id):
    conn = get_db_connection()
    try:
        conn.execute('''
            DELETE FROM item_compatibility 
            WHERE parent_item_id = ? AND child_item_id = ?
        ''', (item_id, child_item_id))
        conn.commit()
        return jsonify({'message': 'Compatibilidade removida'})
    except Exception as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 400
    finally:
        conn.close()

# Loadout de items para players
@app.route('/player_loadout_items/<player_id>', methods=['GET', 'POST'])
@login_required
def player_loadout_items(player_id):
    conn = get_db_connection()
    
    items = conn.execute('''
        SELECT pi.id AS player_item_id, i.*
        FROM player_items pi
        JOIN item i ON pi.item_id = i.id
        WHERE pi.player_id = ?
    ''', (player_id,)).fetchall()
    conn.close()
    conn2 = get_db_connection_players_beco_c1()
    player = conn2.execute('SELECT * FROM players_database WHERE PlayerID = ?', (player_id,)).fetchone()
    conn.close()
    return render_template(
        'player_loadout_items.html',
        items=items, player=player
    )

@app.route('/players/<player_id>/items', methods=['DELETE'])
@login_required
def clear_player_items(player_id):
    conn = get_db_connection()
    conn.execute('DELETE FROM player_items WHERE player_id = ?', (player_id,))
    conn.commit()
    conn.close()
    return jsonify({'message': f'Todos os itens do jogador {player_id} foram removidos.'})

# Novos metodos do loadout de items
# Função auxiliar para obter os itens do jogador
def get_player_items(player_id):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT i.*, pi.quantity, it.name as type_name
        FROM player_items pi
        JOIN item i ON pi.item_id = i.id
        JOIN item_types it ON i.type_id = it.id
        WHERE pi.player_id = ?
    """, (player_id,))
    items = cursor.fetchall()
    return items

# Endpoint para listar os itens de um jogador
@app.route('/players/<player_id>/items', methods=['GET'])
@login_required
def list_player_items(player_id):
    try:
        items = get_player_items(player_id)
        return jsonify([dict(item) for item in items])
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# Endpoint para adicionar um item ao jogador
@app.route('/players/<player_id>/items', methods=['POST'])
@login_required
def add_item_to_player(player_id):
    item_id = request.json.get('item_id')
    if not item_id:
        return jsonify({'error': 'Item ID é obrigatório'}), 400

    # Verifica se o item já está presente no inventário do jogador
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT quantity FROM player_items WHERE player_id = ? AND item_id = ?
    """, (player_id, item_id))
    result = cursor.fetchone()

    # Se o item já existe, incrementa a quantidade
    if result:
        cursor.execute("""
            UPDATE player_items
            SET quantity = quantity + 1
            WHERE player_id = ? AND item_id = ?
        """, (player_id, item_id))
    else:
        # Caso contrário, insere o item com quantidade 1
        cursor.execute("""
            INSERT INTO player_items (player_id, item_id, quantity)
            VALUES (?, ?, 1)
        """, (player_id, item_id))

    conn.commit()
    return jsonify({'message': 'Item adicionado com sucesso'}), 200

# Endpoint para remover um item do jogador
@app.route('/players/<player_id>/items/<item_id>', methods=['DELETE'])
@login_required
def remove_item_from_player(player_id, item_id):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        DELETE FROM player_items WHERE player_id = ? AND item_id = ?
    """, (player_id, item_id))
    conn.commit()
    return jsonify({'message': 'Item removido com sucesso'}), 200

# Endpoint para verificar compatibilidade entre os itens
@app.route('/items/compatibility', methods=['GET'])
@login_required
def check_item_compatibility():
    parent_item_id = request.args.get('parent_item_id')
    child_item_id = request.args.get('child_item_id')

    if not parent_item_id or not child_item_id:
        return jsonify({'error': 'Os IDs dos itens são necessários'}), 400

    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        SELECT * FROM item_compatibility
        WHERE parent_item_id = ? AND child_item_id = ?
    """, (parent_item_id, child_item_id))
    compatibility = cursor.fetchone()

    if compatibility:
        return jsonify({'compatible': True}), 200
    else:
        return jsonify({'compatible': False}), 200

# Endpoint para listar todos os itens
@app.route('/items', methods=['GET'])
@login_required
def list_items():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, name_type, img FROM item")
    items = cursor.fetchall()
    return jsonify([{
        'id': item['id'],
        'name': item['name'],
        'name_type': item['name_type'],
        'img': item['img']
    } for item in items])

@app.route('/players/<player_id>/items/<int:item_id>/quantity', methods=['PATCH'])
@login_required
def update_item_quantity(player_id, item_id):
    delta = request.json.get('delta')
    if not isinstance(delta, int):
        return jsonify({'error': 'Delta inválido'}), 400

    conn = get_db_connection()
    cursor = conn.cursor()

    cursor.execute('SELECT quantity FROM player_items WHERE player_id = ? AND item_id = ?', (player_id, item_id))
    row = cursor.fetchone()
    if not row:
        return jsonify({'error': 'Item não encontrado'}), 404

    new_quantity = row['quantity'] + delta
    if new_quantity < 1:
        cursor.execute('DELETE FROM player_items WHERE player_id = ? AND item_id = ?', (player_id, item_id))
    else:
        cursor.execute('UPDATE player_items SET quantity = ? WHERE player_id = ? AND item_id = ?', (new_quantity, player_id, item_id))

    conn.commit()
    conn.close()
    return jsonify({'success': True})

@app.route('/delete_loadout_items/<player_id>', methods=['GET'])
@login_required
def delete_loadout_items(player_id):
    conn = get_db_connection()
    
    try:
        # Depois deleta o loadout principal
        conn.execute('DELETE FROM player_items WHERE player_id = ?', (player_id,))
        
        conn.commit()
        flash('Loadout excluído com sucesso!', 'success')
    except Exception as e:
        conn.rollback()
        flash(f'Erro ao excluir loadout: {str(e)}', 'danger')
    finally:
        conn.close()
    
    return redirect(url_for('player_loadout_items', player_id=player_id))

# ✅ ESSENCIAL PARA INICIAR O SERVIDOR
if __name__ == '__main__':
    start_scheduler()
    app.run(host='0.0.0.0', port=54321, debug=True)