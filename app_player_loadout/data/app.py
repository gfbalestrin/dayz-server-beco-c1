from flask import Flask, render_template, request, redirect
import sqlite3

app = Flask(__name__)

def get_db_connection():
    conn = sqlite3.connect('dayz_items.db')
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/')
def index():
    conn = get_db_connection()
    weapons = conn.execute('SELECT * FROM weapons').fetchall()
    conn.close()
    return render_template('index.html', weapons=weapons)

@app.route('/add_weapon', methods=['GET', 'POST'])
def add_weapon():
    if request.method == 'POST':
        name = request.form['name']
        feed_type = request.form['feed_type']
        conn = get_db_connection()
        conn.execute('INSERT INTO weapons (name, feed_type) VALUES (?, ?)', (name, feed_type))
        conn.commit()
        conn.close()
        return redirect('/')
    return render_template('add_weapon.html')

@app.route('/edit_weapon/<int:id>', methods=['GET', 'POST'])
def edit_weapon(id):
    conn = get_db_connection()
    weapon = conn.execute('SELECT * FROM weapons WHERE id = ?', (id,)).fetchone()
    
    if request.method == 'POST':
        name = request.form['name']
        feed_type = request.form['feed_type']
        conn.execute('UPDATE weapons SET name = ?, feed_type = ? WHERE id = ?', (name, feed_type, id))
        conn.commit()
        conn.close()
        return redirect('/')
    
    conn.close()
    return render_template('edit_weapon.html', weapon=weapon)

@app.route('/delete_weapon/<int:id>')
def delete_weapon(id):
    conn = get_db_connection()
    conn.execute('DELETE FROM weapons WHERE id = ?', (id,))
    conn.commit()
    conn.close()
    return redirect('/')

# ✅ ESSENCIAL PARA INICIAR O SERVIDOR
if __name__ == '__main__':
    app.run(debug=True)