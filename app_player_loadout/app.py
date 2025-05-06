import discord
from discord.ext import commands
import sqlite3

intents = discord.Intents.default()
intents.message_content = True  # Necessário para comandos prefixados
intents.members = True
bot = commands.Bot(command_prefix="!", intents=intents)

# === CONFIGURANDO O BANCO DE DADOS ===
conn = sqlite3.connect("loadouts.db")
cursor = conn.cursor()
cursor.execute("""
    CREATE TABLE IF NOT EXISTS loadouts (
        user_id TEXT PRIMARY KEY,
        arma TEXT,
        mochila TEXT
    )
""")
conn.commit()

# === MENUS INTERATIVOS ===

class LoadoutView(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)

        self.armas = ["M4A1", "AK-74", "SVD", "Mosin"]
        self.mochilas = ["Assault Bag", "Hunter Backpack", "Mountain Backpack"]

        self.select_arma = discord.ui.Select(
            placeholder="Escolha sua arma",
            options=[discord.SelectOption(label=arma) for arma in self.armas]
        )
        self.select_mochila = discord.ui.Select(
            placeholder="Escolha sua mochila",
            options=[discord.SelectOption(label=mochila) for mochila in self.mochilas]
        )

        self.select_arma.callback = self.arma_callback
        self.select_mochila.callback = self.mochila_callback

        self.add_item(self.select_arma)
        self.add_item(self.select_mochila)

    async def arma_callback(self, interaction: discord.Interaction):
        user_id = str(interaction.user.id)
        arma = self.select_arma.values[0]

        cursor.execute("INSERT OR IGNORE INTO loadouts (user_id, arma, mochila) VALUES (?, ?, ?)", (user_id, arma, None))
        cursor.execute("UPDATE loadouts SET arma = ? WHERE user_id = ?", (arma, user_id))
        conn.commit()

        await interaction.response.send_message(f"Arma selecionada: *{arma}*", ephemeral=True)

    async def mochila_callback(self, interaction: discord.Interaction):
        user_id = str(interaction.user.id)
        mochila = self.select_mochila.values[0]

        cursor.execute("INSERT OR IGNORE INTO loadouts (user_id, arma, mochila) VALUES (?, ?, ?)", (user_id, None, mochila))
        cursor.execute("UPDATE loadouts SET mochila = ? WHERE user_id = ?", (mochila, user_id))
        conn.commit()

        await interaction.response.send_message(f"Mochila selecionada: *{mochila}*", ephemeral=True)

# === COMANDO PARA EXIBIR O MENU ===
@bot.command()
async def loadout(ctx):
    view = LoadoutView()
    await ctx.author.send("Configure seu loadout selecionando abaixo:", view=view)

# === OPCIONAL: VER LOADOUT SALVO ===
@bot.command()
async def meu_loadout(ctx):
    user_id = str(ctx.author.id)
    cursor.execute("SELECT arma, mochila FROM loadouts WHERE user_id = ?", (user_id,))
    result = cursor.fetchone()

    if result:
        arma, mochila = result
        await ctx.author.send(f"Seu loadout atual:\n*Arma:* {arma or 'Nenhuma'}\n*Mochila:* {mochila or 'Nenhuma'}")
    else:
        await ctx.author.send("Você ainda não configurou seu loadout.")
        

# === INICIAR BOT ===
bot.run("SEU_TOKEN_DO_BOT")