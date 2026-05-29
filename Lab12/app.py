import os
import json
import random
from flask import Flask, jsonify, request

app = Flask(__name__)

STORAGE_DIR = 'sensor_configs'

if not os.path.exists(STORAGE_DIR):
    os.makedirs(STORAGE_DIR)

def get_config_path(sensor_id):
    return os.path.join(STORAGE_DIR, f"config_{sensor_id}.json")


# Citirea unei valori de la un senzor
@app.route('/sensors/<sensor_id>', methods=['GET'])
def get_sensor_value(sensor_id):
    try:
        # Simulare senzor
        base_value = round(random.uniform(15.0, 30.0), 2)
        scale = 1.0  # Scala implicita
        
        file_path = get_config_path(sensor_id)
        
        if os.path.exists(file_path):
            with open(file_path, 'r', encoding='utf-8') as f:
                config_data = json.load(f)
                scale = config_data.get('scale', 1.0)
        
        final_value = round(base_value * scale, 2)
        
        return jsonify({
            "sensor_id": sensor_id,
            "raw_value": base_value,
            "applied_scale": scale,
            "measured_value": final_value
        }), 200
        
    except Exception as e:
        return jsonify({"error": f"Eroare la citirea senzorului: {str(e)}"}), 500


# Crearea unui fisier de configurare
@app.route('/sensors/<sensor_id>', methods=['POST'])
def create_sensor_config(sensor_id):
    data = request.get_json()
    
    if not data or 'scale' not in data:
        return jsonify({"error": "Lipseste campul 'scale' din corpul cererii."}), 400
        
    file_path = get_config_path(sensor_id)
    
    if os.path.exists(file_path):
        return jsonify({
            "error": f"Conflict: Fisierul de configurare pentru senzorul '{sensor_id}' exista deja. Folositi metoda PUT pentru modificare."
        }), 409

    try:
        config_content = {
            "scale": float(data['scale'])
        }
        
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(config_content, f, indent=4)
            
        return jsonify({
            "message": f"Fisierul de configurare pentru senzorul '{sensor_id}' a fost creat cu succes.",
            "filename": f"config_{sensor_id}.json",
            "config": config_content
        }), 201
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500


# Modificarea fisierului de configurare existent
@app.route('/sensors/<sensor_id>', methods=['PUT'])
def update_sensor_config(sensor_id):
    file_path = get_config_path(sensor_id)
    
    if not os.path.exists(file_path):
        return jsonify({
            "error": f"Fisierul de configurare pentru senzorul '{sensor_id}' nu exista. Nu se poate efectua actualizarea prin PUT."
        }), 409
        
    data = request.get_json()
    if not data or 'scale' not in data:
        return jsonify({"error": "Lipseste campul 'scale' din corpul cererii."}), 400
        
    try:
        config_content = {
            "scale": float(data['scale'])
        }
        
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(config_content, f, indent=4)
            
        return jsonify({
            "message": f"Fisierul de configurare pentru senzorul '{sensor_id}' a fost actualizat cu succes.",
            "config": config_content
        }), 200
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == '__main__':
    app.run(debug=True, port=5000)
