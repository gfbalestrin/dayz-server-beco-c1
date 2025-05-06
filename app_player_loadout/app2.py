import discord
from discord.ext import commands
import sqlite3
import asyncio

intents = discord.Intents.default()
intents.message_content = True  # Necessário para comandos prefixados
intents.members = True
bot = commands.Bot(command_prefix="!", intents=intents)

# === Banco de Dados ===
conn = sqlite3.connect("players_loadout.db")
cursor = conn.cursor()
cursor.execute("""
    CREATE TABLE IF NOT EXISTS players_loadout (
        DiscordID TEXT PRIMARY KEY,
        Weapon1 TEXT,
        Weapon1Magazine TEXT,
        MagazineQuantity INTEGER
    )
""")
conn.commit()

# === Classe dos Botões ===
class WeaponButtonView(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)
        # Lista de armas com imagens
        self.Weapon1 = {
            "M4A1": "https://static.wikia.nocookie.net/dayz_gamepedia/images/a/a1/M4A1.png/revision/latest/scale-to-width-down/268?cb=20220330014851",
            "AK-74": "https://static.wikia.nocookie.net/dayz_gamepedia/images/8/8b/AK74.png/revision/latest/scale-to-width-down/268?cb=20210505013141"
        }
        for name, img in self.Weapon1.items():
            self.add_item(WeaponButton(name, img, "weapon"))

class WeaponButton(discord.ui.Button):
    def __init__(self, name, image_url, item_type):
        super().__init__(label=name, style=discord.ButtonStyle.primary)
        self.name = name
        self.image_url = image_url
        self.item_type = item_type

    async def callback(self, interaction: discord.Interaction):
        user_id = str(interaction.user.id)
        
        # Verificar se o jogador já tem um loadout
        cursor.execute("SELECT * FROM players_loadout WHERE DiscordID = ?", (user_id,))
        result = cursor.fetchone()

        # Se o item for uma arma
        if self.item_type == "weapon":
            cursor.execute("INSERT OR IGNORE INTO players_loadout (DiscordID, Weapon1) VALUES (?, ?)", (user_id, self.name))
            cursor.execute("UPDATE players_loadout SET Weapon1 = ? WHERE DiscordID = ?", (self.name, user_id))
            conn.commit()

            embed = discord.Embed(title="Arma selecionada!", description=f"Você escolheu a arma: **{self.name}**")
            embed.set_thumbnail(url=self.image_url)

            # Após escolher a arma, mostrar as opções de carregadores
            await interaction.response.send_message(embed=embed, ephemeral=True)
            await interaction.followup.send(
                "Agora escolha o carregador que deseja usar com a sua arma.",
                view=MagazineButtonView(user_id)
            )

# === Classe para o carregador ===
class MagazineButtonView(discord.ui.View):
    def __init__(self, user_id):
        super().__init__(timeout=None)
        self.user_id = user_id
        # Lista de carregadores com imagens
        self.Magazines = {
            "M4A1_Mag": {
                "image": "https://static.wikia.nocookie.net/dayz_gamepedia/images/3/3d/M4_Magazine.png/revision/latest/scale-to-width-down/268?cb=20220330014926", 
                "name": "M4A1 Magazine"
            },
            "AK74_Mag": {
                "image": "https://static.wikia.nocookie.net/dayz_gamepedia/images/2/27/AK74_Magazine.png/revision/latest/scale-to-width-down/268?cb=20210505013141",
                "name": "AK74 Magazine"
            }
        }
        for key, value in self.Magazines.items():
            self.add_item(MagazineButton(value['name'], value['image'], key))

class MagazineButton(discord.ui.Button):
    def __init__(self, name, image_url, magazine_key):
        super().__init__(label=name, style=discord.ButtonStyle.primary)
        self.name = name
        self.image_url = image_url
        self.magazine_key = magazine_key

    async def callback(self, interaction: discord.Interaction):
        user_id = str(interaction.user.id)

        # Perguntar pela quantidade, mas apenas uma vez, após escolher o carregador
        await interaction.response.send_message(
            content=f"Você selecionou o carregador **{self.name}**",
            ephemeral=True
        )

        # Espera a quantidade
        await self.ask_quantity(interaction, user_id, self.magazine_key)

    async def ask_quantity(self, interaction: discord.Interaction, user_id: str, magazine_name: str):
        # Cria um prompt para o usuário inserir a quantidade do carregador
        def check(msg):
            return msg.author == interaction.user and msg.channel == interaction.channel

        await interaction.followup.send(
            f"Por favor, insira a quantidade de **{magazine_name}** (número inteiro).",
            ephemeral=True
        )
        
        # Espera a mensagem do usuário com a quantidade
        try:
            msg = await bot.wait_for('message', timeout=30.0, check=check)
            quantity = int(msg.content)  # Convertendo o conteúdo para inteiro
            
            # Verifica se a quantidade é válida
            if quantity <= 0:
                await interaction.followup.send("A quantidade deve ser um número positivo.", ephemeral=True)
                return

            # Armazena a escolha no banco de dados
            cursor.execute("INSERT OR IGNORE INTO players_loadout (DiscordID, Weapon1Magazine, MagazineQuantity) VALUES (?, ?, ?)",
                           (user_id, magazine_name, quantity))
            cursor.execute("UPDATE players_loadout SET Weapon1Magazine = ?, MagazineQuantity = ? WHERE DiscordID = ?",
                           (magazine_name, quantity, user_id))
            conn.commit()

            await interaction.followup.send(f"Você escolheu **{quantity}** unidades de **{magazine_name}**.", ephemeral=True)
        except ValueError:
            await interaction.followup.send("Por favor, insira um número válido para a quantidade.", ephemeral=True)
        except asyncio.TimeoutError:
            await interaction.followup.send("Você demorou para responder. O processo foi cancelado.", ephemeral=True)

# === Comando do Bot para iniciar o formulário ===
@bot.command()
async def loadout(ctx):
    embed = discord.Embed(
        title="Escolha sua arma",
        description="Clique no botão correspondente para selecionar sua arma.",
        color=discord.Color.green()
    )
    embed.set_image(url="https://i.ytimg.com/vi/TuMEvRLv6uI/hq720.jpg?sqp=-oaymwEhCK4FEIIDSFryq4qpAxMIARUAAAAAGAElAADIQj0AgKJD&rs=AOn4CLDm31NqNFWLEoE4fHTHgGy7If52uw")  # Uma imagem geral, opcional
    await ctx.send(embed=embed, view=WeaponButtonView())

# === Iniciar Bot ===
bot.run("SEU_TOKEN_DO_BOT")
