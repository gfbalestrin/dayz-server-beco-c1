import discord
from discord.ext import commands
import sqlite3

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
        Weapon1 TEXT
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
        self.Magazines = {
            "M4A1_Mag": "https://example.com/m4_mag_image.png",  # Exemplo de URL de imagem do carregador
            "AK74_Mag": "https://example.com/ak74_mag_image.png"  # Exemplo de URL de imagem do carregador
        }
        for name, img in self.Weapon1.items():
            self.add_item(WeaponButton(name, img))
        # Adiciona os botões de carregadores
        for name, img in self.Magazines.items():
            self.add_item(WeaponButton(name, img, "magazine"))

class WeaponButton(discord.ui.Button):
    def __init__(self, name, image_url):
        super().__init__(label=name, style=discord.ButtonStyle.primary)
        self.name = name
        self.image_url = image_url

    async def callback(self, interaction: discord.Interaction):
        user_id = str(interaction.user.id)
        cursor.execute("INSERT OR IGNORE INTO players_loadout (DiscordID, Weapon1) VALUES (?, ?)", (user_id, self.name))
        cursor.execute("UPDATE players_loadout SET Weapon1 = ? WHERE DiscordID = ?", (self.name, user_id))
        conn.commit()

        embed = discord.Embed(title="Loadout atualizado!", description=f"Você escolheu a arma: **{self.name}**")
        embed.set_thumbnail(url=self.image_url)

        await interaction.response.send_message(embed=embed, ephemeral=True)

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