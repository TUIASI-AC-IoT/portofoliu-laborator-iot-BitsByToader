#!/bin/zsh

BASE_URL="http://localhost:5001"

# Test Login valid pentru user1 folosind POST /auth
echo -e "\n[TEST 1] Autentificare cu succes (user1):"
RESPONSE=$(curl -s -X POST "$BASE_URL/auth" \
  -H "Content-Type: application/json" \
  -d '{"username": "user1", "passwd": "parola1"}')
echo "Raspuns: $RESPONSE"

TOKEN=$(echo "$RESPONSE" | sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')

if [ -z "$TOKEN" ]; then
    echo "Eroare: Nu s-a putut obtine token-ul din raspuns!"
    exit 1
fi

# Test Login invalid (Trebuie sa returneze eroare de autentificare)
echo -e "\n[TEST 2] Autentificare cu date eronate:"
curl -s -o /dev/null -w "Status Code: %{http_code} (Asteptat: 401)\n" -X POST "$BASE_URL/auth" \
  -H "Content-Type: application/json" \
  -d '{"username": "user1", "passwd": "parola_gresita"}'

# Test Verificare Token Valid folosind GET /auth/jwtStore
echo -e "\n[TEST 3] Verificare validitate token activ (Așteptat: 200 + Rol):"
curl -s -w "\nStatus Code: %{http_code}\n" -X GET "$BASE_URL/auth/jwtStore" \
  -H "Authorization: Bearer $TOKEN"

# Test Logout / Invalidare Token folosind DELETE /auth/jwtStore
echo -e "\n[TEST 4] Eliminare token din store via Logout:"
curl -s -w "Status Code: %{http_code} (Asteptat: 200)\n" -X DELETE "$BASE_URL/auth/jwtStore" \
  -H "Authorization: Bearer $TOKEN"

# Test Re-verificare Token după Logout
echo -e "\n[TEST 5] Re-verificare token dupa invalidare:"
curl -s -w "\nStatus Code: %{http_code} (Asteptat: 404)\n" -X GET "$BASE_URL/auth/jwtStore" \
  -H "Authorization: Bearer $TOKEN"

# Test Token fictiv/inexistent
echo -e "\n[TEST 6] Verificare token fictiv:"
curl -s -w "\nStatus Code: %{http_code} (Asteptat: 404)\n" -X GET "$BASE_URL/auth/jwtStore" \
  -H "Authorization: Bearer un_token_inventat"
