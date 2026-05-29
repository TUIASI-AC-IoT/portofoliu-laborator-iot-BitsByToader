import os
import uuid
from flask import Flask, jsonify, request

app = Flask(__name__)

STORAGE_DIR = 'sandbox_files'

if not os.path.exists(STORAGE_DIR):
    os.makedirs(STORAGE_DIR)

# List all files in the directory
@app.route('/files', methods=['GET'])
def list_directory_files():
    try:
        # Get everything in the folder but filter out subdirectories
        all_items = os.listdir(STORAGE_DIR)
        files = [f for f in all_items if os.path.isfile(os.path.join(STORAGE_DIR, f))]
        return jsonify({"files": files}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Get the text content of a specific file
@app.route('/files/<filename>', methods=['GET'])
def get_file_content(filename):
    file_path = os.path.join(STORAGE_DIR, filename)
    
    if not os.path.exists(file_path):
        return jsonify({"error": f"File '{filename}' not found."}), 404
        
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        return jsonify({"name": filename, "content": content}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Create a file (handles both named files and content-only files)
@app.route('/files', methods=['POST'])
def create_file():
    data = request.get_json()
    
    if not data or 'content' not in data:
        return jsonify({"error": "Missing 'content' field in request body."}), 400
        
    content = data['content']
    filename = data.get('name')
    
    if not filename:
        filename = f"document_{uuid.uuid4().hex[:8]}.txt"
        
    file_path = os.path.join(STORAGE_DIR, filename)
    
    # Don't overwrite an existing file accidentally via POST
    # Don't support adding to a file.
    if os.path.exists(file_path) and data.get('name'):
        return jsonify({"error": f"File '{filename}' already exists. Use PUT to modify it."}), 409

    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return jsonify({"message": "File created successfully", "filename": filename}), 201
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Delete a file by its name
@app.route('/files/<filename>', methods=['DELETE'])
def delete_file(filename):
    file_path = os.path.join(STORAGE_DIR, filename)
    
    if not os.path.exists(file_path):
        return jsonify({"error": f"File '{filename}' not found."}), 404
        
    try:
        os.remove(file_path)
        return jsonify({"message": f"File '{filename}' deleted successfully."}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Completely update/overwrite an existing file's content
@app.route('/files/<filename>', methods=['PUT'])
def update_file_content(filename):
    file_path = os.path.join(STORAGE_DIR, filename)
    
    if not os.path.exists(file_path):
        return jsonify({"error": f"File '{filename}' does not exist. Cannot update."}), 404
        
    data = request.get_json()
    if not data or 'content' not in data:
        return jsonify({"error": "Missing 'content' field in request body."}), 400
        
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(data['content'])
        return jsonify({"message": f"File '{filename}' updated successfully."}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    # Start the local development server
    app.run(debug=True, port=5000)
