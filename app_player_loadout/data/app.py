import requests
import xml.etree.ElementTree as ET
from flask import Flask, render_template, request, redirect, url_for, session, flash, jsonify
from werkzeug.security import check_password_hash, generate_password_hash
import sqlite3

USERS = {
    "admin": generate_password_hash("tH987c2d#$67")
}

app = Flask(__name__)
app.secret_key = 'd,bw:#.cvH](,9Yi;@+#3u.-50%:7(£91ND4#5Ps'  # Altere para uma chave forte na produção

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
    conn = sqlite3.connect('dayz_items.db')
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/')
def index():
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    weapons = conn.execute('SELECT * FROM weapons').fetchall()
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
    magazines = conn.execute('SELECT * FROM magazines').fetchall()
    calibers = conn.execute('SELECT * FROM calibers').fetchall()
    attachments = conn.execute('SELECT * FROM attachments').fetchall()
    conn.close()
    return render_template('index.html', weapons=weapons, ammunitions=ammunitions, magazines=magazines, calibers=calibers, attachments=attachments)

# Arma
@app.route('/add_weapon', methods=['GET', 'POST'])
def add_weapon():
    if 'user' not in session:
        return redirect(url_for('login'))    
    if request.method == 'POST':
        name = request.form['name']
        name_type = request.form['name_type']
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400        
        feed_type = request.form['feed_type']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn = get_db_connection()
        conn.execute('INSERT INTO weapons (name, name_type, feed_type, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', (name, name_type, feed_type, slots, width, height, img))
        conn.commit()
        conn.close()
        return redirect('/')
    return render_template('add_weapon.html')

@app.route('/edit_weapon/<int:id>', methods=['GET', 'POST'])
def edit_weapon(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (id,)).fetchone()
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
    weapon_ammunitions = conn.execute("""SELECT 
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
JOIN calibers ON ammunitions.caliber_id = calibers.id WHERE weapon_id = ?;""", (id,)).fetchall()
    weapon_magazines = conn.execute("""SELECT 
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
    magazines ON weapon_magazines.magazine_id = magazines.id WHERE weapon_id = ?;""", (id,)).fetchall()
    weapon_attachments = conn.execute("""SELECT 
    weapon_attachments.weapon_id,
    attachments.id AS attachment_id,
    attachments.name,
    attachments.name_type,
    attachments.type,
    attachments.slots,
    attachments.width,
    attachments.height,
    attachments.img
FROM 
    weapon_attachments
JOIN 
    attachments ON weapon_attachments.attachment_id = attachments.id WHERE weapon_id = ?;""", (id,)).fetchall()
    if request.method == 'POST':
        name = request.form['name']
        name_type = request.form['name_type']
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400
        
        feed_type = request.form['feed_type']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn.execute('UPDATE weapons SET name = ?, name_type = ?, feed_type = ?, slots = ?, width = ?, height = ?, img = ?  WHERE id = ?', (name, name_type, feed_type, slots, width, height, img, id))
        conn.commit()
        conn.close()
        return redirect('/')
    
    conn.close()
    return render_template('edit_weapon.html', weapon=weapon, magazines=magazines, ammunitions=ammunitions, attachments=attachments, weapon_ammunitions=weapon_ammunitions, weapon_magazines=weapon_magazines, weapon_attachments=weapon_attachments)

@app.route('/delete_weapon/<int:id>')
def delete_weapon(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM weapons WHERE id = ?', (id,))
    conn.commit()
    conn.close()
    return redirect('/')

# Calibre
@app.route('/add_caliber', methods=['GET', 'POST'])
def add_caliber():
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    if request.method == 'POST':
        name = request.form['name']
        conn.execute('INSERT INTO calibers (name) VALUES (?)', (name,))
        conn.commit()
        conn.close()
        return redirect('/')
    return render_template('add_caliber.html')

@app.route('/edit_caliber/<int:id>', methods=['GET', 'POST'])
def edit_caliber(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    caliber = conn.execute('SELECT * FROM calibers WHERE id = ?', (id,)).fetchone()
    
    if request.method == 'POST':
        name = request.form['name'] 
        conn.execute('UPDATE calibers SET name = ? WHERE id = ?', (name, id))
        conn.commit()
        conn.close()
        return redirect('/')
    
    conn.close()
    return render_template('edit_caliber.html', caliber=caliber)

@app.route('/delete_caliber/<int:id>')
def delete_caliber(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM ammunitions WHERE id = ?', (id,))
    conn.commit()
    conn.close()
    return redirect('/')


# Munição
@app.route('/add_ammo', methods=['GET', 'POST'])
def add_ammo():
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    calibers = conn.execute('SELECT * FROM calibers').fetchall()
    if request.method == 'POST':
        name = request.form['name'] 
        name_type = request.form['name_type']
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400
        
        caliber_id = request.form['caliber_id']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        
        conn.execute('INSERT INTO ammunitions (name, name_type, caliber_id, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', (name, name_type, caliber_id, slots, width, height, img))
        conn.commit()
        conn.close()
        return redirect('/')
    return render_template('add_ammo.html', calibers=calibers)

@app.route('/edit_ammo/<int:id>', methods=['GET', 'POST'])
def edit_ammo(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    ammo = conn.execute('SELECT * FROM ammunitions WHERE id = ?', (id,)).fetchone()
    calibers = conn.execute('SELECT * FROM calibers').fetchall()
    
    if request.method == 'POST':
        name = request.form['name'] 
        name_type = request.form['name_type']
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400
             
        caliber_id = request.form['caliber_id']
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn.execute('UPDATE ammunitions SET name = ?, name_type = ?, caliber_id = ?, width = ?, height = ?, img = ? WHERE id = ?', (name, name_type, caliber_id, width, height, img, id))
        conn.commit()
        conn.close()
        return redirect('/')
    
    conn.close()
    return render_template('edit_ammo.html', ammo=ammo, calibers=calibers)

@app.route('/delete_ammo/<int:id>')
def delete_ammo(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM ammunitions WHERE id = ?', (id,))
    conn.commit()
    conn.close()
    return redirect('/')

# Carregador
@app.route('/add_magazine', methods=['GET', 'POST'])
def add_magazine():
    if 'user' not in session:
        return redirect(url_for('login'))
    
    conn = get_db_connection()
    ammunitions = conn.execute('SELECT * FROM ammunitions').fetchall()
    if request.method == 'POST':
        name = request.form['name']      
        name_type = request.form['name_type']  
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400   
        capacity = request.form['capacity']   
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn.execute('INSERT INTO magazines (name, name_type, capacity, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', (name, name_type, capacity, slots, width, height, img))
        conn.commit()
        conn.close()
        return redirect('/')
    return render_template('add_magazine.html', ammunitions=ammunitions)

@app.route('/edit_magazine/<int:id>', methods=['GET', 'POST'])
def edit_magazine(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    magazine = conn.execute('SELECT * FROM magazines WHERE id = ?', (id,)).fetchone()
    ammunitions = conn.execute('SELECT * FROM ammunitions').fetchall()
    
    if request.method == 'POST':
        name = request.form['name']  
        name_type = request.form['name_type']          
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400   
        capacity = request.form['capacity']  
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn.execute('UPDATE magazines SET name = ?, name_type = ?, capacity = ?, slots = ?, width = ?, height = ?, img = ? WHERE id = ?', (name, name_type, capacity, slots, width, height, img, id))
        conn.commit()
        conn.close()
        return redirect('/')
    
    conn.close()
    return render_template('edit_magazine.html', magazine=magazine, ammunitions=ammunitions)

@app.route('/delete_magazine/<int:id>')
def delete_magazine(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM magazines WHERE id = ?', (id,))
    conn.commit()
    conn.close()
    return redirect('/')

# Acessórios
@app.route('/add_attachment', methods=['GET', 'POST'])
def add_attachment():
    if 'user' not in session:
        return redirect(url_for('login'))
    
    conn = get_db_connection()
    attachments = conn.execute('SELECT * FROM attachments').fetchall()
    if request.method == 'POST':
        name = request.form['name']      
        name_type = request.form['name_type']  
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400   
        type = request.form['type']   
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn.execute('INSERT INTO attachments (name, name_type, type, slots, width, height, img) VALUES (?, ?, ?, ?, ?, ?, ?)', (name, name_type, type, slots, width, height, img))
        conn.commit()
        conn.close()
        return redirect('/')
    return render_template('add_attachment.html', attachments=attachments)

@app.route('/edit_attachment/<int:id>', methods=['GET', 'POST'])
def edit_attachment(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    attachment = conn.execute('SELECT * FROM attachments WHERE id = ?', (id,)).fetchone()
    
    if request.method == 'POST':
        name = request.form['name']  
        name_type = request.form['name_type']          
        if name_type not in type_names:
            return jsonify({"error": f"'{name_type}' não é um item válido do types.xml."}), 400   
        type = request.form['type']  
        slots = request.form['slots']
        width = request.form['width']
        height = request.form['height']
        img = request.form['img']
        conn.execute('UPDATE attachments SET name = ?, name_type = ?, type = ?, slots = ?, width = ?, height = ?, img = ? WHERE id = ?', (name, name_type, type, slots, width, height, img, id))
        conn.commit()
        conn.close()
        return redirect('/')
    
    conn.close()
    return render_template('edit_attachment.html', attachment=attachment)

@app.route('/delete_attachment/<int:id>')
def delete_attachment(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM attachments WHERE id = ?', (id,))
    conn.commit()
    conn.close()
    return redirect('/')

# weapon_ammunitions
@app.route('/add_weapon_ammunitions', methods=['GET', 'POST'])
def add_weapon_ammunitions():
    if 'user' not in session:
        return redirect(url_for('login'))
    
    conn = get_db_connection()
    weapon_ammunitions = conn.execute('SELECT * FROM weapon_ammunitions').fetchall()
    if request.method == 'POST':      
        weapon_id = request.form['weapon_id']      
        ammo_id = request.form['ammo_id']  
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
        
        conn.execute('INSERT INTO weapon_ammunitions (weapon_id, ammo_id) VALUES (?, ?)', (weapon_id, ammo_id))
        conn.commit()
        conn.close()
        return redirect(request.referrer)
    return redirect(request.referrer)

@app.route('/delete_weapon_ammunitions/<int:ammo_id>/<int:weapon_id>')
def delete_weapon_ammunitions(ammo_id, weapon_id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM weapon_ammunitions WHERE ammo_id = ? AND weapon_id = ?', (ammo_id, weapon_id))
    conn.commit()
    conn.close()
    return redirect(request.referrer)

# weapon_magazines
@app.route('/add_weapon_magazines', methods=['GET', 'POST'])
def add_weapon_magazines():
    if 'user' not in session:
        return redirect(url_for('login'))
    
    conn = get_db_connection()
    weapon_magazines = conn.execute('SELECT * FROM weapon_magazines').fetchall()
    if request.method == 'POST':      
        weapon_id = request.form['weapon_id']      
        magazine_id = request.form['magazine_id']  
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
        
        conn.execute('INSERT INTO weapon_magazines (weapon_id, magazine_id) VALUES (?, ?)', (weapon_id, magazine_id))
        conn.commit()
        conn.close()
        return redirect(request.referrer)
    return redirect(request.referrer)

@app.route('/delete_weapon_magazines/<int:magazine_id>/<int:weapon_id>')
def delete_weapon_magazines(magazine_id, weapon_id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM weapon_magazines WHERE magazine_id = ? AND weapon_id = ?', (magazine_id, weapon_id))
    conn.commit()
    conn.close()
    return redirect(request.referrer)

# weapon_attachments
@app.route('/add_weapon_attachments', methods=['GET', 'POST'])
def add_weapon_attachments():
    if 'user' not in session:
        return redirect(url_for('login'))
    
    conn = get_db_connection()
    if request.method == 'POST':      
        weapon_id = request.form['weapon_id']      
        attachment_id = request.form['attachment_id']  
        weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (weapon_id,)).fetchone()
        
        conn.execute('INSERT INTO weapon_attachments (weapon_id, attachment_id) VALUES (?, ?)', (weapon_id, attachment_id))
        conn.commit()
        conn.close()
        return redirect(request.referrer)
    return redirect(request.referrer)

@app.route('/delete_weapon_attachments/<int:attachment_id>/<int:weapon_id>')
def delete_weapon_attachments(attachment_id, weapon_id):
    if 'user' not in session:
        return redirect(url_for('login'))
    conn = get_db_connection()
    conn.execute('DELETE FROM weapon_attachments WHERE attachment_id = ? AND weapon_id = ?', (attachment_id, weapon_id))
    conn.commit()
    conn.close()
    return redirect(request.referrer)


@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']

        if username in USERS and check_password_hash(USERS[username], password):
            session['user'] = username
            return redirect(url_for('index'))
        else:
            flash("Usuário ou senha inválidos", "danger")

    return render_template('login.html')

@app.route('/logout')
def logout():
    session.pop('user', None)
    return redirect(url_for('login'))

# ✅ ESSENCIAL PARA INICIAR O SERVIDOR
if __name__ == '__main__':
    app.run(debug=True)