from flask import Flask, request, jsonify
from flask_jwt_extended import JWTManager, create_access_token, decode_token

app = Flask(__name__)
app.config["JWT_SECRET_KEY"] = "schimba-ma-in-productie-1234"
jwt = JWTManager(app)

# Matricea de utilizatori stocată în memorie
USERS = {
    "user1": {"passwd": "parola1", "role": "admin"},
    "user2": {"passwd": "parola2", "role": "owner"},
    "user3": {"passwd": "parolaX", "role": "owner"}
}

# Format: { token_string: role }
JWT_STORE = {}

def _get_bearer_token():
    auth_header = request.headers.get("Authorization")
    if not auth_header or not auth_header.startswith("Bearer "):
        return None
    return auth_header.split(" ")[1]

@app.route("/auth", methods=["POST"])
def login():
    data = request.get_json() or {}
    username = data.get("username")
    passwd = data.get("passwd")
    
    if not username or not passwd:
        return jsonify({"error": "Parametrii username si passwd sunt obligatorii"}), 400
        
    user = USERS.get(username)
    if user and user["passwd"] == passwd:
        token = create_access_token(identity=username, additional_claims={"role": user["role"]})
        JWT_STORE[token] = user["role"]
        return jsonify({"token": token}), 200
        
    return jsonify({"error": "Nume de utilizator sau parola incorecte"}), 401

@app.route("/auth/jwtStore", methods=["GET"])
def verify_token():
    token = _get_bearer_token()
    if not token:
        return jsonify({"error": "Header-ul Authorization invalid sau lipsa"}), 400
        
    if token not in JWT_STORE:
        return jsonify({"error": "Token-ul nu este regasit in sistem"}), 404
        
    try:
        decode_token(token)
        return jsonify({"role": JWT_STORE[token]}), 200
    except Exception:
        return jsonify({"error": "Token invalid sau expirat"}), 400

@app.route("/auth/jwtStore", methods=["DELETE"])
def logout():
    token = _get_bearer_token()
    if not token:
        return jsonify({"error": "Header-ul Authorization invalid sau lipsa"}), 400
        
    if token in JWT_STORE:
        del JWT_STORE[token]
        return jsonify({"message": "Token invalidat cu succes"}), 200
        
    return jsonify({"error": "Token-ul nu este regasit in sistem"}), 404

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True)
