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
    conn = sqlite3.connect('../databases/dayz_items.db')
    conn.row_factory = sqlite3.Row
    return conn


@app.route('/')
def index():
    if 'user' not in session:
        return redirect(url_for('login'))
    
    try:
        # Conectar ao banco de dados
        conn = get_db_connection()

        # Realizar as consultas
        weapons = conn.execute('SELECT * FROM weapons').fetchall()
        ammunitions = conn.execute("""
            SELECT
                ammunitions.id,
                ammunitions.name,
                ammunitions.name_type,
                calibers.name AS caliber_name,
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

    except DatabaseError as e:
        flash(f"Ocorreu um erro ao acessar o banco de dados: {str(e)}", 'danger')
        return redirect(url_for('login'))

    finally:
        # Fechar a conexão com o banco de dados de forma segura
        conn.close()

    # Renderizar a página com os dados obtidos
    return render_template('index.html', 
                           weapons=weapons, 
                           ammunitions=ammunitions, 
                           magazines=magazines, 
                           calibers=calibers, 
                           attachments=attachments)
# Arma
@app.route('/add_weapon', methods=['GET', 'POST'])
def add_weapon():
    if 'user' not in session:
        return redirect(url_for('login'))

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
def edit_weapon(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_weapon(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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


# Calibre
@app.route('/add_caliber', methods=['GET', 'POST'])
def add_caliber():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def edit_caliber(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_caliber(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
@app.route('/add_ammo', methods=['GET', 'POST'])
def add_ammo():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def edit_ammo(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_ammo(id):
    if 'user' not in session:
        return redirect(url_for('login'))

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


@app.route('/add_magazine', methods=['GET', 'POST'])
def add_magazine():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def edit_magazine(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_magazine(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
@app.route('/add_attachment', methods=['GET', 'POST'])
def add_attachment():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def edit_attachment(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_attachment(id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def add_weapon_ammunitions():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_weapon_ammunitions(ammo_id, weapon_id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def add_weapon_magazines():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_weapon_magazines(magazine_id, weapon_id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def add_weapon_attachments():
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
def delete_weapon_attachments(attachment_id, weapon_id):
    if 'user' not in session:
        return redirect(url_for('login'))
    
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
    app.run(host='0.0.0.0', port=54321)