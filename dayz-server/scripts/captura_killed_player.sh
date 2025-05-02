#!/bin/bash

# Verifica se o parâmetro foi passado
if [[ -z "$1" ]]; then
  echo "Erro: Você deve fornecer o Message como parâmetro."
  echo "Uso: $0 <Message>"
  echo "Exemplo: Player ""dexx"" (DEAD) (id=UwSzI-jZF1KfvnpZLC0LrnjeWwEsPTdAe29fimFnSfM= pos=<2635.3, 1313.4, 22.9>) killed by Player ""Guzulino"" (id=xblv1tW0AmFhZcO4gmLYztqErmih0lCfKd2VqVxwz3E= pos=<2634.2, 1311.6, 23.0>) with KA-74 from 2.0941
2 meters"
  exit 1
fi

Message="$1"

# Extrai nome do atingido
ATINGIDO=$(echo "$Message" | sed -n 's/Player "\([^"]*\)".*/\1/p')
# Extrai nome do atacante
ATACANTE=$(echo "$Message" | sed -n 's/.*killed by Player "\([^"]*\)".*/\1/p')

# Ignora se algum dos nomes não foi capturado
[ -z "$ATINGIDO" ] && echo "Nome do player atingido não foi identificado" && exit 1
[ -z "$ATACANTE" ] && echo "Nome do player que atacou não foi identificado" && exit 1

# Captura os IDs dos jogadores
IDS=$(echo "$Message" | grep -oP 'id=[^ ]+')
ID_ATINGIDO=$(echo "$IDS" | sed -n '1p' | cut -d= -f2)
ID_ATACANTE=$(echo "$IDS" | sed -n '2p' | cut -d= -f2)

# Extrai outras informações
HP=$(echo "$Message" | grep -oP '\[HP: \K[0-9.]+' || echo "Desconhecido")
LOCAL=$(echo "$Message" | grep -oP 'into \K[^ ]+' | sed 's/([0-9]*)//g')
DANO=$(echo "$Message" | grep -oP 'for \K[0-9.]+(?= damage)')
TIPO=$(echo "$Message" | grep -oP 'damage \(\K[^)]+' || echo "Desconhecido")
DISTANCIA=$(echo "$Message" | grep -oP 'from \K[0-9.]+(?= meters)' || echo "0")
ARMA=$(echo "$Message" | grep -oP 'with \K[^ ]+' || echo "Desconhecida")

# Posições
POS_ATINGIDO=$(echo "$Message" | grep -oP 'pos=<[^>]+>' | sed -n '1p' | sed 's/pos=<//' | sed 's/>//')
POS_ATACANTE=$(echo "$Message" | grep -oP 'pos=<[^>]+>' | sed -n '2p' | sed 's/pos=<//' | sed 's/>//')

# Exibição
# echo "O jogador \"$ATINGIDO\" (ID: $ID_ATINGIDO) foi atingido por \"$ATACANTE\" (ID: $ID_ATACANTE)."
# echo "     Local atingido: $LOCAL"
# echo "     Tipo de ataque: $TIPO"
# echo "     Dano causado: $DANO"
# echo "     HP restante: $HP"
# echo "     Posição do atingido: $POS_ATINGIDO"
# echo "     Posição do atacante: $POS_ATACANTE"

#[ -n "$ARMA" ] && echo "     Arma usada: $ARMA"
#[ -n "$DISTANCIA" ] && echo "     Distância do disparo: $DISTANCIA metros"
echo "$ID_ATACANTE|$ID_ATINGIDO|$POS_ATACANTE|$POS_ATINGIDO|$LOCAL|$TIPO|$DANO|$HP|$ARMA|$DISTANCIA" 