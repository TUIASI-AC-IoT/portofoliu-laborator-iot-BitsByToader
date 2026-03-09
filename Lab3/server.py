import io
from flask import Flask, send_file
import os.path

BINARY_NAME = "proj.bin"

app = Flask(__name__)

@app.route('/firmware.bin')
def firm():
    print('Start serve request')
    with open("proj/build/"+BINARY_NAME, 'rb') as bites:
        print(bites)
        return send_file(
                     io.BytesIO(bites.read()),
                     mimetype='application/octet-stream'
               )

@app.route("/")
def hello():
    return "Hello World!"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=53017, ssl_context=('proj/ca_cert.pem', 'proj/ca_key.pem'), debug=True)
